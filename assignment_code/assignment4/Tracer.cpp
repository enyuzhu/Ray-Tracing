#include "Tracer.hpp"

#include <glm/gtx/string_cast.hpp>
#include <stdexcept>
#include <algorithm>

#include "gloo/Transform.hpp"
#include "gloo/components/MaterialComponent.hpp"
#include "gloo/lights/AmbientLight.hpp"

#include "gloo/Image.hpp"
#include "Illuminator.hpp"

namespace GLOO {
const float kEpsilon = 0.001f;

void Tracer::Render(const Scene& scene, const std::string& output_file) {
  scene_ptr_ = &scene;

  auto& root = scene_ptr_->GetRootNode();
  tracing_components_ = root.GetComponentPtrsInChildren<TracingComponent>();
  light_components_ = root.GetComponentPtrsInChildren<LightComponent>();

  Image image(image_size_.x, image_size_.y);

  for (size_t y = 0; y < image_size_.y; y++) {
    for (size_t x = 0; x < image_size_.x; x++) {
      // Normalize pixel coords to [-1, 1]
      float px = (2.0f * (x + 0.5f) / image_size_.x) - 1.0f;
      float py = (2.0f * (y + 0.5f) / image_size_.y) - 1.0f;  // Flipped: no "1.0f -"
      
      Ray ray = camera_.GenerateRay(glm::vec2(px, py));
      HitRecord record;
      glm::vec3 color = TraceRay(ray, max_bounces_, record);
      
      color = glm::clamp(color, 0.0f, 1.0f);
      image.SetPixel(x, y, color);
    }
  }

  if (output_file.size())
    image.SavePNG(output_file);
}

glm::vec3 Tracer::TraceRay(const Ray& ray, size_t bounces, HitRecord& record) const {
  bool hit_anything = false;
  float closest_t = std::numeric_limits<float>::max();
  glm::vec3 closest_normal_object;
  glm::vec3 closest_hit_world;
  TracingComponent* hit_component = nullptr;
  
  // Find closest intersection
  for (auto* component : tracing_components_) {
    auto& hittable = component->GetHittable();
    auto* node_ptr = component->GetNodePtr();
    
    // Transform ray to object space
    glm::mat4 world_to_object = glm::inverse(node_ptr->GetTransform().GetLocalToWorldMatrix());
    Ray object_ray = ray;
    object_ray.ApplyTransform(world_to_object);
    
    // Intersect in object space
    HitRecord temp_record;
    if (hittable.Intersect(object_ray, camera_.GetTMin(), temp_record)) {
      // Transform hit point to world space to get true distance
      glm::vec3 object_hit = object_ray.At(temp_record.time);
      glm::vec4 world_hit_4 = node_ptr->GetTransform().GetLocalToWorldMatrix() * glm::vec4(object_hit, 1.0f);
      glm::vec3 world_hit = glm::vec3(world_hit_4) / world_hit_4.w;
      
      float world_dist = glm::length(world_hit - ray.GetOrigin());
      
      if (world_dist < closest_t) {
        closest_t = world_dist;
        closest_normal_object = temp_record.normal;
        closest_hit_world = world_hit;
        hit_component = component;
        hit_anything = true;
      }
    }
  }
  
  if (!hit_anything) {
    return GetBackgroundColor(ray.GetDirection());
  }
  
  // Transform normal from object space to world space
  glm::mat4 object_to_world = hit_component->GetNodePtr()->GetTransform().GetLocalToWorldMatrix();
  glm::mat3 normal_transform = glm::transpose(glm::inverse(glm::mat3(object_to_world)));
  glm::vec3 normal_world = glm::normalize(normal_transform * closest_normal_object);
  
  record.time = closest_t;
  record.normal = normal_world;
  
  // Get material
  auto* material_comp = hit_component->GetNodePtr()->GetComponentPtr<MaterialComponent>();
  if (!material_comp) {
    return glm::vec3(1.0f, 0.0f, 1.0f); // Magenta for missing material
  }
  
  const Material& material = material_comp->GetMaterial();
  
  // Compute shading
  glm::vec3 color = ComputePhongShading(closest_hit_world, normal_world, ray.GetDirection(), material);
  
  // Reflections
  if (bounces > 0) {
    glm::vec3 spec_color = material.GetSpecularColor();
    float reflectivity = (spec_color.r + spec_color.g + spec_color.b) / 3.0f;
    
    if (reflectivity > 0.01f) {
      glm::vec3 reflect_dir = glm::reflect(glm::normalize(ray.GetDirection()), normal_world);
      Ray reflect_ray(closest_hit_world + kEpsilon * reflect_dir, reflect_dir);
      
      HitRecord reflect_record;
      glm::vec3 reflect_color = TraceRay(reflect_ray, bounces - 1, reflect_record);
      color += spec_color * reflect_color;
    }
  }
  
  return color;
}

glm::vec3 Tracer::ComputePhongShading(const glm::vec3& hit_point,
                                      const glm::vec3& normal,
                                      const glm::vec3& ray_dir,
                                      const Material& material) const {
  glm::vec3 color(0.0f);
  
  // Ambient
  for (auto* light_comp : light_components_) {
    if (light_comp->GetLightPtr()->GetType() == LightType::Ambient) {
      auto* ambient = static_cast<AmbientLight*>(light_comp->GetLightPtr());
      color += material.GetAmbientColor() * ambient->GetAmbientColor();
    }
  }
  
  glm::vec3 view_dir = glm::normalize(-ray_dir);
  
  // Direct lighting
  for (auto* light_comp : light_components_) {
    auto* light = light_comp->GetLightPtr();
    if (light->GetType() == LightType::Ambient) continue;
    
    glm::vec3 light_dir, light_intensity;
    float light_dist;
    Illuminator::GetIllumination(*light_comp, hit_point, light_dir, light_intensity, light_dist);
    
    // Shadows
    bool in_shadow = false;
    if (shadows_enabled_) {
      Ray shadow_ray(hit_point + kEpsilon * light_dir, light_dir);
      
      for (auto* comp : tracing_components_) {
        glm::mat4 w2o = glm::inverse(comp->GetNodePtr()->GetTransform().GetLocalToWorldMatrix());
        Ray shadow_object = shadow_ray;
        shadow_object.ApplyTransform(w2o);
        
        HitRecord shadow_hit;
        if (comp->GetHittable().Intersect(shadow_object, camera_.GetTMin(), shadow_hit)) {
          glm::vec3 shadow_obj_hit = shadow_object.At(shadow_hit.time);
          glm::vec4 shadow_world_4 = comp->GetNodePtr()->GetTransform().GetLocalToWorldMatrix() * glm::vec4(shadow_obj_hit, 1.0f);
          glm::vec3 shadow_world = glm::vec3(shadow_world_4) / shadow_world_4.w;
          
          if (glm::length(shadow_world - shadow_ray.GetOrigin()) < light_dist - kEpsilon) {
            in_shadow = true;
            break;
          }
        }
      }
    }
    
    if (in_shadow) continue;
    
    // Diffuse
    float n_dot_l = std::max(0.0f, glm::dot(normal, light_dir));
    glm::vec3 diffuse = n_dot_l * material.GetDiffuseColor() * light_intensity;
    
    // Specular
    glm::vec3 reflect_dir = glm::reflect(-light_dir, normal);
    float spec_factor = std::pow(std::max(0.0f, glm::dot(reflect_dir, view_dir)), material.GetShininess());
    glm::vec3 specular = spec_factor * material.GetSpecularColor() * light_intensity;
    
    color += diffuse + specular;
  }
  
  return color;
}

glm::vec3 Tracer::GetBackgroundColor(const glm::vec3& direction) const {
  if (cube_map_ != nullptr) {
    return cube_map_->GetTexel(direction);
  }
  return background_color_;
}

}  // namespace GLOO
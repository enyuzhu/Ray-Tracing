#include "Illuminator.hpp"

#include <limits>
#include <stdexcept>

#include <glm/geometric.hpp>

#include "gloo/lights/DirectionalLight.hpp"
#include "gloo/lights/PointLight.hpp"
#include "gloo/SceneNode.hpp"

namespace GLOO {
void Illuminator::GetIllumination(const LightComponent& light_component,
                                  const glm::vec3& hit_pos,
                                  glm::vec3& dir_to_light,
                                  glm::vec3& intensity,
                                  float& dist_to_light) {
  auto light_ptr = light_component.GetLightPtr();
  
  if (light_ptr->GetType() == LightType::Directional) {
    auto directional_light_ptr = static_cast<DirectionalLight*>(light_ptr);
    dir_to_light = -directional_light_ptr->GetDirection();
    intensity = directional_light_ptr->GetDiffuseColor();
    dist_to_light = std::numeric_limits<float>::max();
  } 
  else if (light_ptr->GetType() == LightType::Point) {  
    auto point_light_ptr = static_cast<PointLight*>(light_ptr);
    
    // Get light position from transform matrix
    glm::mat4 light_transform = light_component.GetNodePtr()->GetTransform().GetLocalToWorldMatrix();
    glm::vec3 light_pos = glm::vec3(light_transform[3]);  // Extract translation from matrix
    
    // Vector from hit point to light
    glm::vec3 light_vector = light_pos - hit_pos;
    
    // Distance to light
    dist_to_light = glm::length(light_vector);
    
    // Direction to light (normalized)
    dir_to_light = glm::normalize(light_vector);
    
    // Get attenuation
    glm::vec3 attenuation = point_light_ptr->GetAttenuation();
    float alpha = attenuation.x;
    
    // Apply inverse-square law with attenuation
    // I = I_base / (alpha * d^2)
    float falloff = 1.0f / (alpha * dist_to_light * dist_to_light);
    
    // Final intensity
    intensity = point_light_ptr->GetDiffuseColor() * falloff;
  } 
  else { 
    throw std::runtime_error("Unrecognized light type when computing illumination");
  }
}
}  // namespace GLOO
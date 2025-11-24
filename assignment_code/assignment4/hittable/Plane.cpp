#include "Plane.hpp"

namespace GLOO {
Plane::Plane(const glm::vec3& normal, float d) {
  normal_ = glm::normalize(normal);
  d_ = d;
}

bool Plane::Intersect(const Ray& ray, float t_min, HitRecord& record) const {
  const glm::vec3& ray_origin = ray.GetOrigin();
  const glm::vec3& ray_direction = ray.GetDirection();
  
  float denom = glm::dot(ray_direction, normal_);
  
  // Check if ray is parallel to plane
  if (std::abs(denom) < 1e-6f) {
    return false;
  }
  
  float t = (d_ - glm::dot(ray_origin, normal_)) / denom;
  
  // Check if intersection is valid
  if (t < t_min || t >= record.time) {
    return false;
  }
  
  // Update hit record
  record.time = t;
  record.normal = normal_;  // Use normal as-is from scene file
  
  return true;
}
}  // namespace GLOO
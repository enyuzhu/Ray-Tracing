#include "Triangle.hpp"

#include <iostream>
#include <stdexcept>

#include <glm/common.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Plane.hpp"

namespace GLOO {
Triangle::Triangle(const glm::vec3& p0,
                   const glm::vec3& p1,
                   const glm::vec3& p2,
                   const glm::vec3& n0,
                   const glm::vec3& n1,
                   const glm::vec3& n2) {
  positions_.push_back(p0);
  positions_.push_back(p1);
  positions_.push_back(p2);
  normals_.push_back(n0);
  normals_.push_back(n1);
  normals_.push_back(n2);
}

Triangle::Triangle(const std::vector<glm::vec3>& positions,
                   const std::vector<glm::vec3>& normals) {
  positions_ = positions;
  normals_ = normals;                    
}

bool Triangle::Intersect(const Ray& ray, float t_min, HitRecord& record) const {
  // Use barycentric coordinates method
  // Ray: R(t) = O + t*D
  // Triangle: P(β, γ) = v0 + β(v1 - v0) + γ(v2 - v0)
  // At intersection: O + t*D = v0 + β(v1 - v0) + γ(v2 - v0)
  // Rearrange: O - v0 = -t*D + β(v1 - v0) + γ(v2 - v0)
  // This is a 3x3 linear system: [-D, (v1-v0), (v2-v0)] * [t, β, γ]^T = (O - v0)
  
  const glm::vec3& v0 = positions_[0];
  const glm::vec3& v1 = positions_[1];
  const glm::vec3& v2 = positions_[2];
  
  const glm::vec3& ray_origin = ray.GetOrigin();
  const glm::vec3& ray_direction = ray.GetDirection();
  
  glm::vec3 e1 = v1 - v0;
  glm::vec3 e2 = v2 - v0;
  
  // Build matrix A = [-D, e1, e2]
  glm::mat3 A;
  A[0] = -ray_direction;
  A[1] = e1;
  A[2] = e2;
  
  glm::vec3 b = ray_origin - v0;
  
  // Check if matrix is singular (determinant near zero)
  float det = glm::determinant(A);
  if (std::abs(det) < 1e-6f) {
    return false;  // Ray parallel to triangle
  }
  
  // Solve: A * [t, β, γ]^T = b
  glm::mat3 A_inv = glm::inverse(A);
  glm::vec3 solution = A_inv * b;
  
  float t = solution[0];
  float beta = solution[1];
  float gamma = solution[2];
  float alpha = 1.0f - beta - gamma;
  
  // Check barycentric coordinates
  // Point is inside triangle if: β ≥ 0, γ ≥ 0, β + γ ≤ 1
  if (beta < 0.0f || gamma < 0.0f || (beta + gamma) > 1.0f) {
    return false;  // Intersection outside triangle
  }
  
  // Check if intersection is valid (in front of ray and closer than current hit)
  if (t < t_min || t >= record.time) {
    return false;
  }
  
  // Interpolate normal using barycentric coordinates
  // N = α*n0 + β*n1 + γ*n2, where α = 1 - β - γ
  glm::vec3 interpolated_normal = alpha * normals_[0] + 
                                  beta * normals_[1] + 
                                  gamma * normals_[2];
  interpolated_normal = glm::normalize(interpolated_normal);
  
  // Update hit record
  record.time = t;
  record.normal = interpolated_normal;
  
  return true;
}
}  // namespace GLOO

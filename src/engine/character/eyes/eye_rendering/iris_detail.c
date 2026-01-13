#include <character/eyes/eye_rendering/eye_rendering.h>
#include "engine/include/common.h"
#include <math/vec2.h>
#include <math/vec3.h>

// High-detail iris parallax and texture sampling facade
vec2_t calculate_iris_uv(vec3_t V, eye_params_t *params) {
  // This function calculates the UV coordinates for the iris
  // by refracting the view vector V through the cornea.

  // Cornea refraction stub (Snell's Law)
  float eta = 1.0f / params->cornea_ior;
  float dot_vn = fmaxf(0.0f, vec3_dot(V, params->eye_forward));

  // Simplified refraction vector placeholder
  vec3_t R =
      vec3_sub(vec3_mul(V, eta),
               vec3_mul(params->eye_forward,
                        (eta * dot_vn +
                         sqrtf(1.0f - eta * eta * (1.0f - dot_vn * dot_vn)))));

  // Intersection with iris plane at params->iris_depth
  float t = params->iris_depth / fmaxf(0.01f, vec3_dot(R, params->eye_forward));
  vec3_t P = vec3_mul(R, t);

  // Project P onto 2D UV plane
  return vec2(P.x * 0.5f + 0.5f, P.z * 0.5f + 0.5f);
}

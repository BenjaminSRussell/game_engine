#include "character/eyes/eye_rendering/eye_rendering.h"
#include <include/math/math.h>

/**
 * @file eye_refraction.c
 * @brief Implementation of cornea refraction and iris parallax
 */

// Snell's Law refraction helper
static vec3_t refract_v3(vec3_t I, vec3_t N, float eta) {
    float dot_ni = vec3_dot(N, I);
    float k = 1.0f - eta * eta * (1.0f - dot_ni * dot_ni);
    if (k < 0.0f) {
        return vec3_zero(); // Total internal reflection
    }
    return vec3_sub(vec3_scale(I, eta), vec3_scale(N, (eta * dot_ni + sqrtf(k))));
}

vec2_t calculate_iris_uv(vec3_t refracted, eye_params_t* params) {
    // 1. Ray-plane intersection with iris plane
    // Iris plane is at eye_center + eye_forward * -iris_depth
    vec3_t iris_origin = vec3_set(
        params->eye_center.x - params->eye_forward.x * params->iris_depth,
        params->eye_center.y - params->eye_forward.y * params->iris_depth,
        params->eye_center.z - params->eye_forward.z * params->iris_depth
    );
    
    // Simplification for spherical eye: intersect refracted ray from cornea surface
    // with plane offset by iris_depth
    float t = -params->iris_depth / vec3_dot(refracted, params->eye_forward);
    vec3_t hit = vec3_scale(refracted, t);
    
    // 2. Project hit point to 2D UV
    // TODO: Implement proper coordinate frame projection
    return vec2_set(hit.x * 0.5f + 0.5f, hit.y * 0.5f + 0.5f);
}

vec3_t shade_eye(vec3_t V, vec3_t N, eye_params_t* params) {
    // 1. Refract view through cornea
    vec3_t refracted = refract_v3(vec3_scale(V, -1.0f), N, 1.0f / params->cornea_ior);
    
    // 2. Sample iris with parallax
    vec2_t iris_uv = calculate_iris_uv(refracted, params);
    
    // TODO: Implement iris texture sampling
    vec3_t iris_color = params->iris_color;
    
    // 3. Add cornea specular
    // TODO: Implement Cook-Torrance specular for cornea
    
    return iris_color;
}

// TODO: Implement pupil dilation UV warping
// TODO: Implement limbus darkening blend

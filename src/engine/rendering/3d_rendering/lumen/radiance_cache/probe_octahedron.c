/*
 * probe_octahedron.c
 * Octahedral Mapping for Radiance Probes
 *
 * Part of the Lumen GI subsystem - Radiance Cache
 * Advanced 3D Rendering Engine
 */

#include "probe_octahedron.h"
#include "../../math/vec2.h"
#include "../../math/vec3.h"
#include <math.h>

/*
 * Utilities for Octahedral Mapping, used to store spherical radiance
 * functions in 2D textures (probes).
 */

static float sign_not_zero(float v) {
    return (v >= 0.0f) ? +1.0f : -1.0f;
}

vec2_t oct_encode(vec3_t v) {
    float l1norm = fabsf(v.x) + fabsf(v.y) + fabsf(v.z);
    vec2_t result;
    result.x = v.x / l1norm;
    result.y = v.y / l1norm;
    
    if (v.z < 0.0f) {
        float tx = result.x;
        float ty = result.y;
        result.x = (1.0f - fabsf(ty)) * sign_not_zero(tx);
        result.y = (1.0f - fabsf(tx)) * sign_not_zero(ty);
    }
    
    // Transform to [0, 1] for texture storage? usually kept in [-1, 1] or mapped.
    return result;
}

vec3_t oct_decode(vec2_t e) {
    vec3_t v;
    v.x = e.x;
    v.y = e.y;
    v.z = 1.0f - (fabsf(e.x) + fabsf(e.y));
    
    if (v.z < 0.0f) {
        float tx = v.x;
        float ty = v.y;
        v.x = (1.0f - fabsf(ty)) * sign_not_zero(tx);
        v.y = (1.0f - fabsf(tx)) * sign_not_zero(ty);
    }
    
    // Normalize?
    // v = normalize(v);
    return v;
}

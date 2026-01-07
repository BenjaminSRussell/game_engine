/*
 * tear_damage.c
 * Damage patterns implementation
 *
 * Part of the Wear & Tear subsystem
 * Advanced 3D Rendering Engine
 */

#include "materials/weathering/wear/tear_damage.h"
#include "math/math.h"
#include <math/math.h>

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static float hash13(vec3_t p) {
    vec3_t p3 = vec3_multiply_scalar((vec3_t){sinf(p.x * 12.9898f), sinf(p.y * 78.233f), sinf(p.z * 45.164f)}, 43758.5453123f);
    return p3.x - floorf(p3.x);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

float damage_generate_pattern(const damage_params_t* params, vec3_t world_pos) {
    float val = 0.0f;
    vec3_t p = vec3_multiply_scalar(world_pos, params->scale);

    switch (params->type) {
        case DAMAGE_TYPE_SCRATCH: {
            // Long thin lines
            float line = fabsf(sinf(p.x + p.y + p.z) * 0.5f + sinf(p.x * 0.1f));
            val = saturate(1.0f - powf(line, 0.05f));
            break;
        }
        case DAMAGE_TYPE_DENT: {
            // Circular impact marks
            vec3_t cell = {floorf(p.x), floorf(p.y), floorf(p.z)};
            float h = hash13(cell);
            if (h > 0.95f) {
                vec3_t center = vec3_add_scalar(cell, 0.5f);
                float dist = vec3_distance(p, center);
                val = saturate(1.0f - dist * 2.0f);
            }
            break;
        }
        case DAMAGE_TYPE_CRACK: {
            // Sharp branching lines
            float n = fabsf(sinf(p.x * 3.0f) * cosf(p.z * 3.0f));
            val = saturate(1.0f - powf(n, 0.01f));
            break;
        }
        default: break;
    }

    return val * params->depth;
}

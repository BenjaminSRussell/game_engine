/*
 * grunge_masks.c
 * Procedural grunge implementation
 *
 * Part of the Dirt & Grunge subsystem
 * Advanced 3D Rendering Engine
 */

#include "materials/weathering/dirt/grunge_masks.h"
#include "math/math.h"
#include <math/math.h>

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static float simple_noise_st(float x, float y) {
    return (sinf(x) * cosf(y)) * 0.5f + 0.5f;
}

static float streak_noise(vec3_t pos, float freq) {
    // Stretches noise vertically to create streaks
    return simple_noise_st(pos.x * freq, pos.y * freq * 0.1f + pos.z * freq);
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

float grunge_generate_mask(const grunge_params_t* params, vec3_t world_pos, vec3_t normal) {
    float mask = 0.0f;
    float f = params->frequency;

    switch (params->type) {
        case GRUNGE_TYPE_STREAKS: {
            // High vertical frequency, low horizontal
            float s = streak_noise(world_pos, f);
            // Bias towards vertical surfaces
            float vertical_bias = saturate(1.0f - normal.y);
            mask = s * vertical_bias;
            break;
        }
        case GRUNGE_TYPE_SPOTS: {
            float s = simple_noise_st(world_pos.x * f, world_pos.z * f);
            mask = powf(s, 10.0f); // Make it sharp spots
            break;
        }
        case GRUNGE_TYPE_STAIN: {
            float s = simple_noise_st(world_pos.x * f * 0.2f, world_pos.z * f * 0.2f);
            mask = saturate(s * 1.5f - 0.5f); // Diffused blotch
            break;
        }
        default: mask = 0.0f; break;
    }

    return saturate(mask * params->intensity);
}

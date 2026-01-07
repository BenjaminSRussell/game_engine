/*
 * cascade_blending.h
 * Seamless cascade blending
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADE_BLENDING_H
#define LIGHTING_CASCADE_BLENDING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cascade_blend_info {
    float start_fade;    // Depth where fade begins
    float end_depth;     // Depth where cascade ends
    float blend_size;    // Size of blend region
} cascade_blend_info_t;

/**
 * @brief Calculate blending bands for smooth cascade transitions
 * 
 * @param splits Array of n split depths
 * @param count Number of cascades
 * @param blend_fraction Fraction size of overlap region (0.0 - 0.5)
 * @param out_bands Output array of blend info (size = count)
 */
void cascade_calculate_blend_bands(const float* splits, uint32_t count, float blend_fraction, 
                                  cascade_blend_info_t* out_bands);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADE_BLENDING_H */

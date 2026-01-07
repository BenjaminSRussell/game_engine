/*
 * cascade_resolution.h
 * Cascade resolution management
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADE_RESOLUTION_H
#define LIGHTING_CASCADE_RESOLUTION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum cascade_quality_preset {
    CASCADE_QUALITY_LOW,
    CASCADE_QUALITY_MEDIUM,
    CASCADE_QUALITY_HIGH,
    CASCADE_QUALITY_ULTRA
} cascade_quality_preset_t;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * @brief Calculate optimal resolution for a cascade layer
 * 
 * @param frustum_diagonal_size Size of the cascade frustum diagonal in world units
 * @param texel_density_target Desired pixels per world unit
 * @return Power-of-two resolution
 */
uint32_t cascade_calculate_resolution(float frustum_diagonal_size, float texel_density_target);

/**
 * @brief Calculate current texel density
 * 
 * @param resolution Map resolution
 * @param world_diagonal_size World space diagonal size covered
 * @return Pixels per world unit
 */
float cascade_calculate_texel_density(uint32_t resolution, float world_diagonal_size);

/**
 * @brief Apply quality preset to generate resolutions for cascades
 * 
 * @param preset Quality level
 * @param out_resolutions Array to fill with resolutions
 * @param count Number of cascades
 */
void cascade_apply_quality_preset(cascade_quality_preset_t preset, uint32_t* out_resolutions, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADE_RESOLUTION_H */

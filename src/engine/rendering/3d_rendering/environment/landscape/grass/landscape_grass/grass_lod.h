/*
 * grass_lod.h
 * Grass Level of Detail System
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GRASS_LOD_H
#define GRASS_LOD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
int grass_lod_init(void);
void grass_lod_shutdown(void);

/* Operations */
void grass_lod_update(float camera_x, float camera_z);
int grass_lod_get_level(float distance);
float grass_lod_get_density(uint32_t lod_level);

/* Configuration */
void grass_lod_set_distances(const float* distances, uint32_t count);
void grass_lod_set_density_scales(const float* scales, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* GRASS_LOD_H */

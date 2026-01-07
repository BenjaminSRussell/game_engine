/*
 * probe_parallax.h
 * Probe parallax correction
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PROBE_PARALLAX_H
#define LIGHTING_PROBE_PARALLAX_H

#include <simd/simd.h>
#include "irradiance_probe.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Applies parallax correction to a reflection vector or lookup position based on the probe's bounding box.
 * This is primarily for reflection probes but can be used for localizing irradiance lookups.
 * 
 * @param world_pos The surface position being shaded.
 * @param view_dir The view direction (unused for simple position correction, but used for reflection vector correction).
 * @param reflection_vector The original reflection vector (normalized).
 * @param probe_pos The center of the probe.
 * @param box_min The min bounds of the probe's influence volume.
 * @param box_max The max bounds of the probe's influence volume.
 * @return The corrected vector/position.
 */

// Calculates the intersection of a ray with an AABB
simd_float3 probe_parallax_intersect_aabb(simd_float3 ray_origin, simd_float3 ray_dir, simd_float3 box_min, simd_float3 box_max);

/**
 * Corrects the reflection vector for box projection.
 * @param world_pos Surface position.
 * @param reflection_dir Normalized reflection direction.
 * @param probe_pos Center of the probe.
 * @param box_min AABB min.
 * @param box_max AABB max.
 * @return Corrected sampling position in AABB space (for cubemap lookup).
 */
simd_float3 probe_parallax_box_projection(simd_float3 world_pos, simd_float3 reflection_dir, simd_float3 probe_pos, simd_float3 box_min, simd_float3 box_max);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PROBE_PARALLAX_H */

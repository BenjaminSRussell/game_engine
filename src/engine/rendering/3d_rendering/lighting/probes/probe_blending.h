/*
 * probe_blending.h
 * Probe blending logic
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PROBE_BLENDING_H
#define LIGHTING_PROBE_BLENDING_H

#include <simd/simd.h>
#include "irradiance_probe.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Calculates the blend weight for a probe at the given world position.
 * @param world_pos The point being shaded.
 * @param probe The probe being sampled.
 * @return A weight between 0.0 and 1.0.
 */
float probe_calculate_weight(simd_float3 world_pos, const irradiance_probe_t* probe);

/**
 * Blends irradiance from multiple probes based on their weights.
 * @param probe_indices Array of probe indices to blend.
 * @param weights Array of weights for each probe.
 * @param count Number of probes.
 * @param grid The probe grid.
 * @param normal The surface normal (for SH evaluation).
 * @return The blended irradiance color.
 */
simd_float3 probe_blend_irradiance(const uint32_t* probe_indices, const float* weights, uint32_t count, const probe_grid_t* grid, simd_float3 normal);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PROBE_BLENDING_H */

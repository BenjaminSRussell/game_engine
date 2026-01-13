// include/core/lod_generator.h
//
// Purpose: Runtime LOD (Level of Detail) generation for meshes.
// Provides utilities to automatically generate simplified versions of meshes
// for improved rendering performance at distance.
//
// Public APIs:
// - `lod_generate_levels`: Generate multiple LOD levels from a source mesh
// - `lod_generate_single`: Generate a single LOD level with specified reduction
//
// The generator uses progressive mesh decimation to reduce triangle count
// while preserving the overall shape and silhouette of the mesh.

#ifndef LOD_GENERATOR_H
#define LOD_GENERATOR_H

#include "include/common.h"
#include <include/rendering/mesh.h>

// Generate multiple LOD levels from a source mesh
// Parameters:
//   source: High-detail source mesh
//   lod_array: Output array for generated LOD meshes (must be pre-allocated)
//   num_levels: Number of LOD levels to generate
//   reduction_factors: Array of reduction factors (0.0-1.0) for each level
//                      e.g., [0.5, 0.75, 0.9] = 50%, 75%, 90% reduction
// Returns: true if all levels generated successfully
bool lod_generate_levels(Mesh *source, Mesh *lod_array, u32 num_levels,
                         const f32 *reduction_factors);

// Generate a single LOD level
// Parameters:
//   source: High-detail source mesh
//   dest: Output mesh for the LOD level
//   reduction_factor: Triangle reduction factor (0.0-1.0)
// Returns: true if generation successful
bool lod_generate_single(Mesh *source, Mesh *dest, f32 reduction_factor);

// Calculate optimal reduction factors for a given number of LOD levels
// Parameters:
//   num_levels: Number of LOD levels desired
//   out_factors: Output array for reduction factors (must be pre-allocated)
void lod_calculate_reduction_factors(u32 num_levels, f32 *out_factors);

#endif // LOD_GENERATOR_H

// include/procedural/terrain_utils.h
//
// Purpose: Header file for terrain generation utility functions
// Provides noise functions, smoothing algorithms, and terrain analysis tools

#ifndef TERRAIN_UTILS_H
#define TERRAIN_UTILS_H

#include "../core/core.h"

#ifdef __cplusplus
extern "C" {
#endif

// Apply Gaussian smoothing to heightmap data
// Parameters:
//   heightmap - Pointer to heightmap data (will be modified in-place)
//   width - Width of the heightmap
//   height - Height of the heightmap  
//   intensity - Smoothing intensity (0.0 = no smoothing, 1.0 = full smoothing)
void apply_gaussian_smooth(f32 *heightmap, u32 width, u32 height, f32 intensity);

// Multi-octave Perlin noise function
// Parameters:
//   x, y - Coordinates in noise space
//   octaves - Number of noise layers to combine
//   persistence - How much each octave contributes (0.0 to 1.0)
// Returns: Noise value in range [-1.0, 1.0]
f32 perlin_noise(f32 x, f32 y, u32 octaves, f32 persistence);

#ifdef __cplusplus
}
#endif

#endif // TERRAIN_UTILS_H

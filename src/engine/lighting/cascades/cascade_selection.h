/*
 * cascade_selection.h
 * Runtime cascade selection
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADE_SELECTION_H
#define LIGHTING_CASCADE_SELECTION_H

#include <stdint.h>
#include <include/math/math_all.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate split depths using Practical Split Scheme
 * 
 * @param near_plane Camera near plane
 * @param far_plane Shadow max distance
 * @param lambda Split distribution factor (0.0=Uniform, 1.0=Logarithmic, 0.5=Hybrid)
 * @param count Number of cascades
 * @param out_splits Output array of float depths (size = count)
 */
void cascade_calculate_split_depths(float near_plane, float far_plane, float lambda, uint32_t count, float* out_splits);

/**
 * @brief Calculate transition regions for smooth blending
 * 
 * @param splits Array of split depths
 * @param count Number of cascades
 * @param transition_fraction Fraction of cascade depth to use as transition (e.g. 0.1)
 * @param out_regions Output buffer for transition parameters
 */
void cascade_calculate_transition_regions(const float* splits, uint32_t count, float transition_fraction, float* out_regions);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADE_SELECTION_H */

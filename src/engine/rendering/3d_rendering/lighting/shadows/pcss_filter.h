/*
 * pcss_filter.h
 * PCSS (Percentage Closer Soft Shadows)
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PCSS_FILTER_H
#define LIGHTING_PCSS_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate blocker search radius
 * Used in step 1 of PCSS
 */
float pcss_blocker_search_radius(float light_size, float receiver_depth);

/**
 * @brief Calculate penumbra size (kernel radius for PCF step)
 * Used in step 2 of PCSS
 * 
 * @param receiver_depth Depth of our pixel
 * @param blocker_depth Average depth of blockers found in step 1
 * @param light_size Physical size of light source
 */
float pcss_penumbra_size(float receiver_depth, float blocker_depth, float light_size);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PCSS_FILTER_H */

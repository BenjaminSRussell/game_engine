/*
 * cascade_culling.h
 * Per-cascade frustum culling
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADE_CULLING_H
#define LIGHTING_CASCADE_CULLING_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Cull shadow casters against a single cascade frustum
 * 
 * @param cascade_view_proj Cascade View-Projection matrix (16 floats)
 * @param render_proxies Array of render proxies (implementation dependent opaque pointer)
 * @param proxy_count Number of proxies
 * @param out_visible_indices Output array for visible indices
 * @param out_visible_count Output count of visible proxies
 */
void cascade_cull_casters(const float* cascade_view_proj, 
                          const void* render_proxies, 
                          uint32_t proxy_count,
                          uint32_t* out_visible_indices,
                          uint32_t* out_visible_count);

/**
 * @brief Compute bounds of the receiver volume (view frustum slice)
 * Used to optimize shadow map projection (fit to receivers)
 * 
 * @param camera_view_proj Main camera VP
 * @param cascade_near Near distance of cascade
 * @param cascade_far Far distance of cascade
 * @param out_min Output AABB min (3 floats)
 * @param out_max Output AABB max (3 floats)
 */
void cascade_compute_receiver_bounds(const float* camera_view_proj,
                                    float cascade_near,
                                    float cascade_far,
                                    float* out_min,
                                    float* out_max);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADE_CULLING_H */

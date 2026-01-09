/**
 * blend_2d.h - 2D animation blending
 * 
 * Provides functions for blending animations based on 2D parameters.
 */
#ifndef ANIMATION_BLEND_TREES_BLEND_2D_H
#define ANIMATION_BLEND_TREES_BLEND_2D_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize 2D blend system.
 */
void blend_2d_init(void);

/**
 * Add animation clip at a specific 2D position in the blend space.
 * 
 * @param blend The blend space handle
 * @param clip  The animation clip to add
 * @param x     X position in blend space (e.g., movement speed)
 * @param y     Y position in blend space (e.g., direction)
 */
void blend_2d_add_clip(void *blend, void *clip, float x, float y);

/**
 * Evaluate 2D blend and output the resulting pose.
 * 
 * @param blend       The blend space handle
 * @param param_x     Current X parameter value
 * @param param_y     Current Y parameter value
 * @param output_pose Output pose buffer
 */
void blend_2d_evaluate(void *blend, float param_x, float param_y, void *output_pose);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_BLEND_TREES_BLEND_2D_H

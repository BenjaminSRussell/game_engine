/**
 * blend_1d.h - 1D animation blending
 * 
 * Provides functions for blending animations based on a 1D parameter.
 */
#ifndef ANIMATION_BLEND_TREES_BLEND_1D_H
#define ANIMATION_BLEND_TREES_BLEND_1D_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize 1D blend system.
 */
void blend_1d_init(void);

/**
 * Add animation clip at a specific threshold in the 1D blend space.
 * 
 * @param blend     The blend space handle
 * @param clip      The animation clip to add
 * @param threshold The parameter threshold where this clip is fully active
 */
void blend_1d_add_clip(void *blend, void *clip, float threshold);

/**
 * Evaluate 1D blend and output the resulting pose.
 * 
 * @param blend       The blend space handle
 * @param parameter   The current parameter value
 * @param output_pose Output pose buffer
 */
void blend_1d_evaluate(void *blend, float parameter, void *output_pose);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_BLEND_TREES_BLEND_1D_H

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

typedef struct BlendSpace1D BlendSpace1D;
struct Pose;
struct AnimationClip;

/**
 * Create a new 1D blend space.
 */
BlendSpace1D* blend_1d_create(void);

/**
 * Destroy a 1D blend space.
 */
void blend_1d_destroy(BlendSpace1D* blend);

/**
 * Add animation clip at a specific threshold in the 1D blend space.
 * 
 * @param blend     The blend space handle
 * @param clip      The animation clip to add
 * @param threshold The parameter threshold where this clip is fully active
 */
void blend_1d_add_clip(BlendSpace1D *blend, struct AnimationClip *clip, float threshold);

/**
 * Evaluate 1D blend and output the resulting pose.
 * 
 * @param blend       The blend space handle
 * @param time        The current playback time
 * @param parameter   The current parameter value
 * @param output_pose Output pose buffer
 */
void blend_1d_evaluate(BlendSpace1D *blend, float time, float parameter, struct Pose *output_pose);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_BLEND_TREES_BLEND_1D_H

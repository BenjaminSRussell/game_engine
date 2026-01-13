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

typedef struct BlendSpace2D BlendSpace2D;
struct Pose;
struct AnimationClip;

/**
 * Create a new 2D blend space.
 */
BlendSpace2D* blend_2d_create(void);

/**
 * Destroy a 2D blend space.
 */
void blend_2d_destroy(BlendSpace2D* blend);

/**
 * Add animation clip at a specific 2D position in the blend space.
 * 
 * @param blend The blend space handle
 * @param clip  The animation clip to add
 * @param x     X position in blend space (e.g., movement speed)
 * @param y     Y position in blend space (e.g., direction)
 */
void blend_2d_add_clip(BlendSpace2D *blend, struct AnimationClip *clip, float x, float y);

/**
 * Evaluate 2D blend and output the resulting pose.
 * 
 * @param blend       The blend space handle
 * @param time        The current playback time
 * @param param_x     Current X parameter value
 * @param param_y     Current Y parameter value
 * @param output_pose Output pose buffer
 */
void blend_2d_evaluate(BlendSpace2D *blend, float time, float param_x, float param_y, struct Pose *output_pose);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_BLEND_TREES_BLEND_2D_H

/**
 * additive_blend.h - Additive animation blending
 * 
 * Provides functions for applying additive animations on top of base poses.
 */
#ifndef ANIMATION_BLEND_TREES_ADDITIVE_BLEND_H
#define ANIMATION_BLEND_TREES_ADDITIVE_BLEND_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize additive blend system.
 */
void additive_blend_init(void);

/**
 * Apply an additive animation pose on top of a base pose.
 * 
 * @param base_pose     The base pose to add to
 * @param additive_pose The additive pose to apply  
 * @param weight        Blend weight (0.0 - 1.0)
 * @param output_pose   Output pose buffer
 */
void additive_blend_apply(void *base_pose, void *additive_pose, float weight, void *output_pose);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_BLEND_TREES_ADDITIVE_BLEND_H

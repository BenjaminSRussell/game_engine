/**
 * @file blend_space_2d.h
 * @brief 2D Blend Space for animation blending.
 */
#ifndef ANIMATION_CORE_BLEND_SPACE_2D_H
#define ANIMATION_CORE_BLEND_SPACE_2D_H

#include <math/vec2.h>
#include <math/vec3.h>
#include <animation/animation_system.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration
typedef struct BlendSpace2D BlendSpace2D;

/**
 * Compute barycentric weights for a point inside a triangle.
 */
vec3 get_barycentric_weights(vec2 p, vec2 a, vec2 b, vec2 c);

/**
 * Evaluate the blend space with given 2D input parameters.
 */
void blend_space_evaluate(BlendSpace2D *bs, vec2 input, Pose *out_pose);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_CORE_BLEND_SPACE_2D_H

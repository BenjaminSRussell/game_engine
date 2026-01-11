/**
 * @file blend_space_2d.c
 * @brief 2D Blend Space.
 *
 * Blends multiple animation clips based on 2 input parameters (e.g. Speed,
 * Direction). Uses Barycentric coordinate interpolation on a triangulated grid.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include "character/animation/blending/blend_space_2d.h"
#include "include/animation/animation_system.h" // For Pose, pose_blend
#include "include/math/math.h"

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

typedef struct BlendSample {
  Vec2 point; // X=Direction (-180 to 180), Y=Speed (0 to 600)
  uint32_t clip_id;
} BlendSample;

typedef struct BlendTriangle {
  int idx0, idx1, idx2; // Indices into samples array
} BlendTriangle;

typedef struct BlendSpace2D {
  BlendSample samples[16];
  int sample_count;
  BlendTriangle triangles[32]; // Triangulation of points
  int triangle_count;
  Vec2 min_val, max_val;
} BlendSpace2D;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Computes barycentric weights for a point inside a triangle.
 */
Vec3 get_barycentric_weights(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
  Vec2 v0 = vec2_sub(b, a);
  Vec2 v1 = vec2_sub(c, a);
  Vec2 v2 = vec2_sub(p, a);

  float d00 = vec2_dot(v0, v0);
  float d01 = vec2_dot(v0, v1);
  float d11 = vec2_dot(v1, v1);
  float d20 = vec2_dot(v2, v0);
  float d21 = vec2_dot(v2, v1);

  float denom = d00 * d11 - d01 * d01;

  float v = (d11 * d20 - d01 * d21) / denom;
  float w = (d00 * d21 - d01 * d20) / denom;
  float u = 1.0f - v - w;

  return (Vec3){u, v, w};
}

void blend_space_evaluate(BlendSpace2D *bs, Vec2 input, Pose *out_pose) {
  // 1. Find which triangle the input falls into
  BlendTriangle *tri = NULL;
  Vec3 weights;

  for (int i = 0; i < bs->triangle_count; i++) {
    Vec2 p0 = bs->samples[bs->triangles[i].idx0].point;
    Vec2 p1 = bs->samples[bs->triangles[i].idx1].point;
    Vec2 p2 = bs->samples[bs->triangles[i].idx2].point;

    weights = get_barycentric_weights(input, p0, p1, p2);

    if (weights.x >= 0 && weights.y >= 0 && weights.z >= 0) {
      tri = &bs->triangles[i];
      break;
    }
  }

  // Fallback: Find closest point if outside all triangles
  if (!tri) {
    // ... find closest sample ...
    // weights = (1, 0, 0)
    return;
  }

  // 2. Sample the 3 clips
  Pose p0, p1, p2;
  // clip_sample(bs->samples[tri->idx0].clip_id, time, &p0);
  // clip_sample(bs->samples[tri->idx1].clip_id, time, &p1);
  // clip_sample(bs->samples[tri->idx2].clip_id, time, &p2);

  // 3. Blend
  Pose temp;
  pose_blend(&p0, &p1, weights.y / (weights.x + weights.y),
             &temp); // Approximate blend
  pose_blend(&temp, &p2, weights.z, out_pose);
}

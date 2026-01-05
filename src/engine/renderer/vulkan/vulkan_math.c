#include <common.h>
#include <math.h>
#include <math/mat4.h>
#include <math/vec3.h>
#include <math/vec4.h>
#include <renderer/mesh_optimizer.h>

/**
 * Normalizes a plane represented as a Vec4.
 */
static void normalize_plane(Vec4 *plane) {
  float length =
      sqrtf(plane->x * plane->x + plane->y * plane->y + plane->z * plane->z);
  if (length > 0.0001f) {
    float inv_length = 1.0f / length;
    plane->x *= inv_length;
    plane->y *= inv_length;
    plane->z *= inv_length;
    plane->w *= inv_length;
  }
}

void frustum_from_matrix(Frustum *frustum, const Mat4 *view_proj) {
  if (!frustum || !view_proj)
    return;

  // Left plane
  frustum->planes[0] = vec4(view_proj->data[0][3] + view_proj->data[0][0],
                            view_proj->data[1][3] + view_proj->data[1][0],
                            view_proj->data[2][3] + view_proj->data[2][0],
                            view_proj->data[3][3] + view_proj->data[3][0]);
  normalize_plane(&frustum->planes[0]);

  // Right plane
  frustum->planes[1] = vec4(view_proj->data[0][3] - view_proj->data[0][0],
                            view_proj->data[1][3] - view_proj->data[1][0],
                            view_proj->data[2][3] - view_proj->data[2][0],
                            view_proj->data[3][3] - view_proj->data[3][0]);
  normalize_plane(&frustum->planes[1]);

  // Bottom plane
  frustum->planes[2] = vec4(view_proj->data[0][3] + view_proj->data[0][1],
                            view_proj->data[1][3] + view_proj->data[1][1],
                            view_proj->data[2][3] + view_proj->data[2][1],
                            view_proj->data[3][3] + view_proj->data[3][1]);
  normalize_plane(&frustum->planes[2]);

  // Top plane
  frustum->planes[3] = vec4(view_proj->data[0][3] - view_proj->data[0][1],
                            view_proj->data[1][3] - view_proj->data[1][1],
                            view_proj->data[2][3] - view_proj->data[2][1],
                            view_proj->data[3][3] - view_proj->data[3][1]);
  normalize_plane(&frustum->planes[3]);

  // Near plane
  frustum->planes[4] = vec4(view_proj->data[0][3] + view_proj->data[0][2],
                            view_proj->data[1][3] + view_proj->data[1][2],
                            view_proj->data[2][3] + view_proj->data[2][2],
                            view_proj->data[3][3] + view_proj->data[3][2]);
  normalize_plane(&frustum->planes[4]);

  // Far plane
  frustum->planes[5] = vec4(view_proj->data[0][3] - view_proj->data[0][2],
                            view_proj->data[1][3] - view_proj->data[1][2],
                            view_proj->data[2][3] - view_proj->data[2][2],
                            view_proj->data[3][3] - view_proj->data[3][2]);
  normalize_plane(&frustum->planes[5]);
}

bool frustum_test_sphere(const Frustum *frustum, Vec3 center, float radius) {
  if (!frustum)
    return false;

  for (int i = 0; i < 6; i++) {
    float dist = frustum->planes[i].x * center.x +
                 frustum->planes[i].y * center.y +
                 frustum->planes[i].z * center.z + frustum->planes[i].w;
    if (dist <= -radius) {
      return false;
    }
  }
  return true;
}

bool frustum_test_aabb(const Frustum *frustum, Vec3 min, Vec3 max) {
  if (!frustum)
    return false;

  // Optimized AABB-plane test from
  // http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/
  for (int i = 0; i < 6; i++) {
    Vec3 positive = min;
    Vec3 negative = max;

    if (frustum->planes[i].x >= 0) {
      positive.x = max.x;
      negative.x = min.x;
    }
    if (frustum->planes[i].y >= 0) {
      positive.y = max.y;
      negative.y = min.y;
    }
    if (frustum->planes[i].z >= 0) {
      positive.z = max.z;
      negative.z = min.z;
    }

    if (frustum->planes[i].x * positive.x + frustum->planes[i].y * positive.y +
            frustum->planes[i].z * positive.z + frustum->planes[i].w <
        0) {
      return false;
    }
  }
  return true;
}

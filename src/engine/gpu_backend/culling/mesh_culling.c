/**
 * MESH CULLING SYSTEM
 * Frustum and Occlusion culling for high-performance rendering
 */

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct {
  float m[4][4]; // World View Projection matrix
} Mat4;

typedef struct {
  float planes[6][4]; // frustum planes: x, y, z, distance
} Frustum;

typedef struct {
  float min[3];
  float max[3];
} AABB;

typedef struct {
  float center[3];
  float radius;
} Sphere;

// Extract frustum planes from WVP matrix
void frustum_extract(Frustum *f, const Mat4 *wvp) {
  const float *m = (const float *)wvp->m;

  // Left
  f->planes[0][0] = m[3] + m[0];
  f->planes[0][1] = m[7] + m[4];
  f->planes[0][2] = m[11] + m[8];
  f->planes[0][3] = m[15] + m[12];

  // Right
  f->planes[1][0] = m[3] - m[0];
  f->planes[1][1] = m[7] - m[4];
  f->planes[1][2] = m[11] - m[8];
  f->planes[1][3] = m[15] - m[12];

  // Bottom
  f->planes[2][0] = m[3] + m[1];
  f->planes[2][1] = m[7] + m[5];
  f->planes[2][2] = m[11] + m[9];
  f->planes[2][3] = m[15] + m[13];

  // Top
  f->planes[3][0] = m[3] - m[1];
  f->planes[3][1] = m[7] - m[5];
  f->planes[3][2] = m[11] - m[9];
  f->planes[3][3] = m[15] - m[13];

  // Near
  f->planes[4][0] = m[3] + m[2];
  f->planes[4][1] = m[7] + m[6];
  f->planes[4][2] = m[11] + m[10];
  f->planes[4][3] = m[15] + m[14];

  // Far
  f->planes[5][0] = m[3] - m[2];
  f->planes[5][1] = m[7] - m[6];
  f->planes[5][2] = m[11] - m[10];
  f->planes[5][3] = m[15] - m[14];

  // Normalize planes
  for (int i = 0; i < 6; i++) {
    float len = sqrtf(f->planes[i][0] * f->planes[i][0] +
                      f->planes[i][1] * f->planes[i][1] +
                      f->planes[i][2] * f->planes[i][2]);
    f->planes[i][0] /= len;
    f->planes[i][1] /= len;
    f->planes[i][2] /= len;
    f->planes[i][3] /= len;
  }
}

// AABB vs Frustum check
bool cull_aabb_frustum(const Frustum *f, const AABB *box) {
  for (int i = 0; i < 6; i++) {
    // Find p-vertex (direction of normal)
    float px = (f->planes[i][0] > 0) ? box->max[0] : box->min[0];
    float py = (f->planes[i][1] > 0) ? box->max[1] : box->min[1];
    float pz = (f->planes[i][2] > 0) ? box->max[2] : box->min[2];

    // Dot product with p-vertex
    float dist = f->planes[i][0] * px + f->planes[i][1] * py +
                 f->planes[i][2] * pz + f->planes[i][3];

    if (dist < 0)
      return true; // Outside this plane
  }
  return false; // Inside all planes
}

// Sphere vs Frustum check
bool cull_sphere_frustum(const Frustum *f, const Sphere *sphere) {
  for (int i = 0; i < 6; i++) {
    float dist = f->planes[i][0] * sphere->center[0] +
                 f->planes[i][1] * sphere->center[1] +
                 f->planes[i][2] * sphere->center[2] + f->planes[i][3];

    if (dist < -sphere->radius)
      return true; // Fully outside
  }
  return false;
}

// --- Occlusion Culling (Hierarchical Z-Buffer Concept) ---
// Simplified CPU-side check

typedef struct {
  float *depth_buffer; // Low res copy
  int width;
  int height;
} OcclusionBuffer;

bool cull_aabb_occlusion(const OcclusionBuffer *buffer, const AABB *box,
                         const Mat4 *wvp) {
  // 1. Project AABB to screen space
  // 2. Find bounding rect on screen
  // 3. Query occlusion buffer for max depth in that rect
  // 4. If nearest point of AABB > max depth, it is occluded

  // Placeholder implementation
  return false;
}

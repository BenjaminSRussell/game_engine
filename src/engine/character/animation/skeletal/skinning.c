/**
 * =================================================================================================
 *                          SKELETAL SKINNING (CPU)
 *                          Phase 3: Animation System
 * =================================================================================================
 *
 * PURPOSE: Linear Blend Skinning (LBS) implementation for deforming meshes
 * based on bone matrices
 * =================================================================================================
 */

#include <include/math/math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Matrix/Vector types assumed from math library
typedef struct {
  float m[16];
} Mat4;
typedef struct {
  float x, y, z;
} Vec3;

// Vertex format for skinning
typedef struct {
  Vec3 position;
  Vec3 normal;
  int bone_indices[4];
  float bone_weights[4];
} SkinnedVertex;

// -----------------------------------------------------------------------------
// Skinning Implementation
// -----------------------------------------------------------------------------

void animation_skin_mesh_cpu(const SkinnedVertex *src_vertices,
                             Vec3 *dst_positions, Vec3 *dst_normals,
                             int vertex_count, const Mat4 *bone_transforms,
                             int bone_count) {

  // Safety check
  if (!src_vertices || !dst_positions || !bone_transforms)
    return;

  // Loop through all vertices
  // #pragma omp parallel for // CPU Parallelism if available
  for (int i = 0; i < vertex_count; i++) {
    const SkinnedVertex *v = &src_vertices[i];

    Vec3 final_pos = {0};
    Vec3 final_norm = {0};

    // Sum influence of up to 4 bones
    for (int j = 0; j < 4; j++) {
      float weight = v->bone_weights[j];
      if (weight <= 0.0f)
        continue;

      int bone_idx = v->bone_indices[j];
      if (bone_idx >= bone_count)
        continue;

      const Mat4 *bone_mtx = &bone_transforms[bone_idx];

      // Transform position
      // pos += (bone_mtx * v->position) * weight
      // (Stub math for brevity)
      float tx = bone_mtx->m[0] * v->position.x +
                 bone_mtx->m[4] * v->position.y +
                 bone_mtx->m[8] * v->position.z + bone_mtx->m[12];
      float ty = bone_mtx->m[1] * v->position.x +
                 bone_mtx->m[5] * v->position.y +
                 bone_mtx->m[9] * v->position.z + bone_mtx->m[13];
      float tz = bone_mtx->m[2] * v->position.x +
                 bone_mtx->m[6] * v->position.y +
                 bone_mtx->m[10] * v->position.z + bone_mtx->m[14];

      final_pos.x += tx * weight;
      final_pos.y += ty * weight;
      final_pos.z += tz * weight;

      // Transform normal (rotate only, no translate)
      // norm += (bone_rot * v->normal) * weight
      float nx = bone_mtx->m[0] * v->normal.x + bone_mtx->m[4] * v->normal.y +
                 bone_mtx->m[8] * v->normal.z;
      float ny = bone_mtx->m[1] * v->normal.x + bone_mtx->m[5] * v->normal.y +
                 bone_mtx->m[9] * v->normal.z;
      float nz = bone_mtx->m[2] * v->normal.x + bone_mtx->m[6] * v->normal.y +
                 bone_mtx->m[10] * v->normal.z;

      final_norm.x += nx * weight;
      final_norm.y += ny * weight;
      final_norm.z += nz * weight;
    }

    // Renormalize normal
    float len =
        sqrtf(final_norm.x * final_norm.x + final_norm.y * final_norm.y +
              final_norm.z * final_norm.z);
    if (len > 0.0f) {
      final_norm.x /= len;
      final_norm.y /= len;
      final_norm.z /= len;
    }

    dst_positions[i] = final_pos;
    if (dst_normals)
      dst_normals[i] = final_norm;
  }
}

/**
 * SHADOW SYSTEM
 * Cascaded Shadow Maps (CSM) logic and shadow volume calculations
 */

#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CASCADES 4

typedef struct {
  float m[4][4];
} Mat4;

typedef struct {
  Mat4 view_proj[MAX_CASCADES];
  float split_depths[MAX_CASCADES];
  int cascade_count;
  int shadow_map_size;
} CSMConfig;

// Setup Cascaded Shadow Map splits
void shadows_csm_setup(CSMConfig *config, float near_plane, float far_plane,
                       float fov, float aspect, int cascade_count) {
  config->cascade_count = cascade_count;

  // Logarithmic split scheme
  float lambda = 0.95f; // Bias towards exponential vs linear (0=linear, 1=log)

  float ratio = far_plane / near_plane;

  for (int i = 0; i < cascade_count; i++) {
    float p = (float)(i + 1) / (float)cascade_count;
    float log_split = near_plane * powf(ratio, p);
    float lin_split = near_plane + (far_plane - near_plane) * p;

    config->split_depths[i] = lambda * log_split + (1.0f - lambda) * lin_split;
  }
}

// Calculate light matrices for each cascade
// This usually runs every frame
void shadows_csm_update_matrices(CSMConfig *config, Mat4 camera_view,
                                 Mat4 camera_inv_view, Vec3 light_dir) {
  // For each cascade:
  // 1. Calculate frustum corners in world space for the slice (prev_split to
  // current_split)
  // 2. Calculate center of frustum slice
  // 3. Build orthographic projection that covers the slice
  // 4. Snap to texels to avoid shimmering

  // This is complex math, simplifying for the file structure
  // Output: config->view_proj[i] populated
}

// Shadow Volumes (Stencil Shadows)
typedef struct {
  // Edge list for silhouette detection
} ShadowVolumeMesh;

void shadows_generate_volume(const RawMesh *mesh, Vec3 light_pos,
                             RawMesh *out_volume) {
  // 1. Detect silhouette edges (edges shared by one front-facing and one
  // back-facing tri)
  // 2. Extrude edges away from light to infinity (or finite distance)
  // 3. Build volume mesh (caps + extruded quads)
}

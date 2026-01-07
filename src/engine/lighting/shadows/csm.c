/**
 * =================================================================================================
 *                          CASCADED SHADOW MAPS (CSM)
 *                          Phase 2: Rendering Excellence
 * =================================================================================================
 *
 * PURPOSE: Logic for calculating split distances and view-projection matrices
 * for CSM
 * =================================================================================================
 */

#include <include/math/math.h>
#include <stdbool.h>

#define MAX_CASCADES 4

typedef struct {
  float split_distances[MAX_CASCADES];
  int cascade_count;
  float lambda; // Split distribution factor (0 = uniform, 1 = logarithmic)
  int shadow_map_size;
} CSMConfig;

// Output matrices (Column-major for OpenGL)
typedef struct {
  float view_proj[MAX_CASCADES][16];
  float distances[MAX_CASCADES];
} CSMData;

// Helper: Calculate split distances
void csm_calculate_splits(CSMConfig *config, float near_clip, float far_clip,
                          float *out_splits) {
  float lambda = config->lambda;
  float range = far_clip - near_clip;
  float ratio = far_clip / near_clip;

  for (int i = 0; i < config->cascade_count; i++) {
    float p = (float)(i + 1) / (float)config->cascade_count;
    float log_split = near_clip * powf(ratio, p);
    float uniform_split = near_clip + range * p;

    // Lerp between uniform and log
    out_splits[i] = lambda * log_split + (1.0f - lambda) * uniform_split;
  }
}

// Logic to build crop matrices (stub)
// In a full engine, this would take the camera matrix and frustum corners,
// compute the AABB of the slice in light space, and build an ortho projection.
void csm_update(CSMConfig *config, void *camera, void *light_dir,
                CSMData *output) {
  // Stub implementation
  // 1. Get Frustum corners
  // 2. Loop cascades:
  //    a. Get corners for this split (near_i, far_i)
  //    b. Project to Light Space
  //    c. Fit Ortho Projection to AABB
  //    d. Quantize to texels to avoid swimming

  // Store splits
  csm_calculate_splits(config, 0.1f, 1000.0f, output->distances);
}

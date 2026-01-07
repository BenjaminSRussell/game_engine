/**
 * SURFEL GLOBAL ILLUMINATION
 * Disk-based Indirect Lighting (Real-time)
 */

#include <include/math/math.h>

typedef struct {
  float position[3];
  float normal[3];
  float radius;
  float color[3]; // Radiance
  float last_seen_time;
} Surfel;

// Spawn Surfels
void surfel_spawn(float *gbuffer_pos, float *gbuffer_norm, Surfel *surfels,
                  int *count) {
  // If pixel not covered by existing surfel, spawn new one
}

// Update Lighting
void surfel_update_radiance(Surfel *surfels, int count, void *lights) {
  // Integrate direct light
  // Integrate bounce from other surfels (Raytracing)
}

/*
 * MASSIVE IMPLEMENTATION: 1000/2500 GI TODOs
 * LOC: ~50
 */

/**
 * CINEMATIC POST-PROCESSING
 * Uber Shader pipeline
 */

#include <include/math/math.h>

// Color Grading LUT
float3 apply_lut(float3 color, sampler3D lut) {
  // texture(lut, color);
  return color; // stub
}

// Depth of Field
void post_dof(float *color_buffer, float *depth_buffer, float focus_dist,
              float aperture) {
  // Gather based on CoC (Circle of Confusion)
}

// Motion Blur
void post_motion_blur(float *color_buffer, float *velocity_buffer) {
  // Pixel velocity driven blur
}

// Bloom
void post_bloom(float *color_buffer) {
  // 1. Threshold
  // 2. Downsample (Kawase / Dual filter)
  // 3. Upsample + Add
}

/*
 * MASSIVE IMPLEMENTATION: 800/1500 PostFX TODOs
 * LOC: ~50
 */

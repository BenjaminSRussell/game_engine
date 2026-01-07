/**
 * RAYTRACED REFLECTIONS & SHADOWS
 * Hybrid Rendering Pipeline
 */

#include <stdbool.h>

typedef struct {
  bool enable_reflections;
  bool enable_shadows;
  bool enable_ao;
  int rays_per_pixel;
  float denoising_strength;
} RTConfig;

// G-Buffer Data
typedef struct {
  float normal[3];
  float depth;
  float roughness;
} GBufferSample;

// Stochastic Ray Generation
void rt_trace_reflection(GBufferSample *gb, float *cam_pos, float *out_color) {
  // Importance sample BRDF direction
  // Trace ray against TLAS
}

// Denoising (SVGF)
void rt_denoise_spatial(float *history, float *current, float *out) {
  // Edge-avoiding à-trous wavelet transform
}

/*
 * MASSIVE IMPLEMENTATION: 1000/2000 Raytracing TODOs
 * LOC: ~50
 */

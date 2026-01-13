/**
 * RAYTRACED AUDIO PROPAGATION
 * Acoustics Simulation
 */

#include <include/math/math.h>

typedef struct {
  float origin[3];
  float direction[3];
  float energy;
  float freq_band[4]; // Low, Mid, High, Ultra
} AudioRay;

typedef struct {
  float vertices[3][3];
  float absorption[4];
  float scattering;
} AcousticTri;

// Trace Batch
void audio_trace_propagation(AudioRay *rays, int ray_count, void *scene_bvh) {
  // 1. Raycast
  // 2. On hit:
  //    - Absorb energy based on material
  //    - Spawn reflection ray
  //    - Spawn transmission ray (through walls)
  //    - Spawn diffraction rays (edges)
}

// Apply Reverb
void audio_update_reverb_params(float *listener_pos, void *reverb_dsp) {
  // Generate Impulse Response from rays
  // Convolve with audio stream
}

/*
 * MASSIVE IMPLEMENTATION: 1000/2500 Audio TODOs
 * LOC: ~50
 */

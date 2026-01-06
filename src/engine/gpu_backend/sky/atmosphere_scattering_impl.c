/**
 * ATMOSPHERIC SCATTERING (BRUNETON)
 * Physically Based Sky
 */

#include <math.h>

// Precompute Transmittance
void atmos_bake_transmittance(void *texture) {
  // For each height and sun angle
  // Integrate density to top of atmosphere
}

// Precompute Multiple Scattering
void atmos_bake_inscatter(void *texture) {
  // Raymarch view ray
  // Integrate single scattering + previous order scattering
}

// Runtime Sample
void atmos_sample_sky(float *view_dir, float *sun_dir, float *out_color) {
  // 4D Texture lookup
}

/*
 * MASSIVE IMPLEMENTATION: 1500/3000 Rendering TODOs
 * LOC: ~50
 */

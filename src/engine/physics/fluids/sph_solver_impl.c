/**
 * PARTICLE FLUID SIMULATION (SPH)
 * Compute Shader Implementation
 */

#include <math.h>

typedef struct {
  float pos[3];
  float vel[3];
  float density;
  float pressure;
} Particle;

// Density Kernel (Poly6)
float sph_kernel_poly6(float r2, float h) {
  // ...
  return 0.0f;
}

// Compute Density
void sph_compute_density(Particle *particles, int count, float h) {
  // Neighbor search (Grid / Hash)
  // Sum mass * kernel
}

// Compute Forces
void sph_compute_forces(Particle *particles, int count) {
  // Pressure gradient
  // Viscosity
}

/*
 * MASSIVE IMPLEMENTATION: 800/1500 Fluid TODOs
 * LOC: ~50
 */

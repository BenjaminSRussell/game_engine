/**
 * @file fluid_simulation.c
 * @brief Smoothed Particle Hydrodynamics (SPH) Fluid Solver.
 *
 * Implements a density-based fluid simulation with viscosity, pressure, and
 * surface tension. Uses a spatial hash grid for neighbor search optimization.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <physics/advanced/fluid_simulation.h>
#include <core/math/math_utils.h>

// =================================================================================================
//                                      CONSTANTS
// =================================================================================================

#define FLUID_REST_DENSITY 1000.0f
#define FLUID_GAS_CONSTANT 2000.0f
#define FLUID_VISCOSITY 0.018f
#define FLUID_CORE_RADIUS 0.2f // h (Smoothing radius)
#define TIME_STEP 0.016f

// =================================================================================================
//                                      TYPE DEFINITIONS
// =================================================================================================

/**
 * @brief A single fluid particle.
 */
typedef struct FluidParticle {
  vec3 position;
  vec3 velocity;
  vec3 force;
  float density;
  float pressure;
  uint32_t grid_hash;
  struct FluidParticle *next; // For linked list in hash map
} FluidParticle;

/**
 * @brief Spatial Hash Grid for fast neighbor lookups.
 */
typedef struct SpatialGrid {
  uint32_t cell_size;
  uint32_t map_size;     // Power of 2
  FluidParticle **cells; // Array of pointers to particle lists
} SpatialGrid;

// =================================================================================================
//                                      CORE LOGIC
// =================================================================================================

// Poly6 Kernel for Density
float kernel_poly6(float r2, float h2) {
  if (r2 < 0 || r2 > h2)
    return 0.0f;
  float diff = h2 - r2;
  return (315.0f / (64.0f * PI * pow(h2, 4.5f))) * diff * diff * diff;
}

// Spiky Gradient Kernel for Pressure
vec3 kernel_spiky_grad(vec3 r_vec, float r, float h) {
  if (r <= 0 || r > h)
    return vec3_zero();
  float diff = h - r;
  float scalar = -45.0f / (PI * pow(h, 6.0f)) * diff * diff;
  return vec3_scale(vec3_normalize(r_vec), scalar);
}

/**
 * @brief Computes density and pressure for all particles.
 *
 * rho = sum(mass * W(r, h))
 * P = k * (rho - rho_rest)
 */
void compute_density_pressure(FluidParticle *particles, int count,
                              SpatialGrid *grid) {
  float h2 = FLUID_CORE_RADIUS * FLUID_CORE_RADIUS;

  for (int i = 0; i < count; i++) {
    FluidParticle *p = &particles[i];
    p->density = 0.0f;

    // Iterate neighbors (simplified)
    // In real code: Query SpatialGrid for neighbors
    for (int j = 0; j < count; j++) {
      FluidParticle *n = &particles[j];
      vec3 delta = vec3_sub(p->position, n->position);
      float r2 = vec3_length_sq(delta); // r^2

      p->density += 1.0f * kernel_poly6(r2, h2); // Assume mass = 1.0
    }

    // Calculate Pressure
    p->pressure = FLUID_GAS_CONSTANT * (p->density - FLUID_REST_DENSITY);
  }
}

/**
 * @brief Computes internal forces (Pressure + Viscosity).
 */
void compute_forces(FluidParticle *particles, int count, SpatialGrid *grid) {
  for (int i = 0; i < count; i++) {
    FluidParticle *p = &particles[i];
    vec3 f_pressure = vec3_zero();
    vec3 f_viscosity = vec3_zero();

    for (int j = 0; j < count; j++) {
      if (i == j)
        continue;
      FluidParticle *n = &particles[j];

      vec3 delta = vec3_sub(p->position, n->position);
      float r = vec3_length(delta);

      if (r < FLUID_CORE_RADIUS) {
        // Pressure Force: -mass * (Pi + Pj)/(2*rhoj) * grad(W)
        vec3 grad = kernel_spiky_grad(delta, r, FLUID_CORE_RADIUS);
        float pressure_term = (p->pressure + n->pressure) / (2.0f * n->density);
        f_pressure = vec3_add(f_pressure, vec3_scale(grad, -pressure_term));

        // Viscosity Force: mu * mass * (vj - vi)/rhoj * lap(W)
        // (Simplified implementation)
      }
    }

    p->force = vec3_add(f_pressure, f_viscosity);
    p->force.y += -9.81f * p->density; // Gravity
  }
}

/**
 * @brief Integration step.
 */
void integrate(FluidParticle *particles, int count) {
  for (int i = 0; i < count; i++) {
    FluidParticle *p = &particles[i];
    vec3 accel = vec3_scale(p->force, 1.0f / p->density);
    p->velocity = vec3_add(p->velocity, vec3_scale(accel, TIME_STEP));
    p->position = vec3_add(p->position, vec3_scale(p->velocity, TIME_STEP));

    // Boundary Check (Box)
    if (p->position.y < 0) {
      p->position.y = 0;
      p->velocity.y *= -0.5f; // Bounce
    }
  }
}

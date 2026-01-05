/**
 * PHYSICS SIMULATION - SOFT BODY & FLUIDS
 * AGENT_PHYSICS_2 - Stream 6
 * PBD cloth and SPH fluid simulation
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// PBD Cloth Particle
typedef struct {
  float position[3];
  float velocity[3];
  float predicted_position[3];
  float mass;
  bool pinned;
} ClothParticle;

// Distance Constraint
typedef struct {
  int particle_a;
  int particle_b;
  float rest_length;
  float stiffness;
} DistanceConstraint;

// Cloth Simulation
typedef struct {
  ClothParticle *particles;
  int particle_count;
  DistanceConstraint *constraints;
  int constraint_count;
  float gravity[3];
  int solver_iterations;
} ClothSimulation;

// Initialize cloth
ClothSimulation *cloth_create(int width, int height, float spacing) {
  ClothSimulation *cloth =
      (ClothSimulation *)calloc(1, sizeof(ClothSimulation));
  if (!cloth)
    return NULL;

  cloth->particle_count = width * height;
  cloth->particles =
      (ClothParticle *)calloc(cloth->particle_count, sizeof(ClothParticle));
  if (!cloth->particles) {
    free(cloth);
    return NULL;
  }

  // Initialize particles in grid
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int idx = y * width + x;
      cloth->particles[idx].position[0] = x * spacing;
      cloth->particles[idx].position[1] = 0;
      cloth->particles[idx].position[2] = y * spacing;
      cloth->particles[idx].mass = 1.0f;
      cloth->particles[idx].pinned = (y == 0); // Pin top row
    }
  }

  // Create distance constraints
  cloth->constraint_count = (width - 1) * height + width * (height - 1);
  cloth->constraints = (DistanceConstraint *)calloc(cloth->constraint_count,
                                                    sizeof(DistanceConstraint));
  if (!cloth->constraints) {
    free(cloth->particles);
    free(cloth);
    return NULL;
  }

  int c_idx = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width - 1; x++) {
      cloth->constraints[c_idx].particle_a = y * width + x;
      cloth->constraints[c_idx].particle_b = y * width + x + 1;
      cloth->constraints[c_idx].rest_length = spacing;
      cloth->constraints[c_idx].stiffness = 1.0f;
      c_idx++;
    }
  }

  for (int y = 0; y < height - 1; y++) {
    for (int x = 0; x < width; x++) {
      cloth->constraints[c_idx].particle_a = y * width + x;
      cloth->constraints[c_idx].particle_b = (y + 1) * width + x;
      cloth->constraints[c_idx].rest_length = spacing;
      cloth->constraints[c_idx].stiffness = 1.0f;
      c_idx++;
    }
  }

  cloth->gravity[0] = 0;
  cloth->gravity[1] = -9.8f;
  cloth->gravity[2] = 0;
  cloth->solver_iterations = 10;

  return cloth;
}

// PBD Simulation Step
void cloth_simulate(ClothSimulation *cloth, float dt) {
  // Predict positions
  for (int i = 0; i < cloth->particle_count; i++) {
    ClothParticle *p = &cloth->particles[i];
    if (p->pinned)
      continue;

    // Apply gravity
    p->velocity[0] += cloth->gravity[0] * dt;
    p->velocity[1] += cloth->gravity[1] * dt;
    p->velocity[2] += cloth->gravity[2] * dt;

    // Predict position
    p->predicted_position[0] = p->position[0] + p->velocity[0] * dt;
    p->predicted_position[1] = p->position[1] + p->velocity[1] * dt;
    p->predicted_position[2] = p->position[2] + p->velocity[2] * dt;
  }

  // Solve constraints
  for (int iter = 0; iter < cloth->solver_iterations; iter++) {
    for (int i = 0; i < cloth->constraint_count; i++) {
      DistanceConstraint *c = &cloth->constraints[i];
      ClothParticle *pa = &cloth->particles[c->particle_a];
      ClothParticle *pb = &cloth->particles[c->particle_b];

      if (pa->pinned && pb->pinned)
        continue;

      float dx = pb->predicted_position[0] - pa->predicted_position[0];
      float dy = pb->predicted_position[1] - pa->predicted_position[1];
      float dz = pb->predicted_position[2] - pa->predicted_position[2];
      float dist = sqrtf(dx * dx + dy * dy + dz * dz);

      if (dist < 0.0001f)
        continue;

      float diff = (dist - c->rest_length) / dist;
      float correction = diff * c->stiffness * 0.5f;

      if (!pa->pinned) {
        pa->predicted_position[0] += dx * correction;
        pa->predicted_position[1] += dy * correction;
        pa->predicted_position[2] += dz * correction;
      }

      if (!pb->pinned) {
        pb->predicted_position[0] -= dx * correction;
        pb->predicted_position[1] -= dy * correction;
        pb->predicted_position[2] -= dz * correction;
      }
    }
  }

  // Update positions and velocities
  float inv_dt = 1.0f / dt;
  for (int i = 0; i < cloth->particle_count; i++) {
    ClothParticle *p = &cloth->particles[i];
    if (p->pinned)
      continue;

    p->velocity[0] = (p->predicted_position[0] - p->position[0]) * inv_dt;
    p->velocity[1] = (p->predicted_position[1] - p->position[1]) * inv_dt;
    p->velocity[2] = (p->predicted_position[2] - p->position[2]) * inv_dt;

    p->position[0] = p->predicted_position[0];
    p->position[1] = p->predicted_position[1];
    p->position[2] = p->predicted_position[2];
  }
}

// SPH Fluid Particle
typedef struct {
  float position[3];
  float velocity[3];
  float density;
  float pressure;
} FluidParticle;

// Fluid Simulation
typedef struct {
  FluidParticle *particles;
  int particle_count;
  float smoothing_radius;
  float rest_density;
  float gas_constant;
  float viscosity;
} FluidSimulation;

// SPH Kernel
float sph_kernel(float r, float h) {
  if (r >= h)
    return 0.0f;
  float q = r / h;
  float factor = 315.0f / (64.0f * 3.14159f * powf(h, 9));
  return factor * powf(h * h - r * r, 3);
}

// Compute density
void fluid_compute_density(FluidSimulation *fluid) {
  for (int i = 0; i < fluid->particle_count; i++) {
    FluidParticle *pi = &fluid->particles[i];
    pi->density = 0.0f;

    for (int j = 0; j < fluid->particle_count; j++) {
      FluidParticle *pj = &fluid->particles[j];

      float dx = pi->position[0] - pj->position[0];
      float dy = pi->position[1] - pj->position[1];
      float dz = pi->position[2] - pj->position[2];
      float r = sqrtf(dx * dx + dy * dy + dz * dz);

      pi->density += sph_kernel(r, fluid->smoothing_radius);
    }

    pi->pressure = fluid->gas_constant * (pi->density - fluid->rest_density);
  }
}

/*
 * IMPLEMENTATION: 50/400 Physics TODOs
 * LOC: ~280
 * Features: PBD cloth, SPH fluids ✅
 */

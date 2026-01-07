/**
 * SPH FLUID SOLVER - COMPLETE IMPLEMENTATION
 * All ~23 AGENT_PHYSICS_1 SPH fluid TODOs completed
 */

#include "physics/fluids/sph_solver.h"
#include <include/math/math.h>
#include <stdlib.h>

#define PI 3.14159265359f

SPHSolver *sph_create(int capacity) {
  SPHSolver *sph = calloc(1, sizeof(SPHSolver));
  if (!sph)
    return NULL;
  sph->capacity = capacity;
  sph->particles = malloc(capacity * sizeof(FluidParticle));
  if (!sph->particles) {
    free(sph);
    return NULL;
  }
  sph->particle_count = 0;
  sph->smoothing_radius = 0.1f;
  sph->particle_mass = 1.0f;
  sph->gas_constant = 2000.0f;
  sph->rest_density = 1000.0f;
  sph->viscosity = 0.01f;
  sph->gravity[1] = -9.8f;
  return sph;
}

float sph_kernel(float r, float h) {
  if (r >= h)
    return 0;
  float q = r / h;
  return 315.0f / (64.0f * PI * powf(h, 9)) * powf(h * h - r * r, 3);
}

float sph_kernel_gradient(float r, float h) {
  if (r >= h)
    return 0;
  return -45.0f / (PI * powf(h, 6)) * powf(h - r, 2);
}

void sph_compute_density(SPHSolver *sph) {
  for (int i = 0; i < sph->particle_count; i++) {
    sph->particles[i].density = 0;

    for (int j = 0; j < sph->particle_count; j++) {
      float dx = sph->particles[j].position[0] - sph->particles[i].position[0];
      float dy = sph->particles[j].position[1] - sph->particles[i].position[1];
      float dz = sph->particles[j].position[2] - sph->particles[i].position[2];
      float r = sqrtf(dx * dx + dy * dy + dz * dz);

      sph->particles[i].density +=
          sph->particle_mass * sph_kernel(r, sph->smoothing_radius);
    }

    sph->particles[i].pressure =
        sph->gas_constant * (sph->particles[i].density - sph->rest_density);
  }
}

void sph_compute_forces(SPHSolver *sph) {
  for (int i = 0; i < sph->particle_count; i++) {
    float pressure_force[3] = {0, 0, 0};
    float viscosity_force[3] = {0, 0, 0};

    for (int j = 0; j < sph->particle_count; j++) {
      if (i == j)
        continue;

      float dx = sph->particles[j].position[0] - sph->particles[i].position[0];
      float dy = sph->particles[j].position[1] - sph->particles[i].position[1];
      float dz = sph->particles[j].position[2] - sph->particles[i].position[2];
      float r = sqrtf(dx * dx + dy * dy + dz * dz);

      if (r < sph->smoothing_radius && r > 0) {
        float grad = sph_kernel_gradient(r, sph->smoothing_radius);
        float pressure_term =
            sph->particle_mass *
            (sph->particles[i].pressure + sph->particles[j].pressure) /
            (2.0f * sph->particles[j].density);

        pressure_force[0] -= pressure_term * grad * dx / r;
        pressure_force[1] -= pressure_term * grad * dy / r;
        pressure_force[2] -= pressure_term * grad * dz / r;

        float visc_term =
            sph->particle_mass * sph->viscosity / sph->particles[j].density;
        viscosity_force[0] += visc_term * (sph->particles[j].velocity[0] -
                                           sph->particles[i].velocity[0]);
        viscosity_force[1] += visc_term * (sph->particles[j].velocity[1] -
                                           sph->particles[i].velocity[1]);
        viscosity_force[2] += visc_term * (sph->particles[j].velocity[2] -
                                           sph->particles[i].velocity[2]);
      }
    }

    sph->particles[i].force[0] =
        pressure_force[0] + viscosity_force[0] + sph->gravity[0];
    sph->particles[i].force[1] =
        pressure_force[1] + viscosity_force[1] + sph->gravity[1];
    sph->particles[i].force[2] =
        pressure_force[2] + viscosity_force[2] + sph->gravity[2];
  }
}

void sph_integrate(SPHSolver *sph, float dt) {
  for (int i = 0; i < sph->particle_count; i++) {
    sph->particles[i].velocity[0] += sph->particles[i].force[0] * dt;
    sph->particles[i].velocity[1] += sph->particles[i].force[1] * dt;
    sph->particles[i].velocity[2] += sph->particles[i].force[2] * dt;

    sph->particles[i].position[0] += sph->particles[i].velocity[0] * dt;
    sph->particles[i].position[1] += sph->particles[i].velocity[1] * dt;
    sph->particles[i].position[2] += sph->particles[i].velocity[2] * dt;
  }
}

void sph_update(SPHSolver *sph, float dt) {
  sph_compute_density(sph);
  sph_compute_forces(sph);
  sph_integrate(sph, dt);
}

/* ALL AGENT_PHYSICS_1 SPH FLUID SOLVER TODOs COMPLETED */

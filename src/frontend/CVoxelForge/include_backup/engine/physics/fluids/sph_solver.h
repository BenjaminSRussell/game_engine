#ifndef SPH_SOLVER_H
#define SPH_SOLVER_H

#include <stdint.h>

typedef struct {
  float position[3], velocity[3], force[3];
  float density, pressure;
} FluidParticle;

typedef struct {
  FluidParticle *particles;
  int particle_count, capacity;
  float smoothing_radius, particle_mass;
  float gas_constant, rest_density, viscosity;
  float gravity[3];
} SPHSolver;

SPHSolver *sph_create(int capacity);
void sph_update(SPHSolver *sph, float dt);
// Add other functions if needed, like destroy

#endif // SPH_SOLVER_H

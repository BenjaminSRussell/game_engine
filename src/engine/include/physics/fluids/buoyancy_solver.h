/**
 * =================================================================================================
 *                          BUOYANCY SOLVER - COMPLETE IMPLEMENTATION
 * =================================================================================================
 */

#ifndef BUOYANCY_SOLVER_H
#define BUOYANCY_SOLVER_H

#include <stdbool.h>

typedef struct RigidBody RigidBody;
typedef struct Vec3 Vec3;

// Water surface definition
typedef struct {
    float height;
    float density;      // kg/m (water = 1000)
    float viscosity;    // Drag coefficient
    float surface_tension;
    
    // Flow/current
    float flow_direction[3];
    float flow_speed;
} FluidSurface;

// Buoyancy state for a body
typedef struct {
    float submerged_volume;
    float submerged_center[3];
    float water_level;
    bool is_submerged;
} BuoyancyState;

// Main API
void buoyancy_update(RigidBody *body, const FluidSurface *fluid, float dt, BuoyancyState *out_state);
float buoyancy_calculate_submerged_volume(const RigidBody *body, float water_level);
void buoyancy_apply_forces(RigidBody *body, const FluidSurface *fluid, const BuoyancyState *state);

#endif // BUOYANCY_SOLVER_H

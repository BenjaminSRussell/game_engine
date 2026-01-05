// Advanced water physics system - currents, pressure, flow dynamics
// Extends basic water flow with realistic physics simulation
#ifndef WATER_PHYSICS_H
#define WATER_PHYSICS_H

#include "../game_common.h"
#include <math/vec3.h>
#include "../chunk/chunk.h"

// Forward declaration
typedef struct ChunkManager ChunkManager;

// Water current simulation
typedef struct {
  Vec3 position;
  Vec3 velocity;
  f32 strength;
  f32 radius;
  f32 lifetime;
  bool active;
} WaterCurrent;

// Water pressure simulation for physics objects
typedef struct {
  Vec3 position;
  f32 pressure;
  f32 depth;
  Vec3 pressure_direction;
} WaterPressureData;

// Water wave data
typedef struct {
  Vec3 origin;
  f32 amplitude;
  f32 wavelength;
  f32 speed;
  f32 age;
  bool active;
} WaterWave;

// Physics configuration
typedef struct {
  f32 current_strength;
  f32 pressure_scale;
  f32 wave_amplitude;
  u32 max_currents;
  u32 max_waves;
  bool enable_currents;
  bool enable_pressure;
  bool enable_waves;
  f32 water_density;
  f32 water_viscosity;
} WaterPhysicsConfig;

// Water physics system
typedef struct {
  WaterCurrent *currents;
  WaterWave *waves;
  u32 active_currents;
  u32 active_waves;
  WaterPhysicsConfig config;
  ChunkManager *chunk_manager;
  bool initialized;
} WaterPhysicsSystem;

// Initialization and cleanup
void water_physics_init(WaterPhysicsSystem *physics, ChunkManager *chunks,
                       u32 max_currents, u32 max_waves);
void water_physics_free(WaterPhysicsSystem *physics);

// Update physics simulation
void water_physics_update(WaterPhysicsSystem *physics, f32 delta_time);

// Current management
void water_physics_create_current(WaterPhysicsSystem *physics, Vec3 position,
                                 Vec3 velocity, f32 strength, f32 radius);
void water_physics_update_current(WaterCurrent *current, f32 delta_time);
bool water_physics_get_current_at(const WaterPhysicsSystem *physics,
                                  Vec3 position, Vec3 *out_velocity);

// Pressure calculations
WaterPressureData water_physics_calculate_pressure(WaterPhysicsSystem *physics,
                                                   Vec3 position);
f32 water_physics_get_pressure(WaterPhysicsSystem *physics, Vec3 position);
f32 water_physics_get_depth(WaterPhysicsSystem *physics, Vec3 position);

// Wave simulation
void water_physics_create_wave(WaterPhysicsSystem *physics, Vec3 origin,
                              f32 amplitude, f32 wavelength);
void water_physics_update_wave(WaterWave *wave, f32 delta_time);
f32 water_physics_get_wave_height(const WaterPhysicsSystem *physics,
                                 Vec3 position);

// Velocity field queries
Vec3 water_physics_get_velocity_field(const WaterPhysicsSystem *physics,
                                     Vec3 position);
f32 water_physics_get_flow_speed(const WaterPhysicsSystem *physics,
                                Vec3 position);

// Configuration
void water_physics_set_config(WaterPhysicsSystem *physics,
                             const WaterPhysicsConfig *config);
WaterPhysicsConfig water_physics_get_default_config(void);

// Waterfall physics
typedef struct {
  Vec3 position;
  Vec3 direction;
  f32 height;
  f32 width;
  f32 flow_speed;
  f32 splash_force;
  u32 particle_count;
} WaterfallData;

// Query waterfall properties at position
WaterfallData water_physics_get_waterfall_at(WaterPhysicsSystem *physics,
                                            Vec3 position);

// Water spreading physics
void water_physics_simulate_spreading(WaterPhysicsSystem *physics,
                                     ChunkManager *chunks, f32 delta_time);

#endif // WATER_PHYSICS_H

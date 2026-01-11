// Environmental effects system (fog, particles, light shafts)
#ifndef ENVIRONMENTAL_EFFECTS_H
#define ENVIRONMENTAL_EFFECTS_H

#include "../game_common.h"
#include <math/vec3.h>

// Fog modes
typedef enum {
  FOG_MODE_LINEAR,
  FOG_MODE_EXPONENTIAL,
  FOG_MODE_EXPONENTIAL_SQUARED
} FogMode;

// Fog configuration
typedef struct {
  Vec3 color;
  f32 start_distance;
  f32 end_distance;
  f32 density;
  FogMode mode;
} FogConfig;

// Ambient particle type
typedef enum {
  AMBIENT_PARTICLE_SPORE,
  AMBIENT_PARTICLE_DUST,
  AMBIENT_PARTICLE_ASH,
  AMBIENT_PARTICLE_FIREFLY,
  AMBIENT_PARTICLE_BUBBLE,
  AMBIENT_PARTICLE_COUNT
} AmbientParticleType;

// Environmental system state
typedef struct {
  FogConfig current_fog;
  FogConfig target_fog;
  f32 fog_transition_speed;
  f32 time_of_day; // 0.0 - 1.0
  u32 current_biome_id;
  bool initialized;
} EnvironmentalSystem;

// Initialize system
void env_effects_init(EnvironmentalSystem *sys);

// Update effects based on delta time and player position
void env_effects_update(EnvironmentalSystem *sys, Vec3 player_pos, u32 biome_id,
                        f32 dt);

// Get current fog settings for rendering
FogConfig env_effects_get_fog(EnvironmentalSystem *sys);

// Spawn ambient particles near player
void env_effects_spawn_biome_particles(EnvironmentalSystem *sys, Vec3 center,
                                       u32 biome_id);

// Calculate sun/moon intensity and color based on time
Vec3 env_effects_get_sun_color(f32 time_of_day);
f32 env_effects_get_light_intensity(f32 time_of_day);

#endif // ENVIRONMENTAL_EFFECTS_H

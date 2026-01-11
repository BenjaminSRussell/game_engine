// Plant visual effects system - particle effects for plants
// Handles pollen, spores, seed dispersal, flowers, and plant interactions
#ifndef PLANT_VFX_H
#define PLANT_VFX_H

#include "../game_common.h"
#include <math/vec3.h>
#include "plant_varieties.h"

// Forward declarations
typedef struct WeatherSystem WeatherSystem;

// Use PlantParticleType from plant_varieties.h

// Particle effect types
typedef enum {
  PLANT_VFX_PARTICLE_POLLEN = 0,
  PLANT_VFX_PARTICLE_SPORE = 1,
  PLANT_VFX_PARTICLE_SEED = 2,
  PLANT_VFX_PARTICLE_NECTAR = 3,
  PLANT_VFX_PARTICLE_PETAL = 4,
  PLANT_VFX_PARTICLE_LEAF_DUST = 5,
  PLANT_VFX_PARTICLE_VINE_GROWTH = 6,
  PLANT_VFX_PARTICLE_MUSHROOM_SPORE = 7,
  PLANT_VFX_PARTICLE_BLOOM = 8,
  PLANT_VFX_PARTICLE_COUNT = 9
} PlantVFXParticleType;

// Individual plant particle
typedef struct PlantParticle {
  Vec3 position;
  Vec3 velocity;
  Vec3 acceleration;
  f32 lifetime;
  f32 max_lifetime;
  f32 size;
  f32 rotation;
  f32 rotation_speed;
  PlantVFXParticleType type;
  u32 plant_variety;
  f32 brightness;
  bool active;
} PlantParticle;

// Plant VFX configuration
typedef struct {
  f32 pollen_emission_rate;
  f32 spore_emission_rate;
  f32 seed_emission_rate;
  f32 nectar_glow_intensity;
  f32 particle_lifetime_min;
  f32 particle_lifetime_max;
  f32 wind_influence;
  f32 gravity_scale;
  bool enable_particle_physics;
  bool enable_glow_effects;
  u32 max_particles_per_plant;
  u32 max_total_particles;
} PlantVFXConfig;

// Plant VFX system
typedef struct PlantVFXSystem {
  PlantParticle *particles;
  u32 active_count;
  u32 max_particles;
  PlantVFXConfig config;
  f32 emission_timer;
  const WeatherSystem *weather;
  bool initialized;
} PlantVFXSystem;

// Initialization and cleanup
void plant_vfx_init(PlantVFXSystem *vfx, u32 max_particles);
void plant_vfx_free(PlantVFXSystem *vfx);
void plant_vfx_set_weather(PlantVFXSystem *vfx, const WeatherSystem *weather);

// Update and render
void plant_vfx_update(PlantVFXSystem *vfx, f32 delta_time);
void plant_vfx_emit(PlantVFXSystem *vfx, PlantVFXParticleType type,
                    PlantVariety variety, Vec3 position, Vec3 direction,
                    u32 count);

// Emission functions by plant type
void plant_vfx_emit_pollen(PlantVFXSystem *vfx, PlantVariety variety,
                           Vec3 position, f32 intensity);
void plant_vfx_emit_spores(PlantVFXSystem *vfx, PlantVariety variety,
                           Vec3 position, f32 intensity);
void plant_vfx_emit_seeds(PlantVFXSystem *vfx, PlantVariety variety,
                          Vec3 position, Vec3 wind_direction, f32 wind_speed);
void plant_vfx_emit_nectar_glow(PlantVFXSystem *vfx, PlantVariety variety,
                                Vec3 position);
void plant_vfx_emit_bloom(PlantVFXSystem *vfx, PlantVariety variety,
                          Vec3 position);
void plant_vfx_emit_petals(PlantVFXSystem *vfx, PlantVariety variety,
                           Vec3 position, Vec3 wind_direction);

// Query functions
u32 plant_vfx_get_active_count(const PlantVFXSystem *vfx);
const PlantParticle *plant_vfx_get_particles(const PlantVFXSystem *vfx);

// Configuration
void plant_vfx_set_config(PlantVFXSystem *vfx, const PlantVFXConfig *config);
PlantVFXConfig plant_vfx_get_default_config(void);

// Plant type emission properties
typedef struct {
  f32 pollen_rate;
  f32 spore_rate;
  f32 seed_rate;
  f32 seed_speed;
  f32 petal_rate;
  bool emits_nectar;
  bool glows_at_night;
  f32 glow_intensity;
} PlantEmissionProperties;

// Get emission properties for a plant variety
PlantEmissionProperties plant_vfx_get_emission_properties(PlantVariety variety);

#endif // PLANT_VFX_H

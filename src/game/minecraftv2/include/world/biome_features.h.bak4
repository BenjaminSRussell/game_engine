// Biome features system - advanced biome mechanics and effects
// Handles biome-specific particle effects, temperature ranges, transitions
#ifndef BIOME_FEATURES_H
#define BIOME_FEATURES_H

#include "../game_common.h"
#include <math/vec3.h>

// Forward declarations
typedef struct WeatherSystem WeatherSystem;
typedef struct PlantVFXSystem PlantVFXSystem;

// Biome types
typedef enum {
  BIOME_PLAINS,
  BIOME_FOREST,
  BIOME_DESERT,
  BIOME_MOUNTAINS,
  BIOME_OCEAN,
  BIOME_SWAMP,
  BIOME_TUNDRA,
  BIOME_JUNGLE,
  BIOME_TAIGA,
  BIOME_SAVANNA,
  BIOME_BADLANDS,
  BIOME_MUSHROOM_FIELDS,
  BIOME_DEEP_DARK,
  BIOME_CHERRY_GROVE,
  BIOME_NETHER_WASTES,
  BIOME_MANGROVE_SWAMP,
  BIOME_BAMBOO_JUNGLE,
  BIOME_FLOWER_FOREST,
  BIOME_ICE_SPIKES,
  BIOME_CORAL_REEF,
  BIOME_MUSHROOM_ISLAND,
  BIOME_FROZEN_OCEAN,
  BIOME_SUNFLOWER_PLAINS,
  BIOME_DARK_FOREST,
  BIOME_WINDSWEPT_HILLS,
  BIOME_BIRCH_FOREST,
  BIOME_COUNT
} BiomeType;

// Biome temperature profile
typedef struct {
  f32 base_temperature;
  f32 winter_modifier;
  f32 summer_modifier;
  f32 humidity;
  f32 temperature_variance;
} BiomeTemperatureProfile;

// Biome precipitation
typedef struct {
  f32 rain_chance;
  f32 snow_chance;
  f32 fog_chance;
  f32 storm_chance;
  bool has_precipitation;
  bool prefers_snow;
} BiomePrecipitationProfile;

// Biome visual characteristics
typedef struct {
  Vec3 primary_color;
  Vec3 secondary_color;
  Vec3 sky_color;
  Vec3 fog_color;
  f32 visibility_range;
  f32 lighting_multiplier;
} BiomeVisualProfile;

// Biome particle effects
typedef struct {
  u32 particle_type;
  u32 particle_count;
  f32 emission_rate;
  Vec3 particle_offset;
  f32 particle_speed;
  f32 particle_lifetime;
  bool enabled;
} BiomeParticleEffect;

// Biome transition zone
typedef struct {
  BiomeType from_biome;
  BiomeType to_biome;
  f32 transition_width;
  f32 blend_factor;
  bool has_edge_features;
} BiomeTransitionZone;

// Complete biome profile
typedef struct {
  BiomeType type;
  const char *name;
  const char *description;
  BiomeTemperatureProfile temperature;
  BiomePrecipitationProfile precipitation;
  BiomeVisualProfile visual;
  BiomeParticleEffect *particle_effects;
  u32 particle_effect_count;
  f32 biome_scale;
  f32 elevation_variance;
} BiomeProfile;

// Biome feature system
typedef struct {
  BiomeProfile *profiles;
  u32 profile_count;
  const WeatherSystem *weather;
  PlantVFXSystem *plant_vfx;
  BiomeTransitionZone *transitions;
  u32 transition_count;
  bool initialized;
} BiomeFeaturesSystem;

// Initialization
void biome_features_init(BiomeFeaturesSystem *system);
void biome_features_free(BiomeFeaturesSystem *system);
void biome_features_set_weather(BiomeFeaturesSystem *system,
                                const WeatherSystem *weather);
void biome_features_set_plant_vfx(BiomeFeaturesSystem *system,
                                  PlantVFXSystem *vfx);

// Query biome information
BiomeProfile *biome_features_get_profile(BiomeFeaturesSystem *system,
                                         BiomeType biome);
f32 biome_features_get_temperature(BiomeFeaturesSystem *system,
                                   BiomeType biome);
f32 biome_features_get_humidity(BiomeFeaturesSystem *system, BiomeType biome);
Vec3 biome_features_get_sky_color(BiomeFeaturesSystem *system, BiomeType biome);
Vec3 biome_features_get_fog_color(BiomeFeaturesSystem *system, BiomeType biome);

// Particle effects
void biome_features_spawn_particles(BiomeFeaturesSystem *system,
                                    BiomeType biome, Vec3 position,
                                    f32 delta_time);
void biome_features_emit_biome_particles(BiomeFeaturesSystem *system,
                                         BiomeType biome, Vec3 position);

// Biome transitions and blending
bool biome_features_is_in_transition(BiomeFeaturesSystem *system,
                                     BiomeType biome1, BiomeType biome2);
f32 biome_features_get_transition_blend(BiomeFeaturesSystem *system,
                                        BiomeType from, BiomeType to,
                                        f32 position);

// Weather effects on biomes
void biome_features_apply_weather_effects(BiomeFeaturesSystem *system,
                                          BiomeType biome);

// Rain shadow effects (rain on lee side of mountains)
f32 biome_features_get_rain_shadow_factor(BiomeFeaturesSystem *system,
                                          BiomeType biome, Vec3 wind_direction);

// Edge effects (where biome transitions occur)
void biome_features_spawn_edge_particles(BiomeFeaturesSystem *system,
                                         BiomeType biome1, BiomeType biome2,
                                         Vec3 position);

// Utility functions
const char *biome_type_get_name(BiomeType biome);
const char *biome_type_get_description(BiomeType biome);

// Biome neighbor queries
BiomeType biome_features_get_next_biome(BiomeFeaturesSystem *system,
                                        BiomeType current, Vec3 direction);

#endif // BIOME_FEATURES_H

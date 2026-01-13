// Biome features implementation
#include "engine/include/core/logger.h"
#include <string.h>
#include <weather/weather.h>
#include <world/biome_features.h>

// Internal storage for biome profiles
static BiomeProfile s_profiles[BIOME_COUNT];
static bool s_initialized = false;

// Default colors
static const Vec3 COLOR_SKY_DEFAULT = {0.46f, 0.65f, 1.0f};
static const Vec3 COLOR_FOG_DEFAULT = {0.8f, 0.8f, 1.0f};
static const Vec3 COLOR_GRASS_PLAINS = {0.57f, 0.73f, 0.35f};

void biome_features_init(BiomeFeaturesSystem *system) {
  if (s_initialized)
    return;

  // Initialize all profiles with defaults
  for (int i = 0; i < BIOME_COUNT; i++) {
    BiomeProfile *p = &s_profiles[i];
    p->type = (BiomeType)i;
    p->name = "Unknown";
    p->visual.sky_color = COLOR_SKY_DEFAULT;
    p->visual.fog_color = COLOR_FOG_DEFAULT;
    p->visual.primary_color = COLOR_GRASS_PLAINS; // Grass color
    p->temperature.base_temperature = 0.5f;
    p->temperature.humidity = 0.5f;
  }

  // Configure specific biomes
  // Plains
  s_profiles[BIOME_PLAINS].name = "Plains";
  s_profiles[BIOME_PLAINS].description = "Flat grassy areas";

  // Desert
  s_profiles[BIOME_DESERT].name = "Desert";
  s_profiles[BIOME_DESERT].description = "Hot sandy wasteland";
  s_profiles[BIOME_DESERT].temperature.base_temperature = 2.0f;
  s_profiles[BIOME_DESERT].temperature.humidity = 0.0f;
  s_profiles[BIOME_DESERT].precipitation.has_precipitation = false;
  s_profiles[BIOME_DESERT].visual.primary_color =
      (Vec3){0.76f, 0.69f, 0.44f}; // Dried grass

  // Forest
  s_profiles[BIOME_FOREST].name = "Forest";
  s_profiles[BIOME_FOREST].temperature.base_temperature = 0.7f;
  s_profiles[BIOME_FOREST].temperature.humidity = 0.8f;
  s_profiles[BIOME_FOREST].visual.primary_color =
      (Vec3){0.48f, 0.68f, 0.23f}; // Deep green

  // Mountains
  s_profiles[BIOME_MOUNTAINS].name = "Mountains";
  s_profiles[BIOME_MOUNTAINS].temperature.base_temperature = 0.2f; // Cold
  s_profiles[BIOME_MOUNTAINS].precipitation.prefers_snow = true;

  // Ocean
  s_profiles[BIOME_OCEAN].name = "Ocean";
  s_profiles[BIOME_OCEAN].temperature.base_temperature = 0.5f;

  // New Biomes
  s_profiles[BIOME_MANGROVE_SWAMP].name = "Mangrove Swamp";
  s_profiles[BIOME_MANGROVE_SWAMP].temperature.base_temperature = 0.8f;
  s_profiles[BIOME_MANGROVE_SWAMP].temperature.humidity = 0.9f;
  s_profiles[BIOME_MANGROVE_SWAMP].visual.primary_color =
      (Vec3){0.41f, 0.49f, 0.21f}; // Muddy green

  s_profiles[BIOME_BAMBOO_JUNGLE].name = "Bamboo Jungle";
  s_profiles[BIOME_BAMBOO_JUNGLE].temperature.base_temperature = 0.95f;
  s_profiles[BIOME_BAMBOO_JUNGLE].temperature.humidity = 0.9f;
  s_profiles[BIOME_BAMBOO_JUNGLE].visual.primary_color =
      (Vec3){0.35f, 0.70f, 0.15f}; // Bright jungle green

  s_profiles[BIOME_CHERRY_GROVE].name = "Cherry Grove";
  s_profiles[BIOME_CHERRY_GROVE].visual.primary_color =
      (Vec3){0.70f, 0.85f, 0.45f}; // Lime green
  // Add particle effect: Falling petals (placeholder logic)

  s_profiles[BIOME_ICE_SPIKES].name = "Ice Spikes";
  s_profiles[BIOME_ICE_SPIKES].temperature.base_temperature = -0.5f;
  s_profiles[BIOME_ICE_SPIKES].precipitation.prefers_snow = true;
  s_profiles[BIOME_ICE_SPIKES].visual.sky_color =
      (Vec3){0.60f, 0.70f, 1.0f}; // Icy blue

  s_initialized = true;
}

void biome_features_free(BiomeFeaturesSystem *system) {
  // Nothing to free yet (static profiles)
}

BiomeProfile *biome_features_get_profile(BiomeFeaturesSystem *system,
                                         BiomeType biome) {
  if (biome >= BIOME_COUNT)
    return &s_profiles[BIOME_PLAINS];
  return &s_profiles[biome];
}

f32 biome_features_get_temperature(BiomeFeaturesSystem *system,
                                   BiomeType biome) {
  return biome_features_get_profile(system, biome)
      ->temperature.base_temperature;
}

f32 biome_features_get_humidity(BiomeFeaturesSystem *system, BiomeType biome) {
  return biome_features_get_profile(system, biome)->temperature.humidity;
}

Vec3 biome_features_get_sky_color(BiomeFeaturesSystem *system,
                                  BiomeType biome) {
  return biome_features_get_profile(system, biome)->visual.sky_color;
}

Vec3 biome_features_get_fog_color(BiomeFeaturesSystem *system,
                                  BiomeType biome) {
  return biome_features_get_profile(system, biome)->visual.fog_color;
}

void biome_features_apply_weather_effects(BiomeFeaturesSystem *system,
                                          BiomeType biome) {
  // Placeholder: Change fog density/color based on weather
}

void biome_features_spawn_particles(BiomeFeaturesSystem *system,
                                    BiomeType biome, Vec3 position,
                                    f32 delta_time) {
  // Placeholder: Spawn ambient particles
  // e.g. if biome == BIOME_MUSHROOM_ISLAND -> spawn spores
}

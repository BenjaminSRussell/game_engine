#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct ParticleSystem ParticleSystem;
typedef struct PostProcessing PostProcessing;
typedef struct Weather Weather;

// Particle system
typedef struct {
  float position[3];
  float velocity[3];
  float color[4];
  float size;
  float lifetime;
} Particle;

ParticleSystem *particle_system_create(uint32_t max_particles);
void particle_system_emit(ParticleSystem *system, const Particle *particle);
void particle_system_update(ParticleSystem *system, float delta_time);
void particle_system_destroy(ParticleSystem *system);

// Post-processing
PostProcessing *postprocessing_create(void);
void postprocessing_enable_bloom(PostProcessing *pp, bool enabled);
void postprocessing_enable_ssao(PostProcessing *pp, bool enabled);
void postprocessing_set_exposure(PostProcessing *pp, float exposure);
void postprocessing_destroy(PostProcessing *pp);

// Weather system
typedef enum {
  WEATHER_CLEAR,
  WEATHER_RAIN,
  WEATHER_SNOW,
  WEATHER_STORM
} WeatherType;

Weather *weather_create(void);
void weather_set_type(Weather *weather, WeatherType type);
void weather_update(Weather *weather, float delta_time);
void weather_destroy(Weather *weather);

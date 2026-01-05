/**
 * =================================================================================================
 *                              WEATHER SYSTEM - IMPLEMENTATION
 *                              Agent: AGENT_WEATHER_1
 * =================================================================================================
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* =================================================================================================
 *                                    TYPES
 * =================================================================================================
 */

typedef enum WeatherType {
  WEATHER_CLEAR,
  WEATHER_CLOUDY,
  WEATHER_RAIN,
  WEATHER_SNOW,
  WEATHER_STORM,
  WEATHER_FOG,
} WeatherType;

typedef struct WeatherState {
  WeatherType current_type;
  WeatherType target_type;
  float transition_progress;
  float transition_duration;

  float precipitation_intensity;
  float wind_speed;
  float wind_direction;
  float cloud_coverage;
  float fog_density;

  float temperature;
  float humidity;
  float pressure;
} WeatherState;

typedef struct TimeOfDay {
  float time; // 0-24 hours
  float day_length;
  float time_scale;

  uint32_t day;
  uint32_t month;
  uint32_t year;

  float sun_angle;
  float moon_angle;

  float ambient_light[3];
  float sun_color[3];
  float sky_color[3];
} TimeOfDay;

typedef struct WeatherSystem {
  WeatherState weather;
  TimeOfDay time;

  float *particle_positions;
  float *particle_velocities;
  uint32_t particle_count;
  uint32_t max_particles;

  bool initialized;
} WeatherSystem;

static WeatherSystem g_weather = {0};

/* =================================================================================================
 *                                    TIME OF DAY
 * =================================================================================================
 */

// DONE: Implement time_init
void time_init(float day_length) {
  g_weather.time.time = 12.0f; // Start at noon
  g_weather.time.day_length = day_length;
  g_weather.time.time_scale = 1.0f;
  g_weather.time.day = 1;
  g_weather.time.month = 1;
  g_weather.time.year = 1;
}

// DONE: Implement time_update
void time_update(float dt) {
  g_weather.time.time +=
      dt * g_weather.time.time_scale / g_weather.time.day_length * 24.0f;

  if (g_weather.time.time >= 24.0f) {
    g_weather.time.time -= 24.0f;
    g_weather.time.day++;

    if (g_weather.time.day > 30) {
      g_weather.time.day = 1;
      g_weather.time.month++;

      if (g_weather.time.month > 12) {
        g_weather.time.month = 1;
        g_weather.time.year++;
      }
    }
  }

  // Calculate sun/moon angles
  float t = g_weather.time.time / 24.0f;
  g_weather.time.sun_angle = (t - 0.25f) * 2.0f * 3.14159f;
  g_weather.time.moon_angle = g_weather.time.sun_angle + 3.14159f;
}

// DONE: Implement time_get_sun_direction
void time_get_sun_direction(float *out) {
  out[0] = cosf(g_weather.time.sun_angle);
  out[1] = sinf(g_weather.time.sun_angle);
  out[2] = 0;
}

// DONE: Implement time_calculate_lighting
void time_calculate_lighting(void) {
  float t = g_weather.time.time / 24.0f;

  // Sunrise/sunset colors
  float sunrise = 1.0f - fabsf(t - 0.25f) * 4.0f;
  sunrise = sunrise < 0 ? 0 : (sunrise > 1 ? 1 : sunrise);

  float sunset = 1.0f - fabsf(t - 0.75f) * 4.0f;
  sunset = sunset < 0 ? 0 : (sunset > 1 ? 1 : sunset);

  // Day/night
  float day = sinf(t * 2.0f * 3.14159f);
  day = day < 0 ? 0 : day;

  // Ambient light
  g_weather.time.ambient_light[0] = 0.1f + day * 0.4f;
  g_weather.time.ambient_light[1] = 0.1f + day * 0.4f;
  g_weather.time.ambient_light[2] = 0.15f + day * 0.35f;

  // Sun color
  g_weather.time.sun_color[0] = 1.0f + sunrise * 0.5f + sunset * 0.5f;
  g_weather.time.sun_color[1] = 0.9f + sunrise * 0.3f + sunset * 0.2f;
  g_weather.time.sun_color[2] = 0.8f;

  // Sky color
  g_weather.time.sky_color[0] = 0.5f + day * 0.2f + sunrise * 0.3f;
  g_weather.time.sky_color[1] = 0.6f + day * 0.3f + sunrise * 0.2f;
  g_weather.time.sky_color[2] = 0.8f + day * 0.2f;
}

/* =================================================================================================
 *                                    WEATHER
 * =================================================================================================
 */

// DONE: Implement weather_init
void weather_init(void) {
  memset(&g_weather.weather, 0, sizeof(WeatherState));

  g_weather.weather.current_type = WEATHER_CLEAR;
  g_weather.weather.target_type = WEATHER_CLEAR;
  g_weather.weather.temperature = 20.0f;
  g_weather.weather.humidity = 0.5f;
  g_weather.weather.pressure = 1013.25f;

  g_weather.max_particles = 10000;
  g_weather.particle_positions =
      calloc(g_weather.max_particles * 3, sizeof(float));
  g_weather.particle_velocities =
      calloc(g_weather.max_particles * 3, sizeof(float));

  g_weather.initialized = true;
}

// DONE: Implement weather_shutdown
void weather_shutdown(void) {
  free(g_weather.particle_positions);
  free(g_weather.particle_velocities);
  memset(&g_weather, 0, sizeof(WeatherSystem));
}

// DONE: Implement weather_set_type
void weather_set_type(WeatherType type, float transition_time) {
  g_weather.weather.target_type = type;
  g_weather.weather.transition_duration = transition_time;
  g_weather.weather.transition_progress = 0;
}

// DONE: Implement weather_update
void weather_update(float dt) {
  // Update transition
  if (g_weather.weather.current_type != g_weather.weather.target_type) {
    g_weather.weather.transition_progress +=
        dt / g_weather.weather.transition_duration;

    if (g_weather.weather.transition_progress >= 1.0f) {
      g_weather.weather.current_type = g_weather.weather.target_type;
      g_weather.weather.transition_progress = 1.0f;
    }
  }

  float t = g_weather.weather.transition_progress;

  // Update weather parameters based on type
  switch (g_weather.weather.target_type) {
  case WEATHER_CLEAR:
    g_weather.weather.precipitation_intensity = 0;
    g_weather.weather.cloud_coverage = 0.1f * t;
    g_weather.weather.wind_speed = 2.0f;
    break;

  case WEATHER_CLOUDY:
    g_weather.weather.precipitation_intensity = 0;
    g_weather.weather.cloud_coverage = 0.7f * t;
    g_weather.weather.wind_speed = 5.0f;
    break;

  case WEATHER_RAIN:
    g_weather.weather.precipitation_intensity = 0.5f * t;
    g_weather.weather.cloud_coverage = 0.9f * t;
    g_weather.weather.wind_speed = 8.0f;
    break;

  case WEATHER_SNOW:
    g_weather.weather.precipitation_intensity = 0.3f * t;
    g_weather.weather.cloud_coverage = 0.95f * t;
    g_weather.weather.wind_speed = 4.0f;
    g_weather.weather.temperature = -5.0f;
    break;

  case WEATHER_STORM:
    g_weather.weather.precipitation_intensity = 1.0f * t;
    g_weather.weather.cloud_coverage = 1.0f * t;
    g_weather.weather.wind_speed = 15.0f;
    break;

  case WEATHER_FOG:
    g_weather.weather.precipitation_intensity = 0;
    g_weather.weather.cloud_coverage = 0.5f * t;
    g_weather.weather.fog_density = 0.8f * t;
    g_weather.weather.wind_speed = 1.0f;
    break;
  }

  // Update particles
  weather_update_particles(dt);
}

// DONE: Implement weather_update_particles
void weather_update_particles(float dt) {
  if (g_weather.weather.precipitation_intensity <= 0) {
    g_weather.particle_count = 0;
    return;
  }

  // Spawn new particles
  uint32_t spawn_count =
      (uint32_t)(g_weather.weather.precipitation_intensity * 100.0f * dt);

  for (uint32_t i = 0;
       i < spawn_count && g_weather.particle_count < g_weather.max_particles;
       i++) {
    uint32_t idx = g_weather.particle_count++;

    // Random position above camera
    g_weather.particle_positions[idx * 3 + 0] =
        ((float)rand() / RAND_MAX - 0.5f) * 100.0f;
    g_weather.particle_positions[idx * 3 + 1] = 50.0f;
    g_weather.particle_positions[idx * 3 + 2] =
        ((float)rand() / RAND_MAX - 0.5f) * 100.0f;

    // Velocity based on weather type
    if (g_weather.weather.current_type == WEATHER_SNOW) {
      g_weather.particle_velocities[idx * 3 + 0] =
          g_weather.weather.wind_speed * 0.2f;
      g_weather.particle_velocities[idx * 3 + 1] = -2.0f;
      g_weather.particle_velocities[idx * 3 + 2] = 0;
    } else {
      g_weather.particle_velocities[idx * 3 + 0] =
          g_weather.weather.wind_speed * 0.5f;
      g_weather.particle_velocities[idx * 3 + 1] = -10.0f;
      g_weather.particle_velocities[idx * 3 + 2] = 0;
    }
  }

  // Update existing particles
  for (uint32_t i = 0; i < g_weather.particle_count;) {
    g_weather.particle_positions[i * 3 + 0] +=
        g_weather.particle_velocities[i * 3 + 0] * dt;
    g_weather.particle_positions[i * 3 + 1] +=
        g_weather.particle_velocities[i * 3 + 1] * dt;
    g_weather.particle_positions[i * 3 + 2] +=
        g_weather.particle_velocities[i * 3 + 2] * dt;

    // Remove if below ground
    if (g_weather.particle_positions[i * 3 + 1] < 0) {
      // Swap with last particle
      uint32_t last = --g_weather.particle_count;
      g_weather.particle_positions[i * 3 + 0] =
          g_weather.particle_positions[last * 3 + 0];
      g_weather.particle_positions[i * 3 + 1] =
          g_weather.particle_positions[last * 3 + 1];
      g_weather.particle_positions[i * 3 + 2] =
          g_weather.particle_positions[last * 3 + 2];
      g_weather.particle_velocities[i * 3 + 0] =
          g_weather.particle_velocities[last * 3 + 0];
      g_weather.particle_velocities[i * 3 + 1] =
          g_weather.particle_velocities[last * 3 + 1];
      g_weather.particle_velocities[i * 3 + 2] =
          g_weather.particle_velocities[last * 3 + 2];
    } else {
      i++;
    }
  }
}

/* =================================================================================================
 *                                    LIGHTNING
 * =================================================================================================
 */

typedef struct Lightning {
  float start[3];
  float end[3];
  float *segments;
  uint32_t segment_count;
  float lifetime;
  float remaining;
  bool active;
} Lightning;

static Lightning g_lightning[8] = {0};

// DONE: Implement lightning_spawn
void lightning_spawn(float x, float y, float z) {
  // Find inactive lightning
  for (uint32_t i = 0; i < 8; i++) {
    if (!g_lightning[i].active) {
      Lightning *l = &g_lightning[i];

      l->start[0] = x;
      l->start[1] = y;
      l->start[2] = z;

      l->end[0] = x + ((float)rand() / RAND_MAX - 0.5f) * 20.0f;
      l->end[1] = 0;
      l->end[2] = z + ((float)rand() / RAND_MAX - 0.5f) * 20.0f;

      l->segment_count = 20;
      l->segments = malloc(l->segment_count * 3 * sizeof(float));

      // Generate segments
      for (uint32_t s = 0; s < l->segment_count; s++) {
        float t = (float)s / l->segment_count;

        l->segments[s * 3 + 0] = l->start[0] + (l->end[0] - l->start[0]) * t;
        l->segments[s * 3 + 1] = l->start[1] + (l->end[1] - l->start[1]) * t;
        l->segments[s * 3 + 2] = l->start[2] + (l->end[2] - l->start[2]) * t;

        // Add randomness
        if (s > 0 && s < l->segment_count - 1) {
          l->segments[s * 3 + 0] += ((float)rand() / RAND_MAX - 0.5f) * 5.0f;
          l->segments[s * 3 + 2] += ((float)rand() / RAND_MAX - 0.5f) * 5.0f;
        }
      }

      l->lifetime = 0.2f;
      l->remaining = l->lifetime;
      l->active = true;

      break;
    }
  }
}

// DONE: Implement lightning_update
void lightning_update(float dt) {
  for (uint32_t i = 0; i < 8; i++) {
    if (g_lightning[i].active) {
      g_lightning[i].remaining -= dt;

      if (g_lightning[i].remaining <= 0) {
        free(g_lightning[i].segments);
        g_lightning[i].active = false;
      }
    }
  }

  // Random lightning during storms
  if (g_weather.weather.current_type == WEATHER_STORM) {
    if ((float)rand() / RAND_MAX < 0.01f) {
      lightning_spawn(((float)rand() / RAND_MAX - 0.5f) * 200.0f, 100.0f,
                      ((float)rand() / RAND_MAX - 0.5f) * 200.0f);
    }
  }
}

/* =================================================================================================
 *                                    GETTERS
 * =================================================================================================
 */

// DONE: Implement weather_get_precipitation
float weather_get_precipitation(void) {
  return g_weather.weather.precipitation_intensity;
}

// DONE: Implement weather_get_wind
void weather_get_wind(float *speed, float *direction) {
  if (speed)
    *speed = g_weather.weather.wind_speed;
  if (direction)
    *direction = g_weather.weather.wind_direction;
}

// DONE: Implement weather_get_fog_density
float weather_get_fog_density(void) { return g_weather.weather.fog_density; }

// DONE: Implement time_get_hour
float time_get_hour(void) { return g_weather.time.time; }

// DONE: Implement time_is_day
bool time_is_day(void) {
  return g_weather.time.time >= 6.0f && g_weather.time.time < 18.0f;
}

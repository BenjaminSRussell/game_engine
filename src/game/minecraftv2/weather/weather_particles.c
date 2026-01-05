// Weather particle effects and rendering system
#include <game_common.h>
#include <math.h>
#include <math/vec3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <weather/weather.h>

// Initialize particle system
void weather_particles_init(WeatherParticleSystem *particles) {
  if (!particles)
    return;
  memset(particles, 0, sizeof(WeatherParticleSystem));
  particles->active_count = 0;
  particles->spawn_timer = 0.0f;
  particles->enabled = true;
  particles->spawn_rate = 100;
}

// Cleanup particle system
void weather_particles_free(WeatherParticleSystem *particles) {
  if (!particles)
    return;
  particles->active_count = 0;
  particles->spawn_timer = 0.0f;
}

// Spawn new particles based on weather conditions
void weather_particles_spawn(WeatherParticleSystem *particles, WeatherType type,
                             Vec3 center, f32 radius) {
  // Helper function used by update, or explicitly
  // For now, internal logic in update handles spawning
}

static void spawn_particles_update(WeatherParticleSystem *particles,
                                   const WeatherSystem *weather,
                                   f32 delta_time) {
  if (!particles || !weather || !weather->initialized || !particles->enabled)
    return;

  particles->spawn_timer += delta_time;
  f32 spawn_interval = 1.0f / (f32)particles->spawn_rate;

  u32 particles_to_spawn = (u32)(particles->spawn_timer / spawn_interval);
  particles->spawn_timer -= particles_to_spawn * spawn_interval;

  // Adjust spawn rate based on weather intensity
  particles_to_spawn = (u32)(particles_to_spawn * weather->current.intensity);

  for (u32 i = 0; i < particles_to_spawn &&
                  particles->active_count < MAX_WEATHER_PARTICLES;
       i++) {
    WeatherParticle *particle = &particles->particles[particles->active_count];

    // Random spawn position within player vicinity (using 0,0,0 as player pos
    // stub) In a real system we'd need player position passed in Assuming
    // 'center' is around 0,0,0 for now or fixed camera
    f32 spawn_radius = 50.0f;
    f32 angle = (f32)(rand() % 360) * 3.14159f / 180.0f;
    f32 distance = (f32)(rand() % 1000) / 1000.0f * spawn_radius;

    particle->position.x = cosf(angle) * distance;
    particle->position.y =
        30.0f + (f32)(rand() % 1000) / 1000.0f * 20.0f; // 30-50m high
    particle->position.z = sinf(angle) * distance;

    // Set particle properties based on weather type
    switch (weather->current.type) {
    case WEATHER_RAIN_LIGHT:
    case WEATHER_RAIN_MODERATE:
    case WEATHER_RAIN_HEAVY:
      particle->type = 0;
      particle->velocity.x = weather->current.wind_direction.x *
                             weather->current.wind_speed * 0.1f;
      particle->velocity.y = -15.0f; // Rain falls fast
      particle->velocity.z = weather->current.wind_direction.z *
                             weather->current.wind_speed * 0.1f;
      particle->size = 0.05f + (f32)(rand() % 100) / 10000.0f; // 0.05-0.06m
      particle->max_life = 3.0f; // 3 seconds lifetime
      break;

    case WEATHER_SNOW_LIGHT:
    case WEATHER_SNOW_MODERATE:
    case WEATHER_SNOW_HEAVY:
    case WEATHER_BLIZZARD:
      particle->type = 1;
      particle->velocity.x = weather->current.wind_direction.x *
                             weather->current.wind_speed * 0.05f;
      particle->velocity.y = -2.0f; // Snow falls slowly
      particle->velocity.z = weather->current.wind_direction.z *
                             weather->current.wind_speed * 0.05f;
      particle->size = 0.1f + (f32)(rand() % 100) / 1000.0f; // 0.1-0.2m
      particle->max_life = 8.0f; // 8 seconds lifetime
      break;

    case WEATHER_FOG_LIGHT:
    case WEATHER_FOG_MODERATE:
    case WEATHER_FOG_HEAVY:
      particle->type = 2;
      particle->velocity.x = weather->current.wind_direction.x *
                             weather->current.wind_speed * 0.02f;
      particle->velocity.y = 0.0f; // Fog drifts horizontally
      particle->velocity.z = weather->current.wind_direction.z *
                             weather->current.wind_speed * 0.02f;
      particle->size = 2.0f + (f32)(rand() % 100) / 100.0f; // 2-3m
      particle->max_life = 20.0f; // 20 seconds lifetime
      break;

    default:
      continue; // Don't spawn particles for clear weather
    }

    particle->life = particle->max_life;
    particles->active_count++;
  }
}

// Update particle positions and remove dead particles
void weather_particles_update(WeatherParticleSystem *particles,
                              const WeatherSystem *weather, f32 delta_time) {
  if (!particles)
    return;

  spawn_particles_update(particles, weather, delta_time);

  for (u32 i = 0; i < particles->active_count;) {
    WeatherParticle *particle = &particles->particles[i];

    // Update position
    particle->position.x += particle->velocity.x * delta_time;
    particle->position.y += particle->velocity.y * delta_time;
    particle->position.z += particle->velocity.z * delta_time;

    // Update life
    particle->life -= delta_time;

    // Remove dead particles or particles that fell too far
    if (particle->life <= 0.0f || particle->position.y < -10.0f) {
      // Move last particle to this position
      if (i < particles->active_count - 1) {
        particles->particles[i] =
            particles->particles[particles->active_count - 1];
      }
      particles->active_count--;
    } else {
      i++;
    }
  }
}

// Render particles using Vulkan (stubbed/legacy)
void weather_particles_render(const WeatherParticleSystem *particles,
                              VulkanRenderer *renderer) {
  if (!particles || !renderer)
    return;
  // TODO: Implement rendering
}

u32 weather_particles_get_count(const WeatherParticleSystem *particles) {
  return particles ? particles->active_count : 0;
}

const WeatherParticle *
weather_particles_get_data(const WeatherParticleSystem *particles) {
  return particles ? particles->particles : NULL;
}

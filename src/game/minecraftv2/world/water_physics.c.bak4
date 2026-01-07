// Water physics implementation - currents, pressure, waves
#include <core/logger.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <world/water_physics.h>

// Default physics configuration
static const WaterPhysicsConfig DEFAULT_CONFIG = {
    .current_strength = 1.0f,
    .pressure_scale = 0.1f,
    .wave_amplitude = 0.5f,
    .max_currents = 64,
    .max_waves = 128,
    .enable_currents = true,
    .enable_pressure = true,
    .enable_waves = true,
    .water_density = 1000.0f, // kg/m³
    .water_viscosity = 0.001f // Pa·s
};

// Simple wave simulation using sine function
static f32 wave_height_at(const WaterWave *wave, f32 distance, f32 time) {
  if (!wave->active)
    return 0.0f;

  // Dampen wave over time
  f32 age_factor = 1.0f - (wave->age / 10.0f);
  if (age_factor < 0.0f)
    age_factor = 0.0f;

  // Calculate wave height using sine wave equation
  f32 k = 2.0f * 3.14159f / wave->wavelength; // Wave number
  f32 phase_offset = wave->speed * wave->age;

  f32 height = wave->amplitude * age_factor * sinf(k * distance - phase_offset);

  return height;
}

void water_physics_init(WaterPhysicsSystem *physics, ChunkManager *chunks,
                        u32 max_currents, u32 max_waves) {
  if (!physics)
    return;

  memset(physics, 0, sizeof(WaterPhysicsSystem));

  physics->chunk_manager = chunks;
  physics->config = DEFAULT_CONFIG;
  physics->config.max_currents = max_currents;
  physics->config.max_waves = max_waves;

  // Allocate current array
  physics->currents =
      (WaterCurrent *)malloc(max_currents * sizeof(WaterCurrent));
  if (!physics->currents) {
    LOG_ERROR("Failed to allocate water currents");
    return;
  }

  // Allocate wave array
  physics->waves = (WaterWave *)malloc(max_waves * sizeof(WaterWave));
  if (!physics->waves) {
    LOG_ERROR("Failed to allocate water waves");
    free(physics->currents);
    return;
  }

  // Initialize arrays
  for (u32 i = 0; i < max_currents; i++) {
    physics->currents[i].active = false;
  }

  for (u32 i = 0; i < max_waves; i++) {
    physics->waves[i].active = false;
  }

  physics->active_currents = 0;
  physics->active_waves = 0;
  physics->initialized = true;

  LOG_INFO("Water physics system initialized with %u currents and %u waves",
           max_currents, max_waves);
}

void water_physics_free(WaterPhysicsSystem *physics) {
  if (!physics)
    return;

  if (physics->currents) {
    free(physics->currents);
    physics->currents = NULL;
  }

  if (physics->waves) {
    free(physics->waves);
    physics->waves = NULL;
  }

  memset(physics, 0, sizeof(WaterPhysicsSystem));
}

void water_physics_update(WaterPhysicsSystem *physics, f32 delta_time) {
  if (!physics || !physics->initialized)
    return;

  // Update currents
  u32 active_currents = 0;
  for (u32 i = 0; i < physics->config.max_currents; i++) {
    WaterCurrent *current = &physics->currents[i];

    if (!current->active)
      continue;

    water_physics_update_current(current, delta_time);

    if (current->lifetime > 0.0f) {
      active_currents++;
    } else {
      current->active = false;
    }
  }
  physics->active_currents = active_currents;

  // Update waves
  u32 active_waves = 0;
  for (u32 i = 0; i < physics->config.max_waves; i++) {
    WaterWave *wave = &physics->waves[i];

    if (!wave->active)
      continue;

    water_physics_update_wave(wave, delta_time);

    if (wave->age < 10.0f) {
      active_waves++;
    } else {
      wave->active = false;
    }
  }
  physics->active_waves = active_waves;

  // Simulate water spreading
  if (physics->config.enable_currents) {
    water_physics_simulate_spreading(physics, physics->chunk_manager,
                                     delta_time);
  }
}

void water_physics_create_current(WaterPhysicsSystem *physics, Vec3 position,
                                  Vec3 velocity, f32 strength, f32 radius) {
  if (!physics || !physics->initialized)
    return;

  // Find an inactive current slot
  for (u32 i = 0; i < physics->config.max_currents; i++) {
    WaterCurrent *current = &physics->currents[i];

    if (!current->active) {
      current->position = position;
      current->velocity = velocity;
      current->strength = strength;
      current->radius = radius;
      current->lifetime = 5.0f; // 5 second lifetime
      current->active = true;

      LOG_TRACE(
          "Water current created at (%.1f, %.1f, %.1f) with strength %.2f",
          position.x, position.y, position.z, strength);
      return;
    }
  }

  LOG_WARN("Could not create water current - no available slots");
}

void water_physics_update_current(WaterCurrent *current, f32 delta_time) {
  if (!current || !current->active)
    return;

  // Update lifetime
  current->lifetime -= delta_time;

  // Apply simple dissipation
  current->strength *= 0.98f;

  // Update position (currents can move)
  current->position =
      vec3_add(current->position, vec3_mul(current->velocity, delta_time));

  // Reduce radius over time
  current->radius *= 0.99f;
}

bool water_physics_get_current_at(const WaterPhysicsSystem *physics,
                                  Vec3 position, Vec3 *out_velocity) {
  if (!physics || !physics->initialized || !out_velocity)
    return false;

  Vec3 total_velocity = vec3(0.0f, 0.0f, 0.0f);
  bool found = false;

  // Check all active currents
  for (u32 i = 0; i < physics->config.max_currents; i++) {
    const WaterCurrent *current = &physics->currents[i];

    if (!current->active)
      continue;

    // Calculate distance to current center
    Vec3 diff = vec3_sub(position, current->position);
    f32 distance = vec3_length(diff);

    if (distance < current->radius && distance > 0.001f) {
      // Apply current influence (stronger closer to center)
      f32 influence = 1.0f - (distance / current->radius);
      total_velocity =
          vec3_add(total_velocity,
                   vec3_mul(current->velocity, influence * current->strength));
      found = true;
    }
  }

  *out_velocity = total_velocity;
  return found;
}

WaterPressureData water_physics_calculate_pressure(WaterPhysicsSystem *physics,
                                                   Vec3 position) {
  WaterPressureData data = {0};
  data.position = position;

  if (!physics)
    return data;

  // Calculate depth (distance from surface)
  // Assuming water surface is at the highest water block
  f32 surface_height = position.y; // Simplified - should query actual surface
  f32 depth = fmaxf(0.0f, surface_height - position.y);

  data.depth = depth;

  // Calculate pressure using hydrostatic equation: P = ρ * g * h
  // P = density * gravity * depth
  f32 gravity = 9.81f;
  data.pressure = physics->config.water_density * gravity * depth;

  // Calculate pressure gradient direction (always pointing up)
  data.pressure_direction = vec3(0.0f, 1.0f, 0.0f);

  return data;
}

f32 water_physics_get_pressure(WaterPhysicsSystem *physics, Vec3 position) {
  if (!physics)
    return 0.0f;

  WaterPressureData data = water_physics_calculate_pressure(physics, position);
  return data.pressure;
}

f32 water_physics_get_depth(WaterPhysicsSystem *physics, Vec3 position) {
  if (!physics)
    return 0.0f;

  WaterPressureData data = water_physics_calculate_pressure(physics, position);
  return data.depth;
}

void water_physics_create_wave(WaterPhysicsSystem *physics, Vec3 origin,
                               f32 amplitude, f32 wavelength) {
  if (!physics || !physics->initialized)
    return;

  // Find an inactive wave slot
  for (u32 i = 0; i < physics->config.max_waves; i++) {
    WaterWave *wave = &physics->waves[i];

    if (!wave->active) {
      wave->origin = origin;
      wave->amplitude = amplitude * physics->config.wave_amplitude;
      wave->wavelength = wavelength;
      wave->speed = 5.0f; // Wave speed
      wave->age = 0.0f;
      wave->active = true;

      LOG_TRACE("Water wave created at (%.1f, %.1f, %.1f) with amplitude %.2f",
                origin.x, origin.y, origin.z, amplitude);
      return;
    }
  }

  LOG_WARN("Could not create water wave - no available slots");
}

void water_physics_update_wave(WaterWave *wave, f32 delta_time) {
  if (!wave || !wave->active)
    return;

  wave->age += delta_time;

  // Waves dissipate over time
  wave->amplitude *= 0.99f;
}

f32 water_physics_get_wave_height(const WaterPhysicsSystem *physics,
                                  Vec3 position) {
  if (!physics || !physics->initialized)
    return 0.0f;

  f32 total_height = 0.0f;

  // Sum height from all active waves
  for (u32 i = 0; i < physics->config.max_waves; i++) {
    const WaterWave *wave = &physics->waves[i];

    if (!wave->active)
      continue;

    // Calculate distance from wave origin in horizontal plane
    Vec3 diff = vec3_sub(position, wave->origin);
    f32 horizontal_distance = sqrtf(diff.x * diff.x + diff.z * diff.z);

    f32 height = wave_height_at(wave, horizontal_distance, 0.0f);
    total_height += height;
  }

  return total_height;
}

Vec3 water_physics_get_velocity_field(const WaterPhysicsSystem *physics,
                                      Vec3 position) {
  if (!physics)
    return vec3(0.0f, 0.0f, 0.0f);

  Vec3 current_velocity = vec3(0.0f, 0.0f, 0.0f);

  // Get velocity from currents
  water_physics_get_current_at(physics, position, &current_velocity);

  // Add wave-induced velocity (simplified)
  f32 wave_height = water_physics_get_wave_height(physics, position);
  if (fabsf(wave_height) > 0.01f) {
    // Wave adds small vertical component
    current_velocity.y += wave_height * 0.5f;
  }

  return current_velocity;
}

f32 water_physics_get_flow_speed(const WaterPhysicsSystem *physics,
                                 Vec3 position) {
  if (!physics)
    return 0.0f;

  Vec3 velocity = water_physics_get_velocity_field(physics, position);
  return vec3_length(velocity);
}

void water_physics_set_config(WaterPhysicsSystem *physics,
                              const WaterPhysicsConfig *config) {
  if (!physics || !config)
    return;
  physics->config = *config;
}

WaterPhysicsConfig water_physics_get_default_config(void) {
  return DEFAULT_CONFIG;
}

WaterfallData water_physics_get_waterfall_at(WaterPhysicsSystem *physics,
                                             Vec3 position) {
  WaterfallData data = {0};
  data.position = position;
  data.direction = vec3(0.0f, -1.0f, 0.0f);
  data.height = 0.0f;
  data.width = 1.0f;
  data.flow_speed = 0.0f;
  data.splash_force = 0.0f;
  data.particle_count = 0;

  if (!physics)
    return data;

  // Simplified waterfall detection
  // In a real implementation, this would scan downward for flowing water
  f32 flow_speed = water_physics_get_flow_speed(physics, position);

  if (flow_speed > 1.0f) {
    data.flow_speed = flow_speed;
    data.splash_force = flow_speed * 10.0f;
    data.particle_count = (u32)(flow_speed * 50.0f);
    data.height = 10.0f; // Simplified
  }

  return data;
}

void water_physics_simulate_spreading(WaterPhysicsSystem *physics,
                                      ChunkManager *chunks, f32 delta_time) {
  if (!physics || !chunks)
    return;

  // Simplified water spreading simulation
  // In a full implementation, this would:
  // 1. Check for water blocks
  // 2. Calculate pressure gradients
  // 3. Spread water to adjacent blocks
  // 4. Create currents in flowing areas

  // For now, this is a placeholder that demonstrates the architecture
  LOG_TRACE("Water spreading simulation step (%.3f seconds)", delta_time);
}

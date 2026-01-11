// Environmental effects implementation
#include <weather/environmental_effects.h>
#include <core/logger.h>
#include <world/biome_features.h>
#include <math.h>
#include <string.h>

void env_effects_init(EnvironmentalSystem *sys) {
  if (!sys)
    return;

  memset(sys, 0, sizeof(EnvironmentalSystem));
  sys->fog_transition_speed = 0.5f;
  sys->current_biome_id = 1; // Plains default

  sys->current_fog.color = (Vec3){0.8f, 0.8f, 1.0f};
  sys->current_fog.start_distance = 64.0f;
  sys->current_fog.end_distance = 128.0f;
  sys->current_fog.density = 0.01f;
  sys->current_fog.mode = FOG_MODE_LINEAR;

  sys->target_fog = sys->current_fog;
  sys->initialized = true;

  LOG_INFO("Environmental effects system initialized");
}

void env_effects_update(EnvironmentalSystem *sys, Vec3 player_pos, u32 biome_id,
                        f32 dt) {
  if (!sys || !sys->initialized)
    return;

  sys->current_biome_id = biome_id;

  // Smoothly transition fog
  sys->current_fog.color =
      vec3_lerp(sys->current_fog.color, sys->target_fog.color,
                dt * sys->fog_transition_speed);

  f32 dist_lerp = dt * sys->fog_transition_speed;
  sys->current_fog.start_distance +=
      (sys->target_fog.start_distance - sys->current_fog.start_distance) *
      dist_lerp;
  sys->current_fog.end_distance +=
      (sys->target_fog.end_distance - sys->current_fog.end_distance) *
      dist_lerp;
  sys->current_fog.density +=
      (sys->target_fog.density - sys->current_fog.density) * dist_lerp;

  // Update target based on biome (would normally query BiomeFeaturesSystem)
  // Placeholder logic:
  if (biome_id == BIOME_SWAMP || biome_id == BIOME_MANGROVE_SWAMP) {
    sys->target_fog.color = (Vec3){0.4f, 0.5f, 0.4f};
    sys->target_fog.density = 0.05f;
    sys->target_fog.start_distance = 10.0f;
    sys->target_fog.end_distance = 40.0f;
  } else if (biome_id == BIOME_DESERT) {
    sys->target_fog.color = (Vec3){0.9f, 0.8f, 0.6f}; // Dusty
    sys->target_fog.density = 0.005f;
    sys->target_fog.start_distance = 64.0f;
    sys->target_fog.end_distance = 200.0f;
  } else {
    // Default
    sys->target_fog.color = (Vec3){0.8f, 0.8f, 1.0f};
    sys->target_fog.density = 0.01f;
    sys->target_fog.start_distance = 64.0f;
    sys->target_fog.end_distance = 128.0f;
  }
}

FogConfig env_effects_get_fog(EnvironmentalSystem *sys) {
  if (!sys) {
    FogConfig default_fog = {0};
    default_fog.end_distance = 100.0f;
    return default_fog;
  }
  return sys->current_fog;
}

void env_effects_spawn_biome_particles(EnvironmentalSystem *sys, Vec3 center,
                                       u32 biome_id) {
  // Placeholder: Check biome type and spawn appropriate particles
  // e.g., Spores for mushroom island
}

Vec3 env_effects_get_sun_color(f32 time_of_day) {
  // Sunrise/Sunset handling
  if (time_of_day > 0.2f && time_of_day < 0.3f) { // Sunrise
    return (Vec3){1.0f, 0.6f, 0.3f};
  } else if (time_of_day > 0.7f && time_of_day < 0.8f) { // Sunset
    return (Vec3){1.0f, 0.5f, 0.2f};
  } else if (time_of_day >= 0.3f && time_of_day <= 0.7f) { // Day
    return (Vec3){1.0f, 1.0f, 0.9f};
  } else { // Night
    return (Vec3){0.1f, 0.1f, 0.2f};
  }
}

f32 env_effects_get_light_intensity(f32 time_of_day) {
  // Simple sine wave approximation
  f32 intensity = sinf(time_of_day * 3.14159f); // 0 at dawn/dusk, 1 at noon
  if (intensity < 0.1f)
    intensity = 0.1f; // Moonlight/Starlight
  return intensity;
}

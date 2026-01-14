// src/audio/ambient_system.c
// Environmental ambient sound system with dynamic zones and layers

#include <audio/ambient_system.h>
#include <include/math/math_all.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_ZONE_TRANSITION_DURATION 2.0f
#define DEFAULT_FADE_SPEED 1.0f
#define AMBIENT_UPDATE_RATE 0.1f

// Helper functions
static void ambient_update_zone_transitions(AmbientSystem *ambient,
                                            f32 delta_time);
static void ambient_update_layer_volumes(AmbientSystem *ambient,
                                         f32 delta_time);
static void ambient_start_zone_layers(AmbientSystem *ambient,
                                      EnvironmentZone zone);
static void ambient_stop_zone_layers(AmbientSystem *ambient,
                                     EnvironmentZone zone);
static f32 ambient_calculate_layer_volume(AmbientSystem *ambient,
                                          AmbientType type);

void ambient_system_init(AmbientSystem *ambient, AudioSystem *audio) {
  if (!ambient || !audio)
    return;

  memset(ambient, 0, sizeof(AmbientSystem));
  ambient->initialized = true;
  ambient->audio = audio;

  // Initialize global ambient layers
  for (u32 i = 0; i < AMBIENT_COUNT; i++) {
    ambient->global_layers[i].sound_type =
        SOUND_AMBIENT_WIND + i; // Map to existing sounds
    ambient->global_layers[i].channel = 0xFFFFFFFF;
    ambient->global_layers[i].base_volume = 0.0f;
    ambient->global_layers[i].current_volume = 0.0f;
    ambient->global_layers[i].target_volume = 0.0f;
    ambient->global_layers[i].fade_speed = DEFAULT_FADE_SPEED;
    ambient->global_layers[i].active = false;
    ambient->global_layers[i].looping = true;
    ambient->global_layers[i].position = vec3(0.0f, 0.0f, 0.0f);
    ambient->global_layers[i].max_distance = 50.0f;
  }

  // Initialize zones
  ambient->zone_count = 0;
  ambient->current_zone = ENVIRONMENT_SURFACE;
  ambient->previous_zone = ENVIRONMENT_SURFACE;
  ambient->zone_transition_time = 0.0f;
  ambient->zone_transition_duration = DEFAULT_ZONE_TRANSITION_DURATION;

  // Initialize environmental parameters
  ambient->time_of_day = 0.5f;                        // Noon
  ambient->weather_intensity = 0.0f;                  // Clear weather
  ambient->player_position = vec3(0.0f, 64.0f, 0.0f); // Surface level
  ambient->player_height = 64.0f;

  // Initialize dynamic parameters
  ambient->wind_intensity = 0.3f;
  ambient->water_proximity = 0.0f;
  ambient->cave_depth = 0.0f;
  ambient->forest_density = 0.0f;
  ambient->biome_temperature = 0.5f;

  // Initialize volumes
  ambient->master_volume = 0.6f;
  for (u32 i = 0; i < ENVIRONMENT_COUNT; i++) {
    ambient->zone_volumes[i] = 1.0f;
  }
  for (u32 i = 0; i < AMBIENT_COUNT; i++) {
    ambient->ambient_volumes[i] = 1.0f;
  }

  // Performance settings
  ambient->max_concurrent_ambients = 8;
  ambient->active_ambient_count = 0;
  ambient->enable_spatial_ambience = true;
  ambient->update_rate = AMBIENT_UPDATE_RATE;
  ambient->accumulated_time = 0.0f;

  // Create default environmental zones
  ambient_add_zone(ambient, ENVIRONMENT_SURFACE,
                   vec3(-1000.0f, -1000.0f, -1000.0f),
                   vec3(1000.0f, 1000.0f, 1000.0f));

}

void ambient_system_free(AmbientSystem *ambient) {
  if (!ambient || !ambient->initialized)
    return;

  // Stop all ambient layers
  for (u32 i = 0; i < AMBIENT_COUNT; i++) {
    if (ambient->global_layers[i].active &&
        ambient->global_layers[i].channel != 0xFFFFFFFF) {
      audio_stop_sound(ambient->audio, ambient->global_layers[i].channel);
    }
  }

  ambient->initialized = false;
}

void ambient_system_update(AmbientSystem *ambient, f32 delta_time) {
  if (!ambient || !ambient->initialized)
    return;

  // Accumulate time for rate-limited updates
  ambient->accumulated_time += delta_time;

  if (ambient->accumulated_time >= ambient->update_rate) {
    f32 update_delta = ambient->accumulated_time;
    ambient->accumulated_time = 0.0f;

    // Detect current environment zone
    EnvironmentZone detected_zone =
        ambient_detect_environment(ambient, ambient->player_position);

    // Handle zone transitions
    if (detected_zone != ambient->current_zone) {
      ambient->previous_zone = ambient->current_zone;
      ambient->current_zone = detected_zone;
      ambient->zone_transition_time = 0.0f;

             ambient_get_zone_name(detected_zone));
    }

    // Update zone transitions
    ambient_update_zone_transitions(ambient, update_delta);

    // Update layer volumes based on environmental conditions
    ambient_update_layer_volumes(ambient, update_delta);
  }
}

u32 ambient_add_zone(AmbientSystem *ambient, EnvironmentZone zone_type,
                     Vec3 min_bounds, Vec3 max_bounds) {
  if (!ambient || !ambient->initialized || ambient->zone_count >= 32) {
    return 0xFFFFFFFF;
  }

  u32 zone_index = ambient->zone_count++;
  AudioZone *zone = &ambient->zones[zone_index];

  zone->zone_type = zone_type;
  zone->min_bounds = min_bounds;
  zone->max_bounds = max_bounds;
  zone->active_layers = 0;
  zone->transition_fade_time = DEFAULT_ZONE_TRANSITION_DURATION;
  zone->active = true;

  // Initialize zone-specific ambient layers
  switch (zone_type) {
  case ENVIRONMENT_SURFACE:
    // Surface ambience: wind, birds (if forest), water proximity
    zone->layers[zone->active_layers++] = ambient->global_layers[AMBIENT_WIND];
    break;

  case ENVIRONMENT_UNDERGROUND:
    // Underground ambience: cave sounds, dripping
    zone->layers[zone->active_layers++] = ambient->global_layers[AMBIENT_CAVE];
    break;

  case ENVIRONMENT_UNDERWATER:
    // Underwater ambience: muffled sounds, bubbles
    zone->layers[zone->active_layers++] = ambient->global_layers[AMBIENT_WATER];
    break;

  case ENVIRONMENT_NETHER:
    // Nether ambience: fire, ominous sounds
    zone->layers[zone->active_layers++] = ambient->global_layers[AMBIENT_FIRE];
    zone->layers[zone->active_layers++] =
        ambient->global_layers[AMBIENT_NETHER];
    break;

  case ENVIRONMENT_END:
    // End ambience: eerie, void sounds
    zone->layers[zone->active_layers++] = ambient->global_layers[AMBIENT_END];
    break;

  default:
    break;
  }

         ambient_get_zone_name(zone_type));
  return zone_index;
}

void ambient_remove_zone(AmbientSystem *ambient, u32 zone_index) {
  if (!ambient || !ambient->initialized || zone_index >= ambient->zone_count)
    return;

  AudioZone *zone = &ambient->zones[zone_index];

  // Stop all layers in this zone
  for (u32 i = 0; i < zone->active_layers; i++) {
    if (zone->layers[i].active && zone->layers[i].channel != 0xFFFFFFFF) {
      audio_stop_sound(ambient->audio, zone->layers[i].channel);
    }
  }

  zone->active = false;
}

void ambient_set_zone_volume(AmbientSystem *ambient, EnvironmentZone zone,
                             f32 volume) {
  if (!ambient || zone >= ENVIRONMENT_COUNT)
    return;

  ambient->zone_volumes[zone] = fmaxf(0.0f, fminf(1.0f, volume));
}

void ambient_play_layer(AmbientSystem *ambient, AmbientType type, f32 volume) {
  if (!ambient || !ambient->initialized || type >= AMBIENT_COUNT)
    return;

  AmbientLayer *layer = &ambient->global_layers[type];

  if (!layer->active) {
    layer->channel =
        audio_play_sound_ambient(ambient->audio, layer->sound_type, volume);
    if (layer->channel != 0xFFFFFFFF) {
      layer->active = true;
      layer->current_volume = volume;
      layer->target_volume = volume;
      layer->base_volume = volume;
      ambient->active_ambient_count++;

    }
  } else {
    // Update volume if already active
    ambient_set_layer_volume(ambient, type, volume);
  }
}

void ambient_stop_layer(AmbientSystem *ambient, AmbientType type) {
  if (!ambient || !ambient->initialized || type >= AMBIENT_COUNT)
    return;

  AmbientLayer *layer = &ambient->global_layers[type];

  if (layer->active && layer->channel != 0xFFFFFFFF) {
    audio_stop_sound(ambient->audio, layer->channel);
    layer->active = false;
    layer->channel = 0xFFFFFFFF;
    layer->current_volume = 0.0f;
    layer->target_volume = 0.0f;
    ambient->active_ambient_count--;

  }
}

void ambient_fade_layer(AmbientSystem *ambient, AmbientType type,
                        f32 target_volume, f32 duration) {
  if (!ambient || !ambient->initialized || type >= AMBIENT_COUNT)
    return;

  AmbientLayer *layer = &ambient->global_layers[type];

  layer->target_volume = target_volume;
  layer->fade_speed = (duration > 0.0f) ? (1.0f / duration) : 10.0f;

  if (!layer->active && target_volume > 0.01f) {
    ambient_play_layer(ambient, type, 0.0f); // Start with 0 volume and fade in
  } else if (layer->active && target_volume < 0.01f) {
    // Will fade out and stop automatically in update
  }
}

void ambient_set_layer_volume(AmbientSystem *ambient, AmbientType type,
                              f32 volume) {
  if (!ambient || type >= AMBIENT_COUNT)
    return;

  ambient->ambient_volumes[type] = fmaxf(0.0f, fminf(1.0f, volume));

  AmbientLayer *layer = &ambient->global_layers[type];
  if (layer->active && layer->channel != 0xFFFFFFFF) {
    f32 final_volume = volume * ambient->master_volume;
    audio_set_sound_volume(ambient->audio, layer->channel, final_volume);
    layer->current_volume = final_volume;
    layer->target_volume = final_volume;
  }
}

void ambient_set_player_position(AmbientSystem *ambient, Vec3 position) {
  if (!ambient)
    return;

  ambient->player_position = position;
  ambient->player_height = position.y;

  // Update cave depth based on height
  f32 sea_level = 64.0f;
  f32 depth = fmaxf(0.0f, sea_level - position.y);
  ambient->cave_depth = fminf(1.0f, depth / 100.0f);
}

void ambient_set_time_of_day(AmbientSystem *ambient, f32 time) {
  if (!ambient)
    return;

  ambient->time_of_day = fmaxf(0.0f, fminf(1.0f, time));

  // Night ambience
  if (ambient->time_of_day < 0.25f || ambient->time_of_day > 0.75f) {
    ambient_set_layer_volume(ambient, AMBIENT_NIGHT, 0.3f);
  } else {
    ambient_fade_layer(ambient, AMBIENT_NIGHT, 0.0f, 2.0f);
  }
}

void ambient_set_weather_intensity(AmbientSystem *ambient, f32 intensity) {
  if (!ambient)
    return;

  ambient->weather_intensity = fmaxf(0.0f, fminf(1.0f, intensity));

  // Wind intensity increases with weather
  ambient->wind_intensity = 0.3f + (intensity * 0.7f);
}

void ambient_set_biome_data(AmbientSystem *ambient, f32 temperature,
                            f32 forest_density) {
  if (!ambient)
    return;

  ambient->biome_temperature = fmaxf(0.0f, fminf(1.0f, temperature));
  ambient->forest_density = fmaxf(0.0f, fminf(1.0f, forest_density));
}

void ambient_set_wind_intensity(AmbientSystem *ambient, f32 intensity) {
  if (!ambient)
    return;

  ambient->wind_intensity = fmaxf(0.0f, fminf(1.0f, intensity));
}

void ambient_set_water_proximity(AmbientSystem *ambient, f32 proximity) {
  if (!ambient)
    return;

  ambient->water_proximity = fmaxf(0.0f, fminf(1.0f, proximity));
}

void ambient_set_cave_depth(AmbientSystem *ambient, f32 depth) {
  if (!ambient)
    return;

  ambient->cave_depth = fmaxf(0.0f, fminf(1.0f, depth));
}

void ambient_set_environmental_data(AmbientSystem *ambient, f32 wind, f32 water,
                                    f32 cave, f32 forest, f32 temperature) {
  if (!ambient)
    return;

  ambient->wind_intensity = fmaxf(0.0f, fminf(1.0f, wind));
  ambient->water_proximity = fmaxf(0.0f, fminf(1.0f, water));
  ambient->cave_depth = fmaxf(0.0f, fminf(1.0f, cave));
  ambient->forest_density = fmaxf(0.0f, fminf(1.0f, forest));
  ambient->biome_temperature = fmaxf(0.0f, fminf(1.0f, temperature));
}

void ambient_set_master_volume(AmbientSystem *ambient, f32 volume) {
  if (!ambient)
    return;

  ambient->master_volume = fmaxf(0.0f, fminf(1.0f, volume));

  // Update all active layers
  for (u32 i = 0; i < AMBIENT_COUNT; i++) {
    AmbientLayer *layer = &ambient->global_layers[i];
    if (layer->active && layer->channel != 0xFFFFFFFF) {
      f32 final_volume = layer->current_volume * ambient->master_volume;
      audio_set_sound_volume(ambient->audio, layer->channel, final_volume);
    }
  }
}

void ambient_set_ambient_volume(AmbientSystem *ambient, AmbientType type,
                                f32 volume) {
  if (!ambient || type >= AMBIENT_COUNT)
    return;

  ambient->ambient_volumes[type] = fmaxf(0.0f, fminf(1.0f, volume));
  ambient_set_layer_volume(ambient, type, volume);
}

// Helper function implementations
static void ambient_update_zone_transitions(AmbientSystem *ambient,
                                            f32 delta_time) {
  if (ambient->zone_transition_time < ambient->zone_transition_duration) {
    ambient->zone_transition_time += delta_time;

    f32 progress =
        ambient->zone_transition_time / ambient->zone_transition_duration;

    // Fade out previous zone layers
    if (ambient->previous_zone != ambient->current_zone) {
      ambient_stop_zone_layers(ambient, ambient->previous_zone);
    }

    // Fade in current zone layers
    ambient_start_zone_layers(ambient, ambient->current_zone);

    if (progress >= 1.0f) {
      ambient->previous_zone = ambient->current_zone;
    }
  }
}

static void ambient_update_layer_volumes(AmbientSystem *ambient,
                                         f32 delta_time) {
  for (u32 i = 0; i < AMBIENT_COUNT; i++) {
    AmbientLayer *layer = &ambient->global_layers[i];

    if (layer->active) {
      // Calculate target volume based on environmental conditions
      f32 calculated_volume =
          ambient_calculate_layer_volume(ambient, (AmbientType)i);
      layer->target_volume = calculated_volume;

      // Smooth volume transitions
      f32 volume_diff = layer->target_volume - layer->current_volume;
      if (fabsf(volume_diff) > 0.001f) {
        f32 step = volume_diff * layer->fade_speed * delta_time;
        layer->current_volume += step;

        // Apply volume to audio system
        if (layer->channel != 0xFFFFFFFF) {
          f32 final_volume = layer->current_volume * ambient->master_volume;
          audio_set_sound_volume(ambient->audio, layer->channel, final_volume);
        }
      }

      // Stop layer if volume is near zero
      if (layer->current_volume < 0.01f && layer->target_volume < 0.01f) {
        ambient_stop_layer(ambient, (AmbientType)i);
      }
    }
  }
}

static void ambient_start_zone_layers(AmbientSystem *ambient,
                                      EnvironmentZone zone) {
  // Start ambient layers appropriate for the current zone
  switch (zone) {
  case ENVIRONMENT_SURFACE:
    ambient_play_layer(ambient, AMBIENT_WIND, 0.3f);
    if (ambient->forest_density > 0.3f) {
      ambient_play_layer(ambient, AMBIENT_FOREST, 0.2f);
    }
    if (ambient->water_proximity > 0.3f) {
      ambient_play_layer(ambient, AMBIENT_WATER, 0.2f);
    }
    break;

  case ENVIRONMENT_UNDERGROUND:
    ambient_play_layer(ambient, AMBIENT_CAVE, 0.4f);
    break;

  case ENVIRONMENT_UNDERWATER:
    ambient_play_layer(ambient, AMBIENT_WATER, 0.6f);
    break;

  case ENVIRONMENT_NETHER:
    ambient_play_layer(ambient, AMBIENT_FIRE, 0.5f);
    ambient_play_layer(ambient, AMBIENT_NETHER, 0.3f);
    break;

  case ENVIRONMENT_END:
    ambient_play_layer(ambient, AMBIENT_END, 0.4f);
    break;

  default:
    break;
  }
}

static void ambient_stop_zone_layers(AmbientSystem *ambient,
                                     EnvironmentZone zone) {
  // Fade out ambient layers for the previous zone
  switch (zone) {
  case ENVIRONMENT_SURFACE:
    ambient_fade_layer(ambient, AMBIENT_WIND, 0.0f, 2.0f);
    ambient_fade_layer(ambient, AMBIENT_FOREST, 0.0f, 2.0f);
    ambient_fade_layer(ambient, AMBIENT_WATER, 0.0f, 2.0f);
    break;

  case ENVIRONMENT_UNDERGROUND:
    ambient_fade_layer(ambient, AMBIENT_CAVE, 0.0f, 2.0f);
    break;

  case ENVIRONMENT_UNDERWATER:
    ambient_fade_layer(ambient, AMBIENT_WATER, 0.0f, 2.0f);
    break;

  case ENVIRONMENT_NETHER:
    ambient_fade_layer(ambient, AMBIENT_FIRE, 0.0f, 2.0f);
    ambient_fade_layer(ambient, AMBIENT_NETHER, 0.0f, 2.0f);
    break;

  case ENVIRONMENT_END:
    ambient_fade_layer(ambient, AMBIENT_END, 0.0f, 2.0f);
    break;

  default:
    break;
  }
}

static f32 ambient_calculate_layer_volume(AmbientSystem *ambient,
                                          AmbientType type) {
  f32 volume = 0.0f;

  switch (type) {
  case AMBIENT_WIND:
    volume = ambient->wind_intensity * 0.4f;
    // Increase wind during storms
    volume += ambient->weather_intensity * 0.3f;
    break;

  case AMBIENT_WATER:
    volume = ambient->water_proximity * 0.5f;
    break;

  case AMBIENT_FIRE:
    volume = (ambient->current_zone == ENVIRONMENT_NETHER) ? 0.5f : 0.0f;
    break;

  case AMBIENT_CAVE:
    volume = ambient->cave_depth * 0.6f;
    break;

  case AMBIENT_FOREST:
    volume = ambient->forest_density * 0.3f;
    // Less forest sound at night
    if (ambient->time_of_day < 0.25f || ambient->time_of_day > 0.75f) {
      volume *= 0.5f;
    }
    break;

  case AMBIENT_NIGHT:
    volume = (ambient->time_of_day < 0.25f || ambient->time_of_day > 0.75f)
                 ? 0.3f
                 : 0.0f;
    break;

  case AMBIENT_UNDERGROUND:
    volume = ambient->cave_depth * 0.4f;
    break;

  case AMBIENT_NETHER:
    volume = (ambient->current_zone == ENVIRONMENT_NETHER) ? 0.3f : 0.0f;
    break;

  case AMBIENT_END:
    volume = (ambient->current_zone == ENVIRONMENT_END) ? 0.4f : 0.0f;
    break;

  default:
    volume = 0.0f;
    break;
  }

  return volume * ambient->ambient_volumes[type];
}

// Utility functions
const char *ambient_get_type_name(AmbientType type) {
  static const char *type_names[] = {"Wind",        "Water",  "Fire", "Cave",
                                     "Forest",      "Desert", "Snow", "Night",
                                     "Underground", "Nether", "End"};

  if (type >= AMBIENT_COUNT)
    return "Unknown";
  return type_names[type];
}

const char *ambient_get_zone_name(EnvironmentZone zone) {
  static const char *zone_names[] = {"Surface", "Underground", "Underwater",
                                     "Nether", "End"};

  if (zone >= ENVIRONMENT_COUNT)
    return "Unknown";
  return zone_names[zone];
}

EnvironmentZone ambient_detect_environment(AmbientSystem *ambient,
                                           Vec3 position) {
  if (!ambient)
    return ENVIRONMENT_SURFACE;

  // Check if in any defined zone
  for (u32 i = 0; i < ambient->zone_count; i++) {
    AudioZone *zone = &ambient->zones[i];
    if (zone->active && ambient_is_position_in_zone(zone, position)) {
      return zone->zone_type;
    }
  }

  // Default detection based on height
  if (position.y < 0.0f) {
    return ENVIRONMENT_UNDERGROUND;
  } else if (position.y < 64.0f) {
    // Check for water (simplified)
    return ENVIRONMENT_UNDERWATER;
  }

  return ENVIRONMENT_SURFACE;
}

bool ambient_is_position_in_zone(AudioZone *zone, Vec3 position) {
  if (!zone)
    return false;

  return (
      position.x >= zone->min_bounds.x && position.x <= zone->max_bounds.x &&
      position.y >= zone->min_bounds.y && position.y <= zone->max_bounds.y &&
      position.z >= zone->min_bounds.z && position.z <= zone->max_bounds.z);
}

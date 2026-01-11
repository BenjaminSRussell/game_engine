// src/audio/audio_manager.c
// Unified audio system manager implementation

#include <audio/audio_manager.h>
#include <audio/audio_occlusion_raycast.h>
#include <math/math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUDIO_UPDATE_RATE 0.016f // ~60 FPS

// Helper function to map environment names to enums
static EnvironmentType get_environment_from_name(const char *name) {
  if (!name)
    return ENVIRONMENT_OUTDOOR_OPEN;

  if (strcmp(name, "outdoor_open") == 0)
    return ENVIRONMENT_OUTDOOR_OPEN;
  if (strcmp(name, "outdoor_semi") == 0)
    return ENVIRONMENT_OUTDOOR_SEMI;
  if (strcmp(name, "small_room") == 0)
    return ENVIRONMENT_SMALL_ROOM;
  if (strcmp(name, "medium_room") == 0)
    return ENVIRONMENT_MEDIUM_ROOM;
  if (strcmp(name, "large_room") == 0)
    return ENVIRONMENT_LARGE_ROOM;
  if (strcmp(name, "cavern") == 0)
    return ENVIRONMENT_CAVERN;
  if (strcmp(name, "hall") == 0)
    return ENVIRONMENT_HALL;

  return ENVIRONMENT_OUTDOOR_OPEN;
}

static MusicMood get_music_mood_from_name(const char *name) {
  if (!name)
    return MOOD_CALM_EXPLORATION;

  if (strcmp(name, "menu") == 0)
    return MOOD_MENU;
  if (strcmp(name, "calm") == 0 || strcmp(name, "calm_exploration") == 0)
    return MOOD_CALM_EXPLORATION;
  if (strcmp(name, "adventure") == 0)
    return MOOD_ADVENTURE;
  if (strcmp(name, "tension") == 0)
    return MOOD_TENSION;
  if (strcmp(name, "combat") == 0)
    return MOOD_COMBAT;
  if (strcmp(name, "boss") == 0)
    return MOOD_BOSS;
  if (strcmp(name, "night") == 0)
    return MOOD_NIGHT;
  if (strcmp(name, "underground") == 0)
    return MOOD_UNDERGROUND;
  if (strcmp(name, "water") == 0 || strcmp(name, "underwater") == 0)
    return MOOD_WATER;

  return MOOD_CALM_EXPLORATION;
}

void audio_manager_init(AudioManager *manager, u32 max_channels) {
  if (!manager)
    return;

  memset(manager, 0, sizeof(AudioManager));
  manager->initialized = true;

  // Initialize core audio system
  audio_system_init(&manager->audio_system, max_channels);

  // Initialize music system
  music_system_init(&manager->music_system, &manager->audio_system);

  // Initialize ambient system
  ambient_system_init(&manager->ambient_system, &manager->audio_system);

  // Initialize effects system
  audio_effects_init(&manager->effects_system, &manager->audio_system.engine);

  // Set default game state
  manager->player_position = vec3(0.0f, 64.0f, 0.0f);
  manager->player_velocity = vec3(0.0f, 0.0f, 0.0f);
  manager->player_forward = vec3(0.0f, 0.0f, -1.0f);
  manager->player_up = vec3(0.0f, 1.0f, 0.0f);
  manager->time_of_day = 0.5f;       // Noon
  manager->weather_intensity = 0.0f; // Clear weather
  manager->in_combat = false;
  manager->combat_intensity = 0.0f;
  manager->underground = false;
  manager->underground_level = 0.0f;
  manager->underwater = false;
  manager->water_depth = 0.0f;
  manager->in_nether = false;
  manager->in_end = false;

  // Set default audio settings
  manager->master_volume = 0.8f;
  manager->music_volume = 0.7f;
  manager->ambient_volume = 0.6f;
  manager->sfx_volume = 0.8f;
  manager->enable_music = true;
  manager->enable_ambient = true;
  manager->enable_sfx = true;
  manager->enable_effects = true;

  // Performance settings
  manager->total_sounds_played = 0;
  manager->active_sound_count = 0;
  manager->cpu_usage = 0.0f;
  manager->performance_mode = false;

  // Update timing
  manager->accumulated_time = 0.0f;
  manager->update_rate = AUDIO_UPDATE_RATE;

  printf("[AUDIO_MANAGER] Audio manager initialized with %u channels\n",
         max_channels);
}

void audio_manager_free(AudioManager *manager) {
  if (!manager || !manager->initialized)
    return;

  // Free all subsystems
  audio_system_free(&manager->audio_system);
  music_system_free(&manager->music_system);
  ambient_system_free(&manager->ambient_system);
  audio_effects_shutdown(&manager->effects_system);

  manager->initialized = false;
  printf("[AUDIO_MANAGER] Audio manager freed\n");
}

void audio_manager_update(AudioManager *manager, f32 delta_time) {
  if (!manager || !manager->initialized)
    return;

  // Accumulate time for rate-limited updates
  manager->accumulated_time += delta_time;

  if (manager->accumulated_time >= manager->update_rate) {
    f32 update_delta = manager->accumulated_time;
    manager->accumulated_time = 0.0f;

    // Update audio listener
    audio_update_listener(&manager->audio_system, manager->player_position,
                          manager->player_forward, manager->player_up,
                          manager->player_velocity);

    // Update core audio system
    audio_system_update(&manager->audio_system, update_delta);

    // Update occlusion if physics system is available
    if (manager->block_physics) {
      Audio_UpdateOcclusion(&manager->audio_system, manager->block_physics);
    }

    // Update music system with game state
    if (manager->enable_music) {
      music_system_update(&manager->music_system, update_delta);
      music_set_combat_intensity(&manager->music_system,
                                 manager->combat_intensity);
      music_set_time_of_day(&manager->music_system, manager->time_of_day);
      music_set_player_position(&manager->music_system,
                                manager->player_position);
      music_set_underground_level(&manager->music_system,
                                  manager->underground_level);

      // Update music context based on game state
      MusicContext context = CONTEXT_EXPLORATION;
      if (manager->in_combat) {
        context = (manager->combat_intensity > 0.7f) ? CONTEXT_BOSS_FIGHT
                                                     : CONTEXT_COMBAT;
      } else if (manager->underground) {
        context = CONTEXT_MINING;
      } else if (manager->underwater) {
        context = CONTEXT_UNDERWATER;
      } else if (manager->in_nether) {
        context = CONTEXT_NETHER;
      } else if (manager->in_end) {
        context = CONTEXT_END;
      }

      music_set_context(&manager->music_system, context);
    }

    // Update ambient system with environmental data
    if (manager->enable_ambient) {
      ambient_system_update(&manager->ambient_system, update_delta);
      ambient_set_player_position(&manager->ambient_system,
                                  manager->player_position);
      ambient_set_time_of_day(&manager->ambient_system, manager->time_of_day);
      ambient_set_weather_intensity(&manager->ambient_system,
                                    manager->weather_intensity);
      ambient_set_cave_depth(&manager->ambient_system,
                             manager->underground_level);

      // Set environment-specific parameters
      f32 wind_intensity = 0.3f + manager->weather_intensity * 0.4f;
      f32 water_proximity = manager->underwater ? 1.0f : 0.0f;
      f32 forest_density = 0.0f; // Would be set based on actual biome data
      f32 temperature = 0.5f;    // Would be set based on biome

      ambient_set_environmental_data(
          &manager->ambient_system, wind_intensity, water_proximity,
          manager->underground_level, forest_density, temperature);
    }

    // Update audio effects with environmental reverb
    if (manager->enable_effects) {
      audio_effects_update(&manager->effects_system);
      audio_effects_update_dynamic_reverb(
          &manager->effects_system, manager->player_position, update_delta);
    }

    // Update underwater filter state
    underwater_filter_set_enabled(manager->underwater);
    if (manager->underwater) {
      underwater_filter_update_depth(manager->water_depth);
    }

    // Update statistics
    manager->active_sound_count = manager->audio_system.active_sources;
  }
}

void audio_manager_set_player_transform(AudioManager *manager, Vec3 position,
                                        Vec3 forward, Vec3 up) {
  if (!manager)
    return;

  manager->player_position = position;
  manager->player_forward = forward;
  manager->player_up = up;

  // Update underground level based on height
  f32 sea_level = 64.0f;
  f32 depth = fmaxf(0.0f, sea_level - position.y);
  manager->underground_level = fminf(1.0f, depth / 100.0f);
  manager->underground = manager->underground_level > 0.1f;
}

void audio_manager_set_player_velocity(AudioManager *manager, Vec3 velocity) {
  if (!manager)
    return;
  manager->player_velocity = velocity;
}

void audio_manager_set_physics_system(AudioManager *manager,
                                      BlockPhysicsSystem *physics) {
  if (!manager)
    return;
  manager->block_physics = physics;
}

void audio_manager_set_time_of_day(AudioManager *manager, f32 time) {
  if (!manager)
    return;
  manager->time_of_day = fmaxf(0.0f, fminf(1.0f, time));
}

void audio_manager_set_weather_intensity(AudioManager *manager, f32 intensity) {
  if (!manager)
    return;
  manager->weather_intensity = fmaxf(0.0f, fminf(1.0f, intensity));
}

void audio_manager_set_combat_state(AudioManager *manager, bool in_combat,
                                    f32 intensity) {
  if (!manager)
    return;
  manager->in_combat = in_combat;
  manager->combat_intensity = fmaxf(0.0f, fminf(1.0f, intensity));
}

void audio_manager_set_environment_state(AudioManager *manager,
                                         bool underground, f32 level,
                                         bool underwater, f32 depth,
                                         bool in_nether, bool in_end) {
  if (!manager)
    return;

  manager->underground = underground;
  manager->underground_level = level;
  manager->underwater = underwater;
  manager->water_depth = depth;
  manager->in_nether = in_nether;
  manager->in_end = in_end;
}

void audio_manager_set_master_volume(AudioManager *manager, f32 volume) {
  if (!manager)
    return;

  manager->master_volume = fmaxf(0.0f, fminf(1.0f, volume));
  audio_set_master_volume(&manager->audio_system, manager->master_volume);
}

void audio_manager_set_music_volume(AudioManager *manager, f32 volume) {
  if (!manager)
    return;

  manager->music_volume = fmaxf(0.0f, fminf(1.0f, volume));
  music_set_master_volume(&manager->music_system, manager->music_volume);
}

void audio_manager_set_ambient_volume(AudioManager *manager, f32 volume) {
  if (!manager)
    return;

  manager->ambient_volume = fmaxf(0.0f, fminf(1.0f, volume));
  ambient_set_master_volume(&manager->ambient_system, manager->ambient_volume);
}

void audio_manager_set_sfx_volume(AudioManager *manager, f32 volume) {
  if (!manager)
    return;

  manager->sfx_volume = fmaxf(0.0f, fminf(1.0f, volume));
  audio_set_volume(&manager->audio_system, SOUND_CATEGORY_HOSTILE,
                   manager->sfx_volume);
  audio_set_volume(&manager->audio_system, SOUND_CATEGORY_NEUTRAL,
                   manager->sfx_volume);
  audio_set_volume(&manager->audio_system, SOUND_CATEGORY_PLAYER,
                   manager->sfx_volume);
  audio_set_volume(&manager->audio_system, SOUND_CATEGORY_BLOCK,
                   manager->sfx_volume);
}

void audio_manager_enable_music(AudioManager *manager, bool enabled) {
  if (!manager)
    return;

  manager->enable_music = enabled;
  if (!enabled) {
    music_stop(&manager->music_system);
  }
}

void audio_manager_enable_ambient(AudioManager *manager, bool enabled) {
  if (!manager)
    return;

  manager->enable_ambient = enabled;
  if (!enabled) {
    // Stop all ambient layers
    for (u32 i = 0; i < AMBIENT_COUNT; i++) {
      ambient_stop_layer(&manager->ambient_system, (AmbientType)i);
    }
  }
}

void audio_manager_enable_sfx(AudioManager *manager, bool enabled) {
  if (!manager)
    return;

  manager->enable_sfx = enabled;
  if (!enabled) {
    audio_disable_category(&manager->audio_system, SOUND_CATEGORY_HOSTILE);
    audio_disable_category(&manager->audio_system, SOUND_CATEGORY_NEUTRAL);
    audio_disable_category(&manager->audio_system, SOUND_CATEGORY_PLAYER);
    audio_disable_category(&manager->audio_system, SOUND_CATEGORY_BLOCK);
  } else {
    audio_enable_category(&manager->audio_system, SOUND_CATEGORY_HOSTILE);
    audio_enable_category(&manager->audio_system, SOUND_CATEGORY_NEUTRAL);
    audio_enable_category(&manager->audio_system, SOUND_CATEGORY_PLAYER);
    audio_enable_category(&manager->audio_system, SOUND_CATEGORY_BLOCK);
  }
}

void audio_manager_enable_effects(AudioManager *manager, bool enabled) {
  if (!manager)
    return;
  manager->enable_effects = enabled;
}

u32 audio_manager_play_sound(AudioManager *manager, SoundType sound,
                             Vec3 position, f32 volume) {
  if (!manager || !manager->enable_sfx)
    return 0xFFFFFFFF;

  u32 channel =
      audio_play_sound(&manager->audio_system, sound, position,
                       volume * manager->sfx_volume, SOUND_CATEGORY_HOSTILE);

  if (channel != 0xFFFFFFFF) {
    manager->total_sounds_played++;
  }

  return channel;
}

u32 audio_manager_play_sound_2d(AudioManager *manager, SoundType sound,
                                f32 volume) {
  if (!manager || !manager->enable_sfx)
    return 0xFFFFFFFF;

  u32 channel =
      audio_play_sound_2d(&manager->audio_system, sound,
                          volume * manager->sfx_volume, SOUND_CATEGORY_HOSTILE);

  if (channel != 0xFFFFFFFF) {
    manager->total_sounds_played++;
  }

  return channel;
}

void audio_manager_stop_sound(AudioManager *manager, u32 channel) {
  if (!manager)
    return;
  audio_stop_sound(&manager->audio_system, channel);
}

void audio_manager_stop_all_sounds(AudioManager *manager) {
  if (!manager)
    return;

  // Stop all active sounds
  for (u32 i = 0; i < manager->audio_system.active_sources; i++) {
    if (manager->audio_system.sources[i].active) {
      audio_stop_sound(&manager->audio_system, i);
    }
  }
}

void audio_manager_play_music(AudioManager *manager, const char *track_name) {
  if (!manager || !manager->enable_music)
    return;

  // This would find and play a specific track by name
  // For now, we'll just resume music if it was paused
  music_resume(&manager->music_system);
}

void audio_manager_set_music_mood(AudioManager *manager, const char *mood) {
  if (!manager || !manager->enable_music)
    return;

  MusicMood music_mood = get_music_mood_from_name(mood);
  music_set_mood(&manager->music_system, music_mood);
}

void audio_manager_pause_music(AudioManager *manager) {
  if (!manager)
    return;
  music_pause(&manager->music_system);
}

void audio_manager_resume_music(AudioManager *manager) {
  if (!manager || !manager->enable_music)
    return;
  music_resume(&manager->music_system);
}

void audio_manager_stop_music(AudioManager *manager) {
  if (!manager)
    return;
  music_stop(&manager->music_system);
}

void audio_manager_set_environment(AudioManager *manager,
                                   const char *environment) {
  if (!manager)
    return;

  EnvironmentType env_type = get_environment_from_name(environment);

  // Apply appropriate reverb preset based on environment
  ReverbPreset preset = REVERB_PRESET_MEDIUM_ROOM;
  switch (env_type) {
  case ENVIRONMENT_OUTDOOR_OPEN:
    preset = REVERB_PRESET_CUSTOM; // Minimal reverb
    break;
  case ENVIRONMENT_OUTDOOR_SEMI:
    preset = REVERB_PRESET_SMALL_ROOM;
    break;
  case ENVIRONMENT_SMALL_ROOM:
    preset = REVERB_PRESET_SMALL_ROOM;
    break;
  case ENVIRONMENT_MEDIUM_ROOM:
    preset = REVERB_PRESET_MEDIUM_ROOM;
    break;
  case ENVIRONMENT_LARGE_ROOM:
    preset = REVERB_PRESET_LARGE_ROOM;
    break;
  case ENVIRONMENT_CAVERN:
    preset = REVERB_PRESET_CAVE;
    break;
  case ENVIRONMENT_HALL:
    preset = REVERB_PRESET_HALL;
    break;
  }

  // Create and apply reverb effect
  u32 reverb_effect =
      audio_effect_create_reverb(&manager->effects_system, preset);
  if (reverb_effect != 0xFFFFFFFF) {
    audio_effect_set_active(&manager->effects_system, reverb_effect, true);
  }
}

void audio_manager_set_biome(AudioManager *manager, const char *biome,
                             f32 forest_density) {
  if (!manager)
    return;

  // Update ambient system with biome information
  ambient_set_biome_data(&manager->ambient_system, 0.5f,
                         forest_density); // Default temperature

  // Adjust ambient parameters based on biome
  if (strcmp(biome, "desert") == 0) {
    ambient_set_environmental_data(&manager->ambient_system, 0.6f, 0.0f, 0.0f,
                                   0.0f, 0.9f);
  } else if (strcmp(biome, "forest") == 0) {
    ambient_set_environmental_data(&manager->ambient_system, 0.3f, 0.1f, 0.0f,
                                   forest_density, 0.5f);
  } else if (strcmp(biome, "snow") == 0) {
    ambient_set_environmental_data(&manager->ambient_system, 0.4f, 0.0f, 0.0f,
                                   0.0f, 0.1f);
  } else if (strcmp(biome, "ocean") == 0) {
    ambient_set_environmental_data(&manager->ambient_system, 0.5f, 0.8f, 0.0f,
                                   0.0f, 0.5f);
  }
}

void audio_manager_set_performance_mode(AudioManager *manager, bool enabled) {
  if (!manager)
    return;

  manager->performance_mode = enabled;

  if (enabled) {
    // Reduce update rate for performance
    manager->update_rate = 0.033f; // ~30 FPS

    // Disable some effects
    audio_manager_enable_effects(manager, false);

    // Reduce maximum concurrent sounds
    manager->audio_system.max_channels = 16;
  } else {
    // Restore normal settings
    manager->update_rate = AUDIO_UPDATE_RATE;
    audio_manager_enable_effects(manager, true);
    manager->audio_system.max_channels = 32;
  }

  printf("[AUDIO_MANAGER] Performance mode %s\n",
         enabled ? "enabled" : "disabled");
}

void audio_manager_get_statistics(AudioManager *manager, u32 *total_sounds,
                                  u32 *active_sounds, f32 *cpu_usage) {
  if (!manager)
    return;

  if (total_sounds)
    *total_sounds = manager->total_sounds_played;
  if (active_sounds)
    *active_sounds = manager->active_sound_count;
  if (cpu_usage)
    *cpu_usage = manager->cpu_usage;
}

void audio_manager_debug_print(AudioManager *manager) {
  if (!manager || !manager->initialized)
    return;

  printf("\n=== AUDIO MANAGER DEBUG INFO ===\n");
  printf("Initialized: %s\n", manager->initialized ? "Yes" : "No");
  printf("Performance Mode: %s\n", manager->performance_mode ? "Yes" : "No");

  printf("\n--- Game State ---\n");
  printf("Player Position: (%.2f, %.2f, %.2f)\n", manager->player_position.x,
         manager->player_position.y, manager->player_position.z);
  printf("Time of Day: %.2f\n", manager->time_of_day);
  printf("Weather Intensity: %.2f\n", manager->weather_intensity);
  printf("In Combat: %s (Intensity: %.2f)\n", manager->in_combat ? "Yes" : "No",
         manager->combat_intensity);
  printf("Underground: %s (Level: %.2f)\n", manager->underground ? "Yes" : "No",
         manager->underground_level);
  printf("Underwater: %s\n", manager->underwater ? "Yes" : "No");

  printf("\n--- Audio Settings ---\n");
  printf("Master Volume: %.2f\n", manager->master_volume);
  printf("Music Volume: %.2f (%s)\n", manager->music_volume,
         manager->enable_music ? "Enabled" : "Disabled");
  printf("Ambient Volume: %.2f (%s)\n", manager->ambient_volume,
         manager->enable_ambient ? "Enabled" : "Disabled");
  printf("SFX Volume: %.2f (%s)\n", manager->sfx_volume,
         manager->enable_sfx ? "Enabled" : "Disabled");
  printf("Effects: %s\n", manager->enable_effects ? "Enabled" : "Disabled");

  printf("\n--- Statistics ---\n");
  printf("Total Sounds Played: %u\n", manager->total_sounds_played);
  printf("Active Sounds: %u\n", manager->active_sound_count);
  printf("CPU Usage: %.2f%%\n", manager->cpu_usage * 100.0f);

  printf("\n--- Subsystem Status ---\n");
  printf("Audio System: %s\n",
         manager->audio_system.initialized ? "Initialized" : "Not Initialized");
  printf("Music System: %s\n",
         manager->music_system.initialized ? "Initialized" : "Not Initialized");
  printf("Ambient System: %s\n", manager->ambient_system.initialized
                                     ? "Initialized"
                                     : "Not Initialized");
  printf("Effects System: %s\n", manager->effects_system.initialized
                                     ? "Initialized"
                                     : "Not Initialized");

  printf("================================\n\n");
}

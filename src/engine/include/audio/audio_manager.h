// include/audio/audio_manager.h
// Unified audio system manager that coordinates all audio subsystems

#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <audio/ambient_system.h>
#include <audio/audio_effects.h>
#include <audio/audio_system.h>
#include <audio/music_system.h>
#include "engine/include/common.h"
#include <math/vec3.h>
#include <physics/block_physics.h>

// Master audio manager that coordinates all audio subsystems
typedef struct AudioManager {
  bool initialized;

  // Core audio system
  AudioSystem audio_system;

  // Music system
  MusicSystem music_system;

  // Ambient system
  AmbientSystem ambient_system;

  // Audio effects system
  AudioEffectsSystem effects_system;

  // Game state for audio decisions
  Vec3 player_position;
  Vec3 player_velocity;
  Vec3 player_forward;
  Vec3 player_up;
  f32 time_of_day;
  f32 weather_intensity;
  bool in_combat;
  f32 combat_intensity;
  bool underground;
  f32 underground_level;
  bool underwater;
  f32 water_depth;
  bool in_nether;
  bool in_end;

  // Audio settings
  f32 master_volume;
  f32 music_volume;
  f32 ambient_volume;
  f32 sfx_volume;
  bool enable_music;
  bool enable_ambient;
  bool enable_sfx;
  bool enable_effects;

  // Performance tracking
  u32 total_sounds_played;
  u32 active_sound_count;
  f32 cpu_usage;
  bool performance_mode;

  // Update timing
  f32 accumulated_time;
  f32 update_rate;

  // Physics integration
  BlockPhysicsSystem *block_physics;
} AudioManager;

// Audio manager lifecycle
void audio_manager_init(AudioManager *manager, u32 max_channels);
void audio_manager_free(AudioManager *manager);
void audio_manager_update(AudioManager *manager, f32 delta_time);

// Game state updates
void audio_manager_set_player_transform(AudioManager *manager, Vec3 position,
                                        Vec3 forward, Vec3 up);
void audio_manager_set_player_velocity(AudioManager *manager, Vec3 velocity);
void audio_manager_set_physics_system(AudioManager *manager,
                                      BlockPhysicsSystem *physics);
void audio_manager_set_time_of_day(AudioManager *manager, f32 time);
void audio_manager_set_weather_intensity(AudioManager *manager, f32 intensity);
void audio_manager_set_combat_state(AudioManager *manager, bool in_combat,
                                    f32 intensity);
void audio_manager_set_environment_state(AudioManager *manager,
                                         bool underground, f32 level,
                                         bool underwater, f32 depth,
                                         bool in_nether, bool in_end);

// Audio settings
void audio_manager_set_master_volume(AudioManager *manager, f32 volume);
void audio_manager_set_music_volume(AudioManager *manager, f32 volume);
void audio_manager_set_ambient_volume(AudioManager *manager, f32 volume);
void audio_manager_set_sfx_volume(AudioManager *manager, f32 volume);
void audio_manager_enable_music(AudioManager *manager, bool enabled);
void audio_manager_enable_ambient(AudioManager *manager, bool enabled);
void audio_manager_enable_sfx(AudioManager *manager, bool enabled);
void audio_manager_enable_effects(AudioManager *manager, bool enabled);

// Sound playback (convenience functions)
u32 audio_manager_play_sound(AudioManager *manager, SoundType sound,
                             Vec3 position, f32 volume);
u32 audio_manager_play_sound_2d(AudioManager *manager, SoundType sound,
                                f32 volume);
void audio_manager_stop_sound(AudioManager *manager, u32 channel);
void audio_manager_stop_all_sounds(AudioManager *manager);

// Music control (convenience functions)
void audio_manager_play_music(AudioManager *manager, const char *track_name);
void audio_manager_set_music_mood(AudioManager *manager, const char *mood);
void audio_manager_pause_music(AudioManager *manager);
void audio_manager_resume_music(AudioManager *manager);
void audio_manager_stop_music(AudioManager *manager);

// Environment audio control
void audio_manager_set_environment(AudioManager *manager,
                                   const char *environment);
void audio_manager_set_biome(AudioManager *manager, const char *biome,
                             f32 forest_density);

// Performance and debugging
void audio_manager_set_performance_mode(AudioManager *manager, bool enabled);
void audio_manager_get_statistics(AudioManager *manager, u32 *total_sounds,
                                  u32 *active_sounds, f32 *cpu_usage);
void audio_manager_debug_print(AudioManager *manager);

#endif

#ifndef UNIFIED_AUDIO_H
#define UNIFIED_AUDIO_H

#include <core/types.h>
#include <math/vec3.h>
#include <stdbool.h>

// Opaque types
typedef struct AudioSystem AudioSystem;
typedef struct AudioSource AudioSource;

// Enums
typedef enum {
  AUDIO_CATEGORY_SFX,
  AUDIO_CATEGORY_MUSIC,
  AUDIO_CATEGORY_AMBIENT,
  AUDIO_CATEGORY_VOICE,
  AUDIO_CATEGORY_UI,
  AUDIO_CATEGORY_COUNT
} AudioCategory;

typedef enum {
  AUDIO_STATE_STOPPED,
  AUDIO_STATE_PLAYING,
  AUDIO_STATE_PAUSED
} AudioState;

typedef struct {
  u32 max_channels;
  u32 sample_rate;
  bool enable_spatial_audio;
} AudioConfig;

// Core System
AudioSystem *audio_system_create(AudioConfig config);
void audio_system_destroy(AudioSystem *system);
void audio_system_update(AudioSystem *system, f32 delta_time);

// Listener
void audio_set_listener(AudioSystem *system, Vec3 position, Vec3 forward,
                        Vec3 up, Vec3 velocity);

// Playback
u32 audio_play_sound_2d(AudioSystem *system, const char *filepath, f32 volume,
                        bool loop, AudioCategory category);
u32 audio_play_sound_3d(AudioSystem *system, const char *filepath,
                        Vec3 position, f32 volume, bool loop,
                        AudioCategory category);
void audio_stop_sound(AudioSystem *system, u32 id);
void audio_pause_sound(AudioSystem *system, u32 id);
void audio_resume_sound(AudioSystem *system, u32 id);

// Source Control
void audio_set_source_position(AudioSystem *system, u32 id, Vec3 position);
void audio_set_source_volume(AudioSystem *system, u32 id, f32 volume);
void audio_set_source_pitch(AudioSystem *system, u32 id, f32 pitch);

// Mixer
void audio_set_master_volume(AudioSystem *system, f32 volume);
void audio_set_category_volume(AudioSystem *system, AudioCategory category,
                               f32 volume);

#endif // UNIFIED_AUDIO_H

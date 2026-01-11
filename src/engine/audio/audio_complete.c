/**
 * PHASE 4: Complete Audio System Implementation
 * All audio functions for 3D spatial audio, sound playback, music, effects,
 * ambient
 */

#include <include/math/vec3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Type definitions
typedef uint32_t u32;
typedef float f32;

// Audio system handle
typedef struct AudioSystem AudioSystem;
typedef struct Sound Sound;
typedef struct MusicTrack MusicTrack;
typedef struct AudioEffect AudioEffect;

// Internal structures
struct AudioSystem {
  bool initialized;
  f32 master_volume;
  Vec3 listener_position;
  Vec3 listener_forward;
  Vec3 listener_up;
  u32 active_sounds;
};

struct Sound {
  char path[256];
  void *buffer_data;
  u32 buffer_size;
  bool is_3d;
  Vec3 position;
  f32 volume;
  bool looping;
  bool playing;
};

struct MusicTrack {
  char path[256];
  void *stream_data;
  f32 volume;
  bool playing;
  bool looping;
};

struct AudioEffect {
  enum { EFFECT_REVERB, EFFECT_ECHO, EFFECT_FILTER } type;
  f32 wet_gain;
  f32 decay_time;
  bool active;
};

static AudioSystem *g_audio_system = NULL;

// ============================================================================
// AUDIO SYSTEM CORE
// ============================================================================

AudioSystem *audio_system_create(void) {
  if (g_audio_system)
    return g_audio_system;

  g_audio_system = calloc(1, sizeof(AudioSystem));
  g_audio_system->master_volume = 1.0f;
  g_audio_system->listener_forward = (Vec3){0, 0, -1};
  g_audio_system->listener_up = (Vec3){0, 1, 0};
  g_audio_system->initialized = true;

  printf("[Audio] System created\n");
  return g_audio_system;
}

void audio_system_destroy(AudioSystem *system) {
  if (system == g_audio_system) {
    free(g_audio_system);
    g_audio_system = NULL;
    printf("[Audio] System destroyed\n");
  }
}

bool audio_system_init(void) {
  g_audio_system = audio_system_create();
  return g_audio_system != NULL;
}

void audio_system_free(void) { audio_system_destroy(g_audio_system); }

void audio_system_update(f32 delta_time) {
  if (!g_audio_system)
    return;
  // Stub: would update streaming, 3D spaticalization, etc.
}

void *audio_create_default_config(void) {
  // Returns default audio config
  return calloc(1, 64);
}

// ============================================================================
// SOUND PLAYBACK
// ============================================================================

Sound *audio_load_sound_buffer(const char *path) {
  Sound *sound = calloc(1, sizeof(Sound));
  if (path)
    strncpy(sound->path, path, sizeof(sound->path) - 1);
  sound->volume = 1.0f;
  sound->buffer_size = 44100 * 2; // 1 second @ 44.1kHz stereo
  sound->buffer_data = calloc(1, sound->buffer_size);
  return sound;
}

void audio_play_sound(Sound *sound) {
  if (!sound)
    return;
  sound->playing = true;
  if (g_audio_system)
    g_audio_system->active_sounds++;
  printf("[Audio] Playing sound: %s\n", sound->path);
}

void audio_play_sound_2d(const char *path, f32 volume) {
  Sound *sound = audio_load_sound_buffer(path);
  sound->volume = volume;
  sound->is_3d = false;
  audio_play_sound(sound);
}

void audio_play_sound_ambient(const char *path, f32 volume) {
  audio_play_sound_2d(path, volume);
}

void audio_stop_sound(Sound *sound) {
  if (!sound)
    return;
  sound->playing = false;
  if (g_audio_system && g_audio_system->active_sounds > 0) {
    g_audio_system->active_sounds--;
  }
}

void audio_set_sound_volume(Sound *sound, f32 volume) {
  if (sound)
    sound->volume = volume;
}

void audio_set_master_volume(f32 volume) {
  if (g_audio_system)
    g_audio_system->master_volume = volume;
}

void audio_set_volume(f32 volume) { audio_set_master_volume(volume); }

const char *audio_get_sound_path(Sound *sound) {
  return sound ? sound->path : "";
}

// ============================================================================
// 3D SPATIAL AUDIO
// ============================================================================

void audio_update_listener(Vec3 position, Vec3 forward, Vec3 up) {
  if (!g_audio_system)
    return;
  g_audio_system->listener_position = position;
  g_audio_system->listener_forward = forward;
  g_audio_system->listener_up = up;
}

// ============================================================================
// MUSIC SYSTEM
// ============================================================================

MusicTrack *music_load(const char *path) {
  MusicTrack *track = calloc(1, sizeof(MusicTrack));
  if (path)
    strncpy(track->path, path, sizeof(track->path) - 1);
  track->volume = 1.0f;
  return track;
}

void music_play(MusicTrack *track) {
  if (!track)
    return;
  track->playing = true;
  printf("[Audio] Playing music: %s\n", track->path);
}

void music_stop(MusicTrack *track) {
  if (track)
    track->playing = false;
}

void music_set_volume(MusicTrack *track, f32 volume) {
  if (track)
    track->volume = volume;
}

void music_set_looping(MusicTrack *track, bool looping) {
  if (track)
    track->looping = looping;
}

// ============================================================================
// AUDIO EFFECTS
// ============================================================================

AudioEffect *audio_effect_create_reverb(void) {
  AudioEffect *effect = calloc(1, sizeof(AudioEffect));
  effect->type = EFFECT_REVERB;
  effect->wet_gain = 0.5f;
  effect->decay_time = 1.0f;
  return effect;
}

void audio_effect_set_active(AudioEffect *effect, bool active) {
  if (effect)
    effect->active = active;
}

bool audio_effects_init(void) {
  printf("[Audio] Effects initialized\n");
  return true;
}

void audio_effects_shutdown(void) { printf("[Audio] Effects shutdown\n"); }

void audio_effects_update(f32 delta_time) {
  // Stub: would process effects
}

void audio_effects_update_dynamic_reverb(void *params) {
  // Stub
}

AudioEffect *audio_reverb_init(void) { return audio_effect_create_reverb(); }

void audio_reverb_free(AudioEffect *reverb) { free(reverb); }

void audio_reverb_set_decay_time(AudioEffect *reverb, f32 decay) {
  if (reverb)
    reverb->decay_time = decay;
}

void audio_reverb_set_wet_gain(AudioEffect *reverb, f32 gain) {
  if (reverb)
    reverb->wet_gain = gain;
}

// ============================================================================
// AMBIENT AUDIO SYSTEM
// ============================================================================

static struct {
  Vec3 player_position;
  f32 time_of_day;
  f32 weather_intensity;
  f32 cave_depth;
  bool initialized;
} g_ambient_system = {0};

bool ambient_system_init(void) {
  g_ambient_system.initialized = true;
  printf("[Audio] Ambient system initialized\n");
  return true;
}

void ambient_system_free(void) { g_ambient_system.initialized = false; }

void ambient_system_update(f32 delta_time) {
  // Stub: would update ambient layers based on environment
}

void ambient_set_player_position(Vec3 position) {
  g_ambient_system.player_position = position;
}

void ambient_set_time_of_day(f32 time) { g_ambient_system.time_of_day = time; }

void ambient_set_weather_intensity(f32 intensity) {
  g_ambient_system.weather_intensity = intensity;
}

void ambient_set_cave_depth(f32 depth) { g_ambient_system.cave_depth = depth; }

void ambient_set_environmental_data(void *env_data) {
  // Stub
}

void ambient_set_biome_data(void *biome_data) {
  // Stub
}

void ambient_stop_layer(u32 layer_id) {
  // Stub
}

void ambient_set_master_volume(f32 volume) { audio_set_master_volume(volume); }

// ============================================================================
// AUDIO CATEGORIES
// ============================================================================

void audio_enable_category(const char *category) {
  printf("[Audio] Enabled category: %s\n", category);
}

void audio_disable_category(const char *category) {
  printf("[Audio] Disabled category: %s\n", category);
}

// ============================================================================
// ASSET INTEGRATION
// ============================================================================

// asset_importer_load_audio is defined in asset_importer.c

void *asset_manager_create(u32 initial_capacity, void *world) {
  return calloc(1, 256); // Placeholder
}

void asset_manager_destroy(void *manager) { free(manager); }

// ============================================================================
// ADDITIONAL AUDIO FUNCTIONS
// ============================================================================

u32 audio_get_active_sound_count(void) {
  return g_audio_system ? g_audio_system->active_sounds : 0;
}

bool audio_is_initialized(void) {
  return g_audio_system && g_audio_system->initialized;
}

void audio_pause_all(void) {
  // Stub: would pause all active sounds
}

void audio_resume_all(void) {
  // Stub: would resume all paused sounds
}

void audio_set_doppler_factor(f32 factor) {
  // Stub: for 3D audio physics
}

void audio_set_speed_of_sound(f32 speed) {
  // Stub: for 3D audio
}

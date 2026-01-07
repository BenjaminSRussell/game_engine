// include/audio/audio_system.h
//
// Purpose: Defines the public API and data structures for the game's audio
// system. This header provides enumerations for various sound types and
// categories, structures for managing individual sound sources and the overall
// audio engine state, and function declarations for initializing, updating, and
// controlling audio playback.
//
// Public APIs:
// - `audio_system_init`, `audio_system_free`: For lifecycle management of the
// audio system.
// - `audio_system_update`, `audio_update_listener`: For per-frame audio
// processing and listener positioning.
// - `audio_play_sound`, `audio_play_sound_2d`, `audio_stop_sound`: For playing
// and stopping sounds.
// - `audio_set_volume`, `audio_set_master_volume`, `audio_enable_category`,
// `audio_disable_category`: For volume and category control.
// - `audio_load_sound_buffer`: For pre-loading sound assets.
// - Functions for managing directional sound cones and reverb zones.
//
// Ownership: Manages `ma_engine` and `ma_device` instances from the miniaudio
// library, as well as a pool of `SoundSource` objects and pre-loaded
// `SoundBuffer`s.
//
// Invariants:
// - `AudioSystem` must be initialized with `audio_system_init` before use and
// freed with `audio_system_free`.
// - Sound `channel` and `zone_index` parameters in API calls must be valid for
// existing sound sources or reverb zones.
// - `miniaudio.h` is included for underlying audio functionality.
//
#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include <common.h>
#include <math/vec3.h>

// Forward declarations
struct PhysicsWorld;

#undef PI
#include <vendor/miniaudio.h>
#ifndef PI
#define PI 3.14159265358979323846f
#endif

// Audio reverb effect
#include "include/audio/audio_reverb.h"

typedef enum {
  SOUND_FOOTSTEP_GRASS,
  SOUND_FOOTSTEP_STONE,
  SOUND_FOOTSTEP_WOOD,
  SOUND_FOOTSTEP_SAND,
  SOUND_FOOTSTEP_GRAVEL,
  SOUND_FOOTSTEP_SNOW,
  SOUND_BLOCK_BREAK,
  SOUND_BLOCK_PLACE,
  SOUND_SWORD_SWING,
  SOUND_SWORD_HIT,
  SOUND_BOW_DRAW,
  SOUND_BOW_FIRE,
  SOUND_ITEM_PICKUP,
  SOUND_ITEM_DROP,
  SOUND_MOB_ZOMBIE_GROAN,
  SOUND_MOB_SKELETON_RATTLE,
  SOUND_MOB_CREEPER_HISS,
  SOUND_MOB_SPIDER_SKITTER,
  SOUND_MOB_COW_MOO,
  SOUND_MOB_PIG_OINK,
  SOUND_WATER_SPLASH,
  SOUND_FIRE_BURN,
  SOUND_AMBIENT_WIND,
  SOUND_AMBIENT_CAVE,
  SOUND_MUSIC_CALM,
  SOUND_MUSIC_ACTION,
  // Weather sounds
  SOUND_RAIN_LIGHT,
  SOUND_RAIN_MODERATE,
  SOUND_RAIN_HEAVY,
  SOUND_WIND_LIGHT,
  SOUND_WIND_MODERATE,
  SOUND_WIND_HEAVY,
  SOUND_THUNDER_01,
  SOUND_THUNDER_02,
  SOUND_SNOW_LIGHT,
  SOUND_SNOW_MODERATE,
  SOUND_SNOW_HEAVY,
  SOUND_FOG_LIGHT,
  SOUND_FOG_MODERATE,
  SOUND_FOG_HEAVY,
  SOUND_PLAYER_HURT,
  SOUND_PLAYER_HEAL,
  SOUND_EXPLOSION,
  SOUND_CRAFTING_SUCCESS,
  SOUND_COUNT
} SoundType;

typedef enum {
  SOUND_CATEGORY_MASTER,
  SOUND_CATEGORY_MUSIC,
  SOUND_CATEGORY_AMBIENT,
  SOUND_CATEGORY_WEATHER,
  SOUND_CATEGORY_BLOCK,
  SOUND_CATEGORY_HOSTILE,
  SOUND_CATEGORY_NEUTRAL,
  SOUND_CATEGORY_PLAYER,
  SOUND_CATEGORY_RECORD
} SoundCategory;

typedef struct {
  ma_sound sound; // miniaudio sound instance
  SoundType sound_type;
  Vec3 position;
  Vec3 velocity;  // For Doppler shift
  Vec3 direction; // Direction for cone attenuation (unit vector)
  f32 volume;
  f32 pitch;
  bool looping;
  bool active; // Whether this source is currently playing
  SoundCategory category;
  f32 min_distance;   // Reference distance for attenuation
  f32 max_distance;   // Maximum audible distance
  f32 rolloff;        // Distance attenuation factor
  f32 doppler_factor; // Strength of Doppler effect

  // Directional cone parameters
  f32 cone_inner_angle; // Inner cone angle in radians (full volume)
  f32 cone_outer_angle; // Outer cone angle in radians
  f32 cone_outer_gain;  // Volume multiplier outside cone

  // Occlusion state
  f32 occlusion_factor; // 0.0 = fully occluded, 1.0 = no occlusion
  f32 target_occlusion; // Target occlusion for smooth transitions
} SoundSource;

typedef struct {
  ma_decoder decoder;
  void *data;
  size_t data_size;
  bool loaded;
  char filepath[256];
} SoundBuffer;

// Reverb zone definition
typedef struct {
  Vec3 min_bounds;  // Minimum corner of reverb zone
  Vec3 max_bounds;  // Maximum corner of reverb zone
  f32 reverb_level; // Reverb intensity (0.0 to 1.0)
  f32 decay_time;   // Reverb decay time in seconds
  bool active;      // Whether this zone is active
} ReverbZone;

typedef struct AudioSystem {
  u32 max_channels;
  u32 active_sources;
  f32 master_volume;
  f32 category_volumes[SOUND_CATEGORY_RECORD + 1];
  SoundSource *sources;
  bool initialized;

  // miniaudio backend
  ma_engine engine;
  ma_device device;

  // 3D listener state
  Vec3 listener_position;
  Vec3 listener_forward;
  Vec3 listener_up;
  Vec3 listener_velocity;

  // Sound buffer cache
  SoundBuffer sound_buffers[SOUND_COUNT]; // Pre-loaded sound buffers

  // Reverb effect
  AudioReverb *reverb_effect;

  // Reverb zones
  ReverbZone reverb_zones[8]; // Support up to 8 reverb zones
  u32 reverb_zone_count;
} AudioSystem;

typedef struct PhysicsWorld PhysicsWorld;

void audio_system_init(AudioSystem *sys, u32 max_channels);
void audio_system_free(AudioSystem *sys);

void audio_system_update(AudioSystem *sys, f32 delta_time);
void audio_update_listener(AudioSystem *sys, Vec3 position, Vec3 forward,
                           Vec3 up, Vec3 velocity);

u32 audio_play_sound(AudioSystem *sys, SoundType sound, Vec3 position,
                     f32 volume, SoundCategory category);
u32 audio_play_sound_2d(AudioSystem *sys, SoundType sound, f32 volume,
                        SoundCategory category);

void audio_stop_sound(AudioSystem *sys, u32 channel);
void audio_set_volume(AudioSystem *sys, SoundCategory category, f32 volume);
void audio_set_sound_volume(AudioSystem *sys, u32 channel, f32 volume);
u32 audio_play_sound_ambient(AudioSystem *sys, SoundType sound, f32 volume);
void audio_set_master_volume(AudioSystem *sys, f32 volume);

void audio_enable_category(AudioSystem *sys, SoundCategory category);
void audio_disable_category(AudioSystem *sys, SoundCategory category);

// Sound buffer loading
bool audio_load_sound_buffer(AudioSystem *sys, SoundType sound,
                             const char *filepath);
void audio_load_all_sounds(AudioSystem *sys);
const char *audio_get_sound_path(SoundType sound);

// #include "../weather/weather.h"
typedef struct WeatherSystem WeatherSystem;
void audio_update_weather_sounds(AudioSystem *sys, WeatherSystem *weather,
                                 f32 delta_time);

// Directional cone control
void audio_set_sound_cone(AudioSystem *sys, u32 channel, f32 inner_angle,
                          f32 outer_angle, f32 outer_gain);
void audio_set_sound_direction(AudioSystem *sys, u32 channel, Vec3 direction);

// Reverb zone management
u32 audio_add_reverb_zone(AudioSystem *sys, Vec3 min_bounds, Vec3 max_bounds,
                          f32 reverb_level, f32 decay_time);
void audio_remove_reverb_zone(AudioSystem *sys, u32 zone_index);
void audio_update_reverb_zones(AudioSystem *sys, PhysicsWorld *physics);

void Audio_SendToOutput(const float* samples, size_t sample_count);

#endif

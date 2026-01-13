// Audio system integration - connects game systems to audio engine
// Provides a unified interface for all audio playback from game systems
#ifndef AUDIO_INTEGRATION_H
#define AUDIO_INTEGRATION_H

#include "engine/include/common.h"
#include <math/vec3.h>

// Forward declarations
typedef struct AudioSystem AudioSystem;
typedef struct WeatherAudioSystem WeatherAudioSystem;
typedef struct WaterSystem WaterSystem;

// Audio channel types
typedef enum {
  AUDIO_CHANNEL_MUSIC = 0,
  AUDIO_CHANNEL_AMBIENT = 1,
  AUDIO_CHANNEL_WEATHER = 2,
  AUDIO_CHANNEL_WATER = 3,
  AUDIO_CHANNEL_CREATURE = 4,
  AUDIO_CHANNEL_UI = 5,
  AUDIO_CHANNEL_VOICE = 6,
  AUDIO_CHANNEL_3D_WORLD = 7,
  AUDIO_CHANNEL_COUNT = 8
} AudioChannelType;

// Audio source types
typedef enum {
  AUDIO_SOURCE_RAIN = 0,
  AUDIO_SOURCE_WIND = 1,
  AUDIO_SOURCE_THUNDER = 2,
  AUDIO_SOURCE_WATER_FLOW = 3,
  AUDIO_SOURCE_SPLASH = 4,
  AUDIO_SOURCE_BUBBLE = 5,
  AUDIO_SOURCE_FOOTSTEP = 6,
  AUDIO_SOURCE_CREATURE = 7,
  AUDIO_SOURCE_UI = 8,
  AUDIO_SOURCE_AMBIENCE = 9,
  AUDIO_SOURCE_COUNT = 10
} AudioSourceType;

// Audio source handle
typedef u32 AudioSourceHandle;

// 3D audio properties
typedef struct {
  Vec3 position;
  Vec3 velocity;
  f32 distance_scale;
  f32 doppler_scale;
  bool loop;
  f32 volume;
  f32 pitch;
} Audio3DProperties;

// Audio integration system
typedef struct {
  AudioSystem *audio_engine;
  WeatherAudioSystem *weather_audio;
  WaterSystem *water_system;

  // Active audio sources
  AudioSourceHandle active_sources[AUDIO_SOURCE_COUNT];
  Audio3DProperties source_properties[AUDIO_SOURCE_COUNT];

  // Master volumes per channel
  f32 channel_volumes[AUDIO_CHANNEL_COUNT];

  // Configuration
  bool enable_3d_audio;
  bool enable_doppler;
  f32 global_volume;
  f32 listener_distance_scale;

  bool initialized;
} AudioIntegration;

// Initialization and cleanup
void audio_integration_init(AudioIntegration *integration, AudioSystem *engine);
void audio_integration_free(AudioIntegration *integration);

// System connections
void audio_integration_connect_weather(AudioIntegration *integration,
                                      WeatherAudioSystem *weather_audio);
void audio_integration_connect_water(AudioIntegration *integration,
                                    WaterSystem *water_system);

// Audio playback control
AudioSourceHandle audio_integration_play_sound(AudioIntegration *integration,
                                              AudioSourceType type,
                                              const char *sound_name,
                                              AudioChannelType channel,
                                              f32 volume);

AudioSourceHandle audio_integration_play_3d_sound(AudioIntegration *integration,
                                                 AudioSourceType type,
                                                 const char *sound_name,
                                                 Vec3 position,
                                                 AudioChannelType channel,
                                                 f32 volume);

void audio_integration_stop_sound(AudioIntegration *integration,
                                 AudioSourceHandle handle);
void audio_integration_stop_all_sounds(AudioIntegration *integration,
                                      AudioChannelType channel);

// Volume control
void audio_integration_set_channel_volume(AudioIntegration *integration,
                                         AudioChannelType channel,
                                         f32 volume);
f32 audio_integration_get_channel_volume(AudioIntegration *integration,
                                        AudioChannelType channel);
void audio_integration_set_master_volume(AudioIntegration *integration,
                                        f32 volume);

// 3D audio positioning
void audio_integration_update_source_position(AudioIntegration *integration,
                                             AudioSourceHandle handle,
                                             Vec3 position);
void audio_integration_update_listener_position(AudioIntegration *integration,
                                               Vec3 position, Vec3 forward,
                                               Vec3 up);

// Audio properties
void audio_integration_set_source_properties(AudioIntegration *integration,
                                            AudioSourceHandle handle,
                                            const Audio3DProperties *props);

// Looping control
void audio_integration_set_loop(AudioIntegration *integration,
                               AudioSourceHandle handle, bool loop);
bool audio_integration_is_playing(AudioIntegration *integration,
                                 AudioSourceHandle handle);

// Utility functions
const char *audio_source_type_get_name(AudioSourceType type);
const char *audio_channel_type_get_name(AudioChannelType channel);

// Audio streaming
void audio_integration_update(AudioIntegration *integration, f32 delta_time);

// Statistics
typedef struct {
  u32 active_sources;
  u32 playing_sounds;
  u32 looped_sounds;
  f32 cpu_usage;
  f32 memory_usage;
} AudioIntegrationStats;

AudioIntegrationStats audio_integration_get_stats(
    const AudioIntegration *integration);

#endif // AUDIO_INTEGRATION_H

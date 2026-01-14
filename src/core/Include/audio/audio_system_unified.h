// include/audio/audio_system_unified.h
//
// Purpose: Unified audio system consolidating all audio subsystems
// This replaces multiple disparate audio APIs with a single interface

#ifndef AUDIO_SYSTEM_UNIFIED_H
#define AUDIO_SYSTEM_UNIFIED_H

#include <common.h>
#include "math/vec3.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// UNIFIED AUDIO CONFIGURATION
// ============================================================================

typedef enum {
    AUDIO_FORMAT_MONO8,
    AUDIO_FORMAT_MONO16,
    AUDIO_FORMAT_STEREO8,
    AUDIO_FORMAT_STEREO16,
    AUDIO_FORMAT_5POINT1_16,
    AUDIO_FORMAT_7POINT1_16
} AudioFormat;

typedef enum {
    AUDIO_SAMPLE_RATE_8KHZ = 8000,
    AUDIO_SAMPLE_RATE_11KHZ = 11025,
    AUDIO_SAMPLE_RATE_16KHZ = 16000,
    AUDIO_SAMPLE_RATE_22KHZ = 22050,
    AUDIO_SAMPLE_RATE_44KHZ = 44100,
    AUDIO_SAMPLE_RATE_48KHZ = 48000,
    AUDIO_SAMPLE_RATE_96KHZ = 96000,
    AUDIO_SAMPLE_RATE_192KHZ = 192000
} AudioSampleRate;

typedef enum {
    AUDIO_EFFECT_REVERB,
    AUDIO_EFFECT_ECHO,
    AUDIO_EFFECT_CHORUS,
    AUDIO_EFFECT_DISTORTION,
    AUDIO_EFFECT_COMPRESSOR,
    AUDIO_EFFECT_EQUALIZER,
    AUDIO_EFFECT_PITCH_SHIFT,
    AUDIO_EFFECT_TIME_STRETCH
} AudioEffectType;

// ============================================================================
// UNIFIED AUDIO DATA STRUCTURES
// ============================================================================

typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 forward;
    Vec3 up;
} AudioListener3D;

typedef struct {
    Vec3 position;
    Vec3 velocity;
    float cone_inner_angle;
    float cone_outer_angle;
    float cone_outer_gain;
    float max_distance;
    float reference_distance;
    float rolloff_factor;
    bool loop;
    bool relative;
} AudioSource3D;

typedef struct {
    uint32_t id;
    AudioFormat format;
    AudioSampleRate sample_rate;
    uint32_t frequency;
    uint32_t size;
    uint8_t* data;
    bool is_compressed;
    uint32_t channels;
    uint32_t bits_per_sample;
} AudioBuffer;

typedef struct {
    uint32_t id;
    uint32_t buffer_id;
    AudioSource3D settings_3d;
    float gain;
    float pitch;
    bool is_playing;
    bool is_paused;
    bool is_looping;
    float position;
    float duration;
    int32_t priority;
    void* user_data;
} AudioSource;

typedef struct {
    AudioEffectType type;
    float wet_level;
    float dry_level;
    float feedback;
    float delay;
    float frequency;
    float q_factor;
    float gain;
    bool enabled;
    void* effect_data;
} AudioEffect;

typedef struct {
    float master_volume;
    float music_volume;
    float sfx_volume;
    float voice_volume;
    float ambient_volume;
    bool enable_3d_audio;
    bool enable_doppler;
    bool enable_hrtf;
    uint32_t max_voices;
    uint32_t active_voices;
} AudioMixerSettings;

typedef struct {
    AudioListener3D listener;
    AudioMixerSettings mixer;
    AudioSource* sources;
    uint32_t source_count;
    uint32_t max_sources;
    AudioBuffer* buffers;
    uint32_t buffer_count;
    uint32_t max_buffers;
    AudioEffect* effects;
    uint32_t effect_count;
    uint32_t max_effects;
    float global_time_scale;
    bool is_initialized;
} AudioWorld;

// ============================================================================
// MAIN AUDIO SYSTEM API
// ============================================================================

// System management
AudioWorld* audio_system_create(uint32_t max_sources, uint32_t max_buffers, uint32_t max_effects);
void audio_system_destroy(AudioWorld* world);
void audio_system_update(AudioWorld* world, float delta_time);
bool audio_system_is_initialized(AudioWorld* world);

// Core audio functions
void audio_system_init(void);
void audio_system_shutdown(void);
void audio_system_set_master_volume(float volume);
float audio_system_get_master_volume(void);
void audio_system_set_sfx_volume(float volume);
float audio_system_get_sfx_volume(void);
void audio_system_set_music_volume(float volume);
float audio_system_get_music_volume(void);
void audio_system_play_sfx(const char* sound_name, float volume);
void audio_system_play_music(const char* music_name, float volume);
void audio_system_stop_music(void);
void audio_system_pause_all(void);
void audio_system_resume_all(void);

// ============================================================================
// 3D AUDIO API
// ============================================================================

// Listener management
void audio_listener_set_position(AudioWorld* world, Vec3 position);
void audio_listener_set_velocity(AudioWorld* world, Vec3 velocity);
void audio_listener_set_orientation(AudioWorld* world, Vec3 forward, Vec3 up);
void audio_listener_get_position(AudioWorld* world, Vec3* position);

// Source management
uint32_t audio_source_create(AudioWorld* world);
void audio_source_destroy(AudioWorld* world, uint32_t source_id);
void audio_source_set_buffer(AudioWorld* world, uint32_t source_id, uint32_t buffer_id);
void audio_source_play(AudioWorld* world, uint32_t source_id);
void audio_source_pause(AudioWorld* world, uint32_t source_id);
void audio_source_stop(AudioWorld* world, uint32_t source_id);
void audio_source_set_gain(AudioWorld* world, uint32_t source_id, float gain);
void audio_source_set_pitch(AudioWorld* world, uint32_t source_id, float pitch);
void audio_source_set_position(AudioWorld* world, uint32_t source_id, Vec3 position);
void audio_source_set_3d_settings(AudioWorld* world, uint32_t source_id, const AudioSource3D* settings);

// ============================================================================
// BUFFER MANAGEMENT API
// ============================================================================

uint32_t audio_buffer_create(AudioWorld* world, const void* data, uint32_t size, 
                           AudioFormat format, AudioSampleRate sample_rate);
void audio_buffer_destroy(AudioWorld* world, uint32_t buffer_id);
uint32_t audio_buffer_load_from_file(AudioWorld* world, const char* filename);
AudioBuffer* audio_buffer_get(AudioWorld* world, uint32_t buffer_id);

// ============================================================================
// EFFECTS API
// ============================================================================

uint32_t audio_effect_create(AudioWorld* world, AudioEffectType type);
void audio_effect_destroy(AudioWorld* world, uint32_t effect_id);
void audio_effect_attach_to_source(AudioWorld* world, uint32_t effect_id, uint32_t source_id);
void audio_effect_set_enabled(AudioWorld* world, uint32_t effect_id, bool enabled);
void audio_effect_set_parameters(AudioWorld* world, uint32_t effect_id, float wet_level, 
                              float dry_level, float feedback, float delay);

// ============================================================================
// MIXER API
// ============================================================================

void audio_mixer_set_master_volume(AudioWorld* world, float volume);
void audio_mixer_set_music_volume(AudioWorld* world, float volume);
void audio_mixer_set_sfx_volume(AudioWorld* world, float volume);
void audio_mixer_get_settings(AudioWorld* world, AudioMixerSettings* settings);

// ============================================================================
// DSP UTILITIES
// ============================================================================

float audio_db_to_linear(float db);
float audio_linear_to_db(float linear);
float audio_milliseconds_to_samples(float milliseconds, AudioSampleRate sample_rate);
float audio_samples_to_milliseconds(uint32_t samples, AudioSampleRate sample_rate);
void audio_calculate_direction(const Vec3* from, const Vec3* to, Vec3* direction);
float audio_calculate_distance(const Vec3* pos1, const Vec3* pos2);

// ============================================================================
// STATISTICS
// ============================================================================

typedef struct {
    uint32_t active_sources;
    uint32_t active_buffers;
    uint32_t active_effects;
    float cpu_usage;
    uint64_t memory_usage;
} AudioStats;

AudioStats audio_system_get_stats(AudioWorld* world);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_SYSTEM_UNIFIED_H

#ifndef UNIFIED_AUDIO_SYSTEM_H
#define UNIFIED_AUDIO_SYSTEM_H

/*
 * Unified Audio System
 * Consolidates all audio functionality including DSP, spatial audio, streaming
 * Fixes header issues and provides standardized audio interfaces
 */

#include "unified_memory.h"
#include "unified_logging.h"
#include "error_handling.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * AUDIO FORMATS AND CONFIGURATION
 * ============================================================================ */

typedef enum {
    AUDIO_FORMAT_MONO8 = 0,
    AUDIO_FORMAT_MONO16,
    AUDIO_FORMAT_STEREO8,
    AUDIO_FORMAT_STEREO16,
    AUDIO_FORMAT_MONO32F,
    AUDIO_FORMAT_STEREO32F,
    AUDIO_FORMAT_5POINT1_16,
    AUDIO_FORMAT_5POINT1_32F,
    AUDIO_FORMAT_7POINT1_16,
    AUDIO_FORMAT_7POINT1_32F,
    AUDIO_FORMAT_COUNT
} audio_format_t;

typedef enum {
    AUDIO_BACKEND_OPENAL = 0,
    AUDIO_BACKEND_XAUDIO2,
    AUDIO_BACKEND_CORE_AUDIO,
    AUDIO_BACKEND_ALSA,
    AUDIO_BACKEND_PULSEAUDIO,
    AUDIO_BACKEND_WASAPI,
    AUDIO_BACKEND_COUNT
} audio_backend_t;

typedef struct {
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
    uint32_t buffer_size;
    uint32_t max_sounds;
    uint32_t max_sources;
    float master_volume;
    float music_volume;
    float sfx_volume;
    float voice_volume;
    bool enable_3d_audio;
    bool enable_streaming;
    bool enable_dsp;
    bool enable_compression;
    float max_distance;
    float reference_distance;
    float rolloff_factor;
    audio_backend_t preferred_backend;
    uint32_t streaming_buffer_size;
    uint32_t max_concurrent_streams;
} audio_config_t;

/* ============================================================================
 * AUDIO BUFFERS AND SOURCES
 * ============================================================================ */

typedef struct {
    void* data;
    uint32_t size;
    audio_format_t format;
    uint32_t sample_rate;
    uint32_t channels;
    uint32_t duration_ms;
    bool streamed;
    char* file_path;
    uint32_t id;
} audio_buffer_t;

typedef struct {
    uint32_t buffer_id;
    float position[3];
    float velocity[3];
    float direction[3];
    float gain;
    float pitch;
    bool loop;
    bool relative;
    bool playing;
    bool paused;
    float cone_inner_angle;
    float cone_outer_angle;
    float cone_outer_gain;
    float min_gain;
    float max_gain;
    float min_distance;
    float max_distance;
    float rolloff_factor;
    uint32_t id;
} audio_source_t;

/* ============================================================================
 * DSP EFFECTS AND PROCESSING
 * ============================================================================ */

typedef enum {
    DSP_EFFECT_NONE = 0,
    DSP_EFFECT_REVERB = (1 << 0),
    DSP_EFFECT_ECHO = (1 << 1),
    DSP_EFFECT_DELAY = (1 << 2),
    DSP_EFFECT_CHORUS = (1 << 3),
    DSP_EFFECT_FLANGER = (1 << 4),
    DSP_EFFECT_PHASER = (1 << 5),
    DSP_EFFECT_DISTORTION = (1 << 6),
    DSP_EFFECT_COMPRESSOR = (1 << 7),
    DSP_EFFECT_LIMITER = (1 << 8),
    DSP_EFFECT_EXPANDER = (1 << 9),
    DSP_EFFECT_GATE = (1 << 10),
    DSP_EFFECT_EQUALIZER = (1 << 11),
    DSP_EFFECT_FILTER = (1 << 12),
    DSP_EFFECT_PITCH_SHIFT = (1 << 13),
    DSP_EFFECT_TIME_STRETCH = (1 << 14),
    DSP_EFFECT_COUNT
} dsp_effect_flags_t;

typedef struct {
    dsp_effect_flags_t enabled_effects;
    
    // Reverb parameters
    float reverb_decay_time;
    float reverb_wet_level;
    float reverb_dry_level;
    float reverb_room_size;
    float reverb_damping;
    float reverb_width;
    
    // Echo/Delay parameters
    float echo_delay_time;
    float echo_feedback;
    float echo_wet_level;
    
    // Chorus parameters
    float chorus_rate;
    float chorus_depth;
    float chorus_feedback;
    float chorus_wet_level;
    
    // Filter parameters
    float filter_cutoff;
    float filter_resonance;
    float filter_type;  // 0=lowpass, 1=highpass, 2=bandpass
    
    // Compressor parameters
    float compressor_threshold;
    float compressor_ratio;
    float compressor_attack;
    float compressor_release;
    float compressor_makeup_gain;
    
    // Equalizer parameters
    float eq_bands[10];  // 10-band equalizer
    float eq_frequencies[10];
    
} dsp_effects_t;

/* ============================================================================
 * SPATIAL AUDIO SYSTEM
 * ============================================================================ */

typedef enum {
    SPATIAL_MODE_STEREO = 0,
    SPATIAL_MODE_HRTF,
    SPATIAL_MODE_VBAP,
    SPATIAL_MODE_AMBISONICS,
    SPATIAL_MODE_COUNT
} spatial_audio_mode_t;

typedef struct {
    spatial_audio_mode_t mode;
    float listener_position[3];
    float listener_velocity[3];
    float listener_forward[3];
    float listener_up[3];
    float listener_orientation[4];  // Quaternion
    
    // HRTF data
    void* hrtf_data;
    uint32_t hrtf_sample_rate;
    uint32_t hrtf_ir_length;
    
    // VBAP speaker setup
    float speaker_positions[32][3];  // Up to 32 speakers
    uint32_t speaker_count;
    float speaker_gains[32];
    
} spatial_audio_t;

/* ============================================================================
 * AUDIO STREAMING SYSTEM
 * ============================================================================ */

typedef struct {
    FILE* file_handle;
    uint32_t file_size;
    uint32_t file_position;
    uint32_t buffer_size;
    uint8_t* read_buffer;
    uint8_t* decode_buffer;
    bool eof_reached;
    bool loop;
    char* file_path;
    audio_format_t format;
    uint32_t sample_rate;
    uint32_t channels;
} audio_stream_t;

typedef struct {
    audio_stream_t* streams;
    uint32_t stream_count;
    uint32_t capacity;
    uint32_t thread_count;
    void* thread_pool;
    bool streaming_enabled;
} audio_streaming_system_t;

/* ============================================================================
 * UNIFIED AUDIO SYSTEM
 * ============================================================================ */

typedef struct {
    // Configuration
    audio_config_t config;
    audio_backend_t backend;
    
    // Audio buffers and sources
    audio_buffer_t* buffers;
    uint32_t buffer_count;
    uint32_t buffer_capacity;
    
    audio_source_t* sources;
    uint32_t source_count;
    uint32_t source_capacity;
    
    // DSP processing
    dsp_effects_t global_effects;
    dsp_effects_t* source_effects;  // Per-source effects
    bool dsp_enabled;
    
    // Spatial audio
    spatial_audio_t spatial_audio;
    
    // Streaming system
    audio_streaming_system_t streaming;
    
    // Backend-specific data
    void* backend_data;
    
    // State
    bool initialized;
    bool suspended;
    uint32_t next_buffer_id;
    uint32_t next_source_id;
    
    // Statistics
    uint32_t playing_sources;
    uint32_t total_buffers_loaded;
    uint32_t total_streams_active;
    float cpu_usage;
    float memory_usage_mb;
    
} unified_audio_system_t;

/* ============================================================================
 * UNIFIED AUDIO SYSTEM API
 * ============================================================================ */

// Lifecycle management
unified_audio_system_t* unified_audio_system_create(const audio_config_t* config);
void unified_audio_system_destroy(unified_audio_system_t* audio);
bool unified_audio_system_initialize(unified_audio_system_t* audio);
void unified_audio_system_shutdown(unified_audio_system_t* audio);

// Configuration
void unified_audio_system_set_config(unified_audio_system_t* audio, const audio_config_t* config);
void unified_audio_system_get_config(unified_audio_system_t* audio, audio_config_t* config);
bool unified_audio_system_set_backend(unified_audio_system_t* audio, audio_backend_t backend);
audio_backend_t unified_audio_system_get_backend(unified_audio_system_t* audio);

// Volume control
void unified_audio_system_set_master_volume(unified_audio_system_t* audio, float volume);
void unified_audio_system_set_music_volume(unified_audio_system_t* audio, float volume);
void unified_audio_system_set_sfx_volume(unified_audio_system_t* audio, float volume);
void unified_audio_system_set_voice_volume(unified_audio_system_t* audio, float volume);
float unified_audio_system_get_master_volume(unified_audio_system_t* audio);

/* ============================================================================
 * AUDIO BUFFER MANAGEMENT API
 * ============================================================================ */

// Buffer loading and management
uint32_t unified_audio_load_buffer(unified_audio_system_t* audio, const char* file_path);
uint32_t unified_audio_load_buffer_from_memory(unified_audio_system_t* audio, 
                                             const void* data, uint32_t size,
                                             audio_format_t format, uint32_t sample_rate);
uint32_t unified_audio_create_streaming_buffer(unified_audio_system_t* audio, const char* file_path);
void unified_audio_unload_buffer(unified_audio_system_t* audio, uint32_t buffer_id);

audio_buffer_t* unified_audio_get_buffer(unified_audio_system_t* audio, uint32_t buffer_id);
bool unified_audio_is_buffer_loaded(unified_audio_system_t* audio, uint32_t buffer_id);

// Buffer utilities
uint32_t unified_audio_get_buffer_duration(unified_audio_system_t* audio, uint32_t buffer_id);
audio_format_t unified_audio_get_buffer_format(unified_audio_system_t* audio, uint32_t buffer_id);
uint32_t unified_audio_get_buffer_sample_rate(unified_audio_system_t* audio, uint32_t buffer_id);

/* ============================================================================
 * AUDIO SOURCE MANAGEMENT API
 * ============================================================================ */

// Source creation and control
uint32_t unified_audio_create_source(unified_audio_system_t* audio);
void unified_audio_destroy_source(unified_audio_system_t* audio, uint32_t source_id);
audio_source_t* unified_audio_get_source(unified_audio_system_t* audio, uint32_t source_id);

// Playback control
void unified_audio_play_source(unified_audio_system_t* audio, uint32_t source_id);
void unified_audio_pause_source(unified_audio_system_t* audio, uint32_t source_id);
void unified_audio_stop_source(unified_audio_system_t* audio, uint32_t source_id);
void unified_audio_resume_source(unified_audio_system_t* audio, uint32_t source_id);

bool unified_audio_is_source_playing(unified_audio_system_t* audio, uint32_t source_id);
bool unified_audio_is_source_paused(unified_audio_system_t* audio, uint32_t source_id);

// Source properties
void unified_audio_set_source_buffer(unified_audio_system_t* audio, uint32_t source_id, uint32_t buffer_id);
void unified_audio_set_source_position(unified_audio_system_t* audio, uint32_t source_id, 
                                     const float position[3]);
void unified_audio_set_source_velocity(unified_audio_system_t* audio, uint32_t source_id, 
                                     const float velocity[3]);
void unified_audio_set_source_direction(unified_audio_system_t* audio, uint32_t source_id, 
                                      const float direction[3]);
void unified_audio_set_source_gain(unified_audio_system_t* audio, uint32_t source_id, float gain);
void unified_audio_set_source_pitch(unified_audio_system_t* audio, uint32_t source_id, float pitch);
void unified_audio_set_source_loop(unified_audio_system_t* audio, uint32_t source_id, bool loop);

// 3D audio properties
void unified_audio_set_source_relative(unified_audio_system_t* audio, uint32_t source_id, bool relative);
void unified_audio_set_source_cone(unified_audio_system_t* audio, uint32_t source_id,
                                  float inner_angle, float outer_angle, float outer_gain);
void unified_audio_set_source_distance_model(unified_audio_system_t* audio, uint32_t source_id,
                                           float min_distance, float max_distance, float rolloff);

/* ============================================================================
 * DSP EFFECTS API
 * ============================================================================ */

// Global DSP effects
void unified_audio_enable_dsp(unified_audio_system_t* audio, bool enabled);
void unified_audio_set_global_effects(unified_audio_system_t* audio, const dsp_effects_t* effects);
void unified_audio_get_global_effects(unified_audio_system_t* audio, dsp_effects_t* effects);

// Per-source DSP effects
void unified_audio_set_source_effects(unified_audio_system_t* audio, uint32_t source_id, 
                                     const dsp_effects_t* effects);
void unified_audio_get_source_effects(unified_audio_system_t* audio, uint32_t source_id, 
                                     dsp_effects_t* effects);

// Individual effect controls
void unified_audio_enable_effect(unified_audio_system_t* audio, dsp_effect_flags_t effect, bool enabled);
void unified_audio_set_reverb(unified_audio_system_t* audio, float decay_time, float wet_level, 
                             float dry_level, float room_size, float damping);
void unified_audio_set_echo(unified_audio_system_t* audio, float delay_time, float feedback, 
                           float wet_level);
void unified_audio_set_chorus(unified_audio_system_t* audio, float rate, float depth, 
                             float feedback, float wet_level);
void unified_audio_set_filter(unified_audio_system_t* audio, float cutoff, float resonance, 
                             uint32_t filter_type);
void unified_audio_set_compressor(unified_audio_system_t* audio, float threshold, float ratio, 
                                 float attack, float release, float makeup_gain);
void unified_audio_set_equalizer(unified_audio_system_t* audio, const float* bands, uint32_t band_count);

/* ============================================================================
 * SPATIAL AUDIO API
 * ============================================================================ */

// Spatial audio configuration
void unified_audio_set_spatial_mode(unified_audio_system_t* audio, spatial_audio_mode_t mode);
spatial_audio_mode_t unified_audio_get_spatial_mode(unified_audio_system_t* audio);

// Listener properties
void unified_audio_set_listener_position(unified_audio_system_t* audio, const float position[3]);
void unified_audio_set_listener_velocity(unified_audio_system_t* audio, const float velocity[3]);
void unified_audio_set_listener_orientation(unified_audio_system_t* audio, const float forward[3], 
                                          const float up[3]);
void unified_audio_set_listener_orientation_quat(unified_audio_system_t* audio, const float quat[4]);

// HRTF support
bool unified_audio_load_hrtf_dataset(unified_audio_system_t* audio, const char* dataset_path);
void unified_audio_enable_hrtf(unified_audio_system_t* audio, bool enabled);

// VBAP support
void unified_audio_set_vbap_speakers(unified_audio_system_t* audio, const float positions[][3], 
                                    uint32_t speaker_count);
void unified_audio_set_vbap_gains(unified_audio_system_t* audio, const float* gains, 
                                  uint32_t speaker_count);

/* ============================================================================
 * AUDIO STREAMING API
 * ============================================================================ */

// Stream management
uint32_t unified_audio_create_stream(unified_audio_system_t* audio, const char* file_path);
void unified_audio_destroy_stream(unified_audio_system_t* audio, uint32_t stream_id);
void unified_audio_play_stream(unified_audio_system_t* audio, uint32_t stream_id);
void unified_audio_pause_stream(unified_audio_system_t* audio, uint32_t stream_id);
void unified_audio_stop_stream(unified_audio_system_t* audio, uint32_t stream_id);

// Stream properties
void unified_audio_set_stream_loop(unified_audio_system_t* audio, uint32_t stream_id, bool loop);
void unified_audio_set_stream_volume(unified_audio_system_t* audio, uint32_t stream_id, float volume);
bool unified_audio_is_stream_playing(unified_audio_system_t* audio, uint32_t stream_id);

// Streaming system control
void unified_audio_enable_streaming(unified_audio_system_t* audio, bool enabled);
void unified_audio_set_streaming_buffer_size(unified_audio_system_t* audio, uint32_t buffer_size);
void unified_audio_set_max_concurrent_streams(unified_audio_system_t* audio, uint32_t max_streams);

/* ============================================================================
 * COMPATIBILITY LAYER - Legacy audio system integration
 * ============================================================================ */

// Legacy audio system types
typedef unified_audio_system_t legacy_audio_system_t;
typedef audio_buffer_t legacy_audio_buffer_t;
typedef audio_source_t legacy_audio_source_t;

// Legacy function mappings
#define legacy_audio_create unified_audio_system_create
#define legacy_audio_destroy unified_audio_system_destroy
#define legacy_audio_load_buffer unified_audio_load_buffer
#define legacy_audio_create_source unified_audio_create_source
#define legacy_audio_play_source unified_audio_play_source

// Migration helpers
void unified_audio_migrate_legacy_system(unified_audio_system_t* unified_system, void* legacy_system);
void unified_audio_import_legacy_config(audio_config_t* unified_config, const void* legacy_config);

/* ============================================================================
 * DEBUG AND PROFILING
 * ============================================================================ */

#ifdef DEBUG_BUILD
#define AUDIO_DEBUG_ENABLED 1
#else
#define AUDIO_DEBUG_ENABLED 0
#endif

#if AUDIO_DEBUG_ENABLED
void unified_audio_debug_print_stats(unified_audio_system_t* audio);
void unified_audio_debug_validate_sources(unified_audio_system_t* audio);
void unified_audio_debug_validate_buffers(unified_audio_system_t* audio);
#else
#define unified_audio_debug_print_stats(audio) ((void)0)
#define unified_audio_debug_validate_sources(audio) ((void)0)
#define unified_audio_debug_validate_buffers(audio) ((void)0)
#endif

// Performance monitoring
void unified_audio_get_performance_stats(unified_audio_system_t* audio, float* cpu_usage, 
                                        float* memory_usage, uint32_t* playing_sources);
void unified_audio_reset_performance_stats(unified_audio_system_t* audio);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

// Format conversion
uint32_t unified_audio_get_bytes_per_sample(audio_format_t format);
uint32_t unified_audio_get_channel_count(audio_format_t format);
bool unified_audio_is_format_supported(unified_audio_system_t* audio, audio_format_t format);

// Backend information
const char* unified_audio_get_backend_name(audio_backend_t backend);
const char* unified_audio_get_current_backend_name(unified_audio_system_t* audio);
const char* unified_audio_get_device_name(unified_audio_system_t* audio);

// Error handling
const char* unified_audio_get_error_string(unified_audio_system_t* audio);
bool unified_audio_check_error(unified_audio_system_t* audio);

/* ============================================================================
 * MACROS AND CONVENIENCE FUNCTIONS
 * ============================================================================ */

// Common audio operations
#define AUDIO_PLAY_SOUND(audio, buffer_id) \
    do { \
        uint32_t source_id = unified_audio_create_source(audio); \
        unified_audio_set_source_buffer(audio, source_id, buffer_id); \
        unified_audio_play_source(audio, source_id); \
    } while(0)

#define AUDIO_PLAY_3D_SOUND(audio, buffer_id, position) \
    do { \
        uint32_t source_id = unified_audio_create_source(audio); \
        unified_audio_set_source_buffer(audio, source_id, buffer_id); \
        unified_audio_set_source_position(audio, source_id, position); \
        unified_audio_play_source(audio, source_id); \
    } while(0)

#define AUDIO_PLAY_LOOPING_SOUND(audio, buffer_id) \
    do { \
        uint32_t source_id = unified_audio_create_source(audio); \
        unified_audio_set_source_buffer(audio, source_id, buffer_id); \
        unified_audio_set_source_loop(audio, source_id, true); \
        unified_audio_play_source(audio, source_id); \
    } while(0)

// Volume control macros
#define AUDIO_SET_MASTER_VOLUME(audio, volume) unified_audio_set_master_volume(audio, volume)
#define AUDIO_SET_MUSIC_VOLUME(audio, volume) unified_audio_set_music_volume(audio, volume)
#define AUDIO_SET_SFX_VOLUME(audio, volume) unified_audio_set_sfx_volume(audio, volume)

// DSP macros
#define AUDIO_ENABLE_REVERB(audio) unified_audio_enable_effect(audio, DSP_EFFECT_REVERB, true)
#define AUDIO_ENABLE_ECHO(audio) unified_audio_enable_effect(audio, DSP_EFFECT_ECHO, true)
#define AUDIO_ENABLE_CHORUS(audio) unified_audio_enable_effect(audio, DSP_EFFECT_CHORUS, true)

// Debug macros
#if AUDIO_DEBUG_ENABLED
#define AUDIO_DEBUG_STATS(audio) unified_audio_debug_print_stats(audio)
#define AUDIO_VALIDATE(audio) \
    do { \
        unified_audio_debug_validate_sources(audio); \
        unified_audio_debug_validate_buffers(audio); \
    } while(0)
#else
#define AUDIO_DEBUG_STATS(audio) ((void)0)
#define AUDIO_VALIDATE(audio) ((void)0)
#endif

/* ============================================================================
 * CONSTANTS AND LIMITS
 * ============================================================================ */

#define UNIFIED_AUDIO_MAX_BUFFERS 1024
#define UNIFIED_AUDIO_MAX_SOURCES 256
#define UNIFIED_AUDIO_MAX_STREAMS 32
#define UNIFIED_AUDIO_MAX_CHANNELS 8
#define UNIFIED_AUDIO_MAX_SAMPLE_RATE 192000
#define UNIFIED_AUDIO_MIN_SAMPLE_RATE 8000
#define UNIFIED_AUDIO_MAX_BUFFER_SIZE 1048576  // 1MB
#define UNIFIED_AUDIO_MIN_BUFFER_SIZE 64

#define UNIFIED_AUDIO_MIN_VOLUME 0.0f
#define UNIFIED_AUDIO_MAX_VOLUME 2.0f
#define UNIFIED_AUDIO_MIN_PITCH 0.5f
#define UNIFIED_AUDIO_MAX_PITCH 2.0f

#define UNIFIED_AUDIO_MIN_DISTANCE 0.1f
#define UNIFIED_AUDIO_MAX_DISTANCE 1000.0f
#define UNIFIED_AUDIO_MIN_ROLLOFF 0.0f
#define UNIFIED_AUDIO_MAX_ROLLOFF 10.0f

/* ============================================================================
 * ERROR CODES
 * ============================================================================ */

typedef enum {
    AUDIO_ERROR_NONE = 0,
    AUDIO_ERROR_INVALID_PARAMETER,
    AUDIO_ERROR_OUT_OF_MEMORY,
    AUDIO_ERROR_BACKEND_NOT_SUPPORTED,
    AUDIO_ERROR_DEVICE_NOT_FOUND,
    AUDIO_ERROR_FORMAT_NOT_SUPPORTED,
    AUDIO_ERROR_FILE_NOT_FOUND,
    AUDIO_ERROR_FILE_CORRUPTED,
    AUDIO_ERROR_STREAMING_FAILED,
    AUDIO_ERROR_DSP_FAILED,
    AUDIO_ERROR_CONTEXT_LOST,
    AUDIO_ERROR_COUNT
} audio_error_t;

const char* audio_error_string(audio_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* UNIFIED_AUDIO_SYSTEM_H */

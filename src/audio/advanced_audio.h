// Advanced Audio System
// Provides comprehensive 3D audio capabilities

#ifndef ADVANCED_AUDIO_H
#define ADVANCED_AUDIO_H

#include <stdbool.h>
#include <stdint.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

// Audio format types
typedef enum {
    AUDIO_FORMAT_MONO8,
    AUDIO_FORMAT_MONO16,
    AUDIO_FORMAT_STEREO8,
    AUDIO_FORMAT_STEREO16,
    AUDIO_FORMAT_5POINT1_16,
    AUDIO_FORMAT_7POINT1_16
} AudioFormat;

// Audio sample rate
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

// Audio 3D settings
typedef struct {
    float x, y, z;        // Position
    float vx, vy, vz;      // Velocity
    float forwardX, forwardY, forwardZ;  // Forward direction
    float upX, upY, upZ;   // Up direction
} AudioListener3D;

typedef struct {
    float x, y, z;        // Position
    float vx, vy, vz;      // Velocity
    float coneInnerAngle;   // Inner cone angle (radians)
    float coneOuterAngle;   // Outer cone angle (radians)
    float coneOuterGain;    // Gain outside outer cone
    float maxDistance;      // Maximum distance
    float referenceDistance; // Reference distance
    float rolloffFactor;    // Rolloff factor
    bool loop;             // Loop the sound
    bool relative;          // Relative to listener
} AudioSource3D;

// Audio buffer
typedef struct {
    uint32_t id;
    AudioFormat format;
    AudioSampleRate sampleRate;
    uint32_t frequency;
    uint32_t size;
    uint8_t* data;
    bool isCompressed;
    uint32_t channels;
    uint32_t bitsPerSample;
} AudioBuffer;

// Audio source
typedef struct {
    uint32_t id;
    uint32_t bufferId;
    AudioSource3D settings3D;
    float gain;
    float pitch;
    bool isPlaying;
    bool isPaused;
    bool isLooping;
    float position;        // Current position in seconds
    float duration;         // Duration in seconds
    int32_t priority;      // Priority for voice management
    void* userData;        // User data
} AudioSource;

// Audio effect types
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

// Audio effect
typedef struct {
    AudioEffectType type;
    float wetLevel;        // Wet signal level (0.0 - 1.0)
    float dryLevel;        // Dry signal level (0.0 - 1.0)
    float feedback;        // Feedback level
    float delay;           // Delay time in seconds
    float frequency;       // Frequency for filters
    float qFactor;         // Q factor for filters
    float gain;           // Gain in dB
    bool enabled;
    void* effectData;     // Platform-specific effect data
} AudioEffect;

// Audio mixer settings
typedef struct {
    float masterVolume;
    float musicVolume;
    float sfxVolume;
    float voiceVolume;
    float ambientVolume;
    bool enable3DAudio;
    bool enableDoppler;
    bool enableHRTF;      // Head-related transfer function
    uint32_t maxVoices;   // Maximum simultaneous voices
    uint32_t activeVoices; // Currently active voices
} AudioMixerSettings;

// Audio streaming
typedef struct {
    uint32_t id;
    AudioFormat format;
    AudioSampleRate sampleRate;
    uint32_t channels;
    uint32_t bufferSize;
    uint8_t* buffers[4];  // Double buffering
    uint32_t currentBuffer;
    bool isStreaming;
    bool isLooping;
    void* fileHandle;      // File handle for streaming
    uint64_t fileSize;
    uint64_t currentPos;
} AudioStream;

// Audio world
typedef struct {
    AudioListener3D listener;
    AudioMixerSettings mixer;
    AudioSource* sources;
    uint32_t sourceCount;
    uint32_t maxSources;
    AudioBuffer* buffers;
    uint32_t bufferCount;
    uint32_t maxBuffers;
    AudioEffect* effects;
    uint32_t effectCount;
    uint32_t maxEffects;
    AudioStream* streams;
    uint32_t streamCount;
    uint32_t maxStreams;
    float globalTimeScale;
    bool isInitialized;
} AudioWorld;

// Function declarations
AudioWorld* audio_world_create(uint32_t maxSources, uint32_t maxBuffers, uint32_t maxEffects);
void audio_world_destroy(AudioWorld* world);
void audio_world_update(AudioWorld* world, float deltaTime);
bool audio_world_is_initialized(AudioWorld* world);

// Listener management
void audio_listener_set_position(AudioWorld* world, float x, float y, float z);
void audio_listener_set_velocity(AudioWorld* world, float vx, float vy, float vz);
void audio_listener_set_orientation(AudioWorld* world, float forwardX, float forwardY, float forwardZ, float upX, float upY, float upZ);
void audio_listener_get_position(AudioWorld* world, float* x, float* y, float* z);

// Buffer management
uint32_t audio_buffer_create(AudioWorld* world, const void* data, uint32_t size, AudioFormat format, AudioSampleRate sampleRate);
void audio_buffer_destroy(AudioWorld* world, uint32_t bufferId);
AudioBuffer* audio_buffer_get(AudioWorld* world, uint32_t bufferId);
uint32_t audio_buffer_load_from_file(AudioWorld* world, const char* filename);
uint32_t audio_buffer_load_from_memory(AudioWorld* world, const void* data, uint32_t size);

// Source management
uint32_t audio_source_create(AudioWorld* world);
void audio_source_destroy(AudioWorld* world, uint32_t sourceId);
AudioSource* audio_source_get(AudioWorld* world, uint32_t sourceId);
void audio_source_set_buffer(AudioWorld* world, uint32_t sourceId, uint32_t bufferId);
void audio_source_play(AudioWorld* world, uint32_t sourceId);
void audio_source_pause(AudioWorld* world, uint32_t sourceId);
void audio_source_stop(AudioWorld* world, uint32_t sourceId);
void audio_source_rewind(AudioWorld* world, uint32_t sourceId);
bool audio_source_is_playing(AudioWorld* world, uint32_t sourceId);
void audio_source_set_gain(AudioWorld* world, uint32_t sourceId, float gain);
void audio_source_set_pitch(AudioWorld* world, uint32_t sourceId, float pitch);
void audio_source_set_looping(AudioWorld* world, uint32_t sourceId, bool looping);
void audio_source_set_position(AudioWorld* world, uint32_t sourceId, float x, float y, float z);
void audio_source_set_velocity(AudioWorld* world, uint32_t sourceId, float vx, float vy, float vz);
void audio_source_set_direction(AudioWorld* world, uint32_t sourceId, float x, float y, float z);
void audio_source_set_cone(AudioWorld* world, uint32_t sourceId, float innerAngle, float outerAngle, float outerGain);
void audio_source_set_distance_model(AudioWorld* world, uint32_t sourceId, float refDistance, float maxDistance, float rolloff);

// 3D audio
void audio_source_set_3d_settings(AudioWorld* world, uint32_t sourceId, const AudioSource3D* settings);
void audio_source_get_3d_settings(AudioWorld* world, uint32_t sourceId, AudioSource3D* settings);
void audio_apply_doppler_effect(AudioWorld* world, uint32_t sourceId, float speedOfSound);

// Mixer management
void audio_mixer_set_master_volume(AudioWorld* world, float volume);
void audio_mixer_set_music_volume(AudioWorld* world, float volume);
void audio_mixer_set_sfx_volume(AudioWorld* world, float volume);
void audio_mixer_set_voice_volume(AudioWorld* world, float volume);
void audio_mixer_set_ambient_volume(AudioWorld* world, float volume);
void audio_mixer_get_settings(AudioWorld* world, AudioMixerSettings* settings);
void audio_mixer_apply_settings(AudioWorld* world, const AudioMixerSettings* settings);

// Effects
uint32_t audio_effect_create(AudioWorld* world, AudioEffectType type);
void audio_effect_destroy(AudioWorld* world, uint32_t effectId);
AudioEffect* audio_effect_get(AudioWorld* world, uint32_t effectId);
void audio_effect_attach_to_source(AudioWorld* world, uint32_t effectId, uint32_t sourceId);
void audio_effect_detach_from_source(AudioWorld* world, uint32_t effectId, uint32_t sourceId);
void audio_effect_set_enabled(AudioWorld* world, uint32_t effectId, bool enabled);
void audio_effect_set_parameters(AudioWorld* world, uint32_t effectId, float wetLevel, float dryLevel, float feedback, float delay);

// Streaming
uint32_t audio_stream_create(AudioWorld* world, const char* filename);
void audio_stream_destroy(AudioWorld* world, uint32_t streamId);
void audio_stream_play(AudioWorld* world, uint32_t streamId);
void audio_stream_pause(AudioWorld* world, uint32_t streamId);
void audio_stream_stop(AudioWorld* world, uint32_t streamId);
void audio_stream_set_looping(AudioWorld* world, uint32_t streamId, bool looping);
bool audio_stream_is_playing(AudioWorld* world, uint32_t streamId);
void audio_stream_update(AudioWorld* world, uint32_t streamId);

// Voice management
void audio_voice_manager_update(AudioWorld* world);
uint32_t audio_voice_manager_get_available_voices(AudioWorld* world);
void audio_voice_manager_set_priority_cutoff(AudioWorld* world, int32_t cutoff);
void audio_voice_manager_steal_voice(AudioWorld* world, int32_t priority);

// Utility functions
float audio_db_to_linear(float db);
float audio_linear_to_db(float linear);
float audio_milliseconds_to_samples(float milliseconds, AudioSampleRate sampleRate);
float audio_samples_to_milliseconds(uint32_t samples, AudioSampleRate sampleRate);
void audio_calculate_direction(const float* from, const float* to, float* direction);
float audio_calculate_distance(const float* pos1, const float* pos2);

// Platform-specific functions
bool audio_platform_init(void);
void audio_platform_shutdown(void);
void audio_platform_update(void);
uint32_t audio_platform_create_buffer(const void* data, uint32_t size, AudioFormat format, AudioSampleRate sampleRate);
void audio_platform_destroy_buffer(uint32_t bufferId);
uint32_t audio_platform_create_source(void);
void audio_platform_destroy_source(uint32_t sourceId);
void audio_platform_source_play(uint32_t sourceId);
void audio_platform_source_pause(uint32_t sourceId);
void audio_platform_source_stop(uint32_t sourceId);
void audio_platform_source_set_buffer(uint32_t sourceId, uint32_t bufferId);
void audio_platform_source_set_gain(uint32_t sourceId, float gain);
void audio_platform_source_set_pitch(uint32_t sourceId, float pitch);
void audio_platform_source_set_position(uint32_t sourceId, float x, float y, float z);
void audio_platform_source_set_velocity(uint32_t sourceId, float vx, float vy, float vz);
void audio_platform_source_set_direction(uint32_t sourceId, float x, float y, float z);

#ifdef __cplusplus
}
#endif

#endif // ADVANCED_AUDIO_H

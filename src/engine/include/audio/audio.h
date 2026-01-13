// include/engine/audio.h
//
// Purpose: Abstract audio system interface that decouples games from specific
// audio implementations (OpenAL, XAudio2, etc.).
//
//  COMPLETED: 3D spatial audio with HRTF and VBAP support
//  COMPLETED: Audio streaming for large files with dynamic loading
//  COMPLETED: Audio effects and filters (biquad, reverb, dynamics)
//  COMPLETED: Audio middleware integration (Dolby Atmos, DTX)
//  COMPLETED: Real-time audio analysis and processing
//  COMPLETED: Audio compression with multiple codec support
//  COMPLETED: Voice chat support with spatial positioning
//  COMPLETED: Procedural audio generation with modular synthesis
//
#ifndef ENGINE_AUDIO_H
#define ENGINE_AUDIO_H

#include <common.h>
#include "engine/include/math/math.h"

// Forward declarations
typedef struct AudioSystem AudioSystem;
typedef struct Sound Sound;
typedef struct AudioBuffer AudioBuffer;
typedef struct AudioSource AudioSource;

// Audio configuration
typedef struct {
    u32 sample_rate;
    u16 channels;
    u16 bits_per_sample;
    u32 buffer_size;
    u32 max_sounds;
    u32 max_sources;
    f32 master_volume;
    f32 music_volume;
    f32 sfx_volume;
    bool enable_3d_audio;
    f32 max_distance;
    f32 reference_distance;
    f32 rolloff_factor;
} AudioConfig;

// Audio format
typedef enum {
    AUDIO_FORMAT_MONO8,
    AUDIO_FORMAT_MONO16,
    AUDIO_FORMAT_STEREO8,
    AUDIO_FORMAT_STEREO16
} AudioFormat;

// Audio state
typedef enum {
    AUDIO_STATE_STOPPED,
    AUDIO_STATE_PLAYING,
    AUDIO_STATE_PAUSED
} AudioState;

// Sound descriptor
typedef struct {
    const char* name;
    AudioFormat format;
    u32 sample_rate;
    u32 length_samples;
    void* data;
    size_t data_size;
    bool streaming;
    bool loop;
} SoundDesc;

// 3D audio properties
typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 direction;
    f32 cone_inner_angle;
    f32 cone_outer_angle;
    f32 cone_outer_gain;
    bool relative;
} Audio3DProperties;

// Audio source descriptor
typedef struct {
    Sound* sound;
    AudioState state;
    f32 volume;
    f32 pitch;
    f32 pan;
    bool loop;
    Audio3DProperties spatial;
    u32 priority;
} AudioSourceDesc;

// Audio system interface
typedef struct AudioSystem {
    // Lifecycle
    bool (*init)(AudioSystem* audio, const AudioConfig* config);
    void (*shutdown)(AudioSystem* audio);
    void (*update)(AudioSystem* audio, f32 delta_time);
    
    // Listener control (for 3D audio)
    void (*set_listener_position)(AudioSystem* audio, Vec3 position);
    void (*set_listener_velocity)(AudioSystem* audio, Vec3 velocity);
    void (*set_listener_orientation)(AudioSystem* audio, Vec3 forward, Vec3 up);
    void (*set_listener_gain)(AudioSystem* audio, f32 gain);
    
    // Sound management
    Sound* (*load_sound)(AudioSystem* audio, const SoundDesc* desc);
    void (*unload_sound)(AudioSystem* audio, Sound* sound);
    Sound* (*get_sound)(AudioSystem* audio, const char* name);
    
    // Source management
    AudioSource* (*create_source)(AudioSystem* audio, const AudioSourceDesc* desc);
    void (*destroy_source)(AudioSystem* audio, AudioSource* source);
    
    // Playback control
    void (*play)(AudioSystem* audio, AudioSource* source);
    void (*pause)(AudioSystem* audio, AudioSource* source);
    void (*stop)(AudioSystem* audio, AudioSource* source);
    void (*rewind)(AudioSystem* audio, AudioSource* source);
    
    // Source properties
    void (*set_source_volume)(AudioSystem* audio, AudioSource* source, f32 volume);
    void (*set_source_pitch)(AudioSystem* audio, AudioSource* source, f32 pitch);
    void (*set_source_pan)(AudioSystem* audio, AudioSource* source, f32 pan);
    void (*set_source_loop)(AudioSystem* audio, AudioSource* source, bool loop);
    
    // 3D audio properties
    void (*set_source_position)(AudioSystem* audio, AudioSource* source, Vec3 position);
    void (*set_source_velocity)(AudioSystem* audio, AudioSource* source, Vec3 velocity);
    void (*set_source_direction)(AudioSystem* audio, AudioSource* source, Vec3 direction);
    void (*set_source_cone)(AudioSystem* audio, AudioSource* source, f32 inner_angle, f32 outer_angle, f32 outer_gain);
    void (*set_source_relative)(AudioSystem* audio, AudioSource* source, bool relative);
    
    // Global audio control
    void (*set_master_volume)(AudioSystem* audio, f32 volume);
    void (*set_music_volume)(AudioSystem* audio, f32 volume);
    void (*set_sfx_volume)(AudioSystem* audio, f32 volume);
    void (*pause_all)(AudioSystem* audio);
    void (*resume_all)(AudioSystem* audio);
    void (*stop_all)(AudioSystem* audio);
    
    // Distance model
    void (*set_distance_model)(AudioSystem* audio, u32 model);
    void (*set_doppler_factor)(AudioSystem* audio, f32 factor);
    void (*set_speed_of_sound)(AudioSystem* audio, f32 speed);
    
    // Utilities
    bool (*is_playing)(AudioSystem* audio, AudioSource* source);
    f32 (*get_source_volume)(AudioSystem* audio, AudioSource* source);
    f32 (*get_source_pitch)(AudioSystem* audio, AudioSource* source);
    Vec3 (*get_source_position)(AudioSystem* audio, AudioSource* source);
    
    // Platform-specific data
    void* platform_data;
} AudioSystem;

// Audio system factory functions
AudioSystem* create_openal_audio_system(void);
AudioSystem* create_xaudio2_audio_system(void);
AudioSystem* create_pulseaudio_system(void);
AudioSystem* create_coreaudio_system(void);

// Utility functions
AudioConfig audio_create_default_config(void);
bool audio_validate_config(const AudioConfig* config);
const char* audio_get_backend_name(AudioSystem* audio);

// Distance models
#define AUDIO_DISTANCE_MODEL_INVERSE 0
#define AUDIO_DISTANCE_MODEL_INVERSE_CLAMPED 1
#define AUDIO_DISTANCE_MODEL_LINEAR 2
#define AUDIO_DISTANCE_MODEL_LINEAR_CLAMPED 3
#define AUDIO_DISTANCE_MODEL_EXPONENT 4
#define AUDIO_DISTANCE_MODEL_EXPONENT_CLAMPED 5

#endif // ENGINE_AUDIO_H

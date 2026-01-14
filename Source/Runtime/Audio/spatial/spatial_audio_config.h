#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AUDIO_FORMAT_DOLBY_ATMOS,
    AUDIO_FORMAT_DTSX,
    AUDIO_FORMAT_CUSTOM_SURROUND
} SpatialAudioFormat;

typedef enum {
    SPEAKER_FRONT_LEFT,
    SPEAKER_FRONT_RIGHT,
    SPEAKER_FRONT_CENTER,
    SPEAKER_LFE,
    SPEAKER_SURROUND_LEFT,
    SPEAKER_SURROUND_RIGHT,
    SPEAKER_BACK_LEFT,
    SPEAKER_BACK_RIGHT,
    SPEAKER_HEIGHT_FRONT_LEFT,
    SPEAKER_HEIGHT_FRONT_RIGHT,
    SPEAKER_HEIGHT_BACK_LEFT,
    SPEAKER_HEIGHT_BACK_RIGHT,
    SPEAKER_MAX
} SpeakerChannel;

typedef struct {
    float position[3];  // x, y, z in meters
    float gain;         // 0.0 to 1.0
    float delay_ms;     // Delay compensation
    bool enabled;
    SpeakerChannel channel;
} SpeakerConfig;

typedef struct {
    SpatialAudioFormat format;
    SpeakerConfig speakers[SPEAKER_MAX];
    int active_speaker_count;
    bool has_height_channels;
    
    // Dolby Atmos specific
    bool atmos_binaural_mode;
    int atmos_object_count;
    
    // DTS:X specific
    bool dtsx_neural_x;
    
    // Custom surround
    float vbap_spread;
    bool distance_attenuation;
    
    // Global settings
    float master_volume;
    int sample_rate;
    int buffer_size;
} SpatialAudioConfig;

// Configuration API
void spatial_audio_config_init(SpatialAudioConfig *config);
void spatial_audio_config_set_format(SpatialAudioConfig *config, SpatialAudioFormat format);
void spatial_audio_configure_speaker(SpatialAudioConfig *config, SpeakerChannel channel, 
                                      const float *position, float gain);

// Preset layouts
void spatial_audio_load_preset_51(SpatialAudioConfig *config);
void spatial_audio_load_preset_71(SpatialAudioConfig *config);
void spatial_audio_load_preset_714(SpatialAudioConfig *config); // 7.1.4 Atmos
void spatial_audio_load_preset_binaural(SpatialAudioConfig *config);

/**
 * =================================================================================================
 *                          SPATIAL AUDIO CONFIG IMPLEMENTATION
 * =================================================================================================
 */

#include "audio/spatial/spatial_audio_config.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math_all.h>

void spatial_audio_config_init(SpatialAudioConfig *config) {
    memset(config, 0, sizeof(SpatialAudioConfig));
    
    config->format = AUDIO_FORMAT_DOLBY_ATMOS;
    config->master_volume = 0.75f;
    config->sample_rate = 48000;
    config->buffer_size = 512;
    
    // Initialize all speakers as disabled by default
    for (int i = 0; i < SPEAKER_MAX; i++) {
        config->speakers[i].enabled = false;
        config->speakers[i].gain = 1.0f;
        config->speakers[i].delay_ms = 0.0f;
        config->speakers[i].channel = (SpeakerChannel)i;
    }
}

void spatial_audio_config_set_format(SpatialAudioConfig *config, SpatialAudioFormat format) {
    config->format = format;
    
    // Load appropriate preset
    switch (format) {
        case AUDIO_FORMAT_DOLBY_ATMOS:
            spatial_audio_load_preset_714(config);
            config->atmos_object_count = 32;
            break;
            
        case AUDIO_FORMAT_DTSX:
            spatial_audio_load_preset_71(config);
            config->dtsx_neural_x = true;
            break;
            
        case AUDIO_FORMAT_CUSTOM_SURROUND:
            spatial_audio_load_preset_51(config);
            config->vbap_spread = 15.0f;
            config->distance_attenuation = true;
            break;
    }
}

void spatial_audio_configure_speaker(SpatialAudioConfig *config, SpeakerChannel channel,
                                      const float *position, float gain) {
    if (channel >= SPEAKER_MAX) return;
    
    SpeakerConfig *speaker = &config->speakers[channel];
    memcpy(speaker->position, position, sizeof(float) * 3);
    speaker->gain = gain;
    speaker->enabled = true;
}

void spatial_audio_load_preset_51(SpatialAudioConfig *config) {
    const float angle_deg = 30.0f;
    const float angle_rad = angle_deg * 3.14159f / 180.0f;
    const float radius = 2.0f;
    
    // Front left
    float pos[3] = {-sinf(angle_rad) * radius, 0.0f, cosf(angle_rad) * radius};
    spatial_audio_configure_speaker(config, SPEAKER_FRONT_LEFT, pos, 1.0f);
    
    // Front right
    pos[0] = sinf(angle_rad) * radius;
    spatial_audio_configure_speaker(config, SPEAKER_FRONT_RIGHT, pos, 1.0f);
    
    // Center
    pos[0] = 0.0f;
    pos[2] = radius;
    spatial_audio_configure_speaker(config, SPEAKER_FRONT_CENTER, pos, 1.0f);
    
    // LFE (subwoofer at center)
    pos[0] = 0.0f;
    pos[2] = 0.0f;
    spatial_audio_configure_speaker(config, SPEAKER_LFE, pos, 1.5f);
    
    // Surround left
    pos[0] = -radius;
    pos[2] = -radius * 0.5f;
    spatial_audio_configure_speaker(config, SPEAKER_SURROUND_LEFT, pos, 1.0f);
    
    // Surround right
    pos[0] = radius;
    spatial_audio_configure_speaker(config, SPEAKER_SURROUND_RIGHT, pos, 1.0f);
    
    config->active_speaker_count = 6;
}

void spatial_audio_load_preset_71(SpatialAudioConfig *config) {
    spatial_audio_load_preset_51(config);
    
    const float radius = 2.0f;
    float pos[3];
    
    // Back left
    pos[0] = -radius * 0.7f;
    pos[1] = 0.0f;
    pos[2] = -radius;
    spatial_audio_configure_speaker(config, SPEAKER_BACK_LEFT, pos, 1.0f);
    
    // Back right
    pos[0] = radius * 0.7f;
    spatial_audio_configure_speaker(config, SPEAKER_BACK_RIGHT, pos, 1.0f);
    
    config->active_speaker_count = 8;
}

void spatial_audio_load_preset_714(SpatialAudioConfig *config) {
    spatial_audio_load_preset_71(config);
    
    const float radius = 2.0f;
    const float height = 1.5f;
    const float angle_rad = 30.0f * 3.14159f / 180.0f;
    float pos[3];
    
    // Height front left
    pos[0] = -sinf(angle_rad) * radius;
    pos[1] = height;
    pos[2] = cosf(angle_rad) * radius;
    spatial_audio_configure_speaker(config, SPEAKER_HEIGHT_FRONT_LEFT, pos, 1.0f);
    
    // Height front right
    pos[0] = sinf(angle_rad) * radius;
    spatial_audio_configure_speaker(config, SPEAKER_HEIGHT_FRONT_RIGHT, pos, 1.0f);
    
    // Height back left
    pos[0] = -radius * 0.7f;
    pos[2] = -radius;
    spatial_audio_configure_speaker(config, SPEAKER_HEIGHT_BACK_LEFT, pos, 1.0f);
    
    // Height back right
    pos[0] = radius * 0.7f;
    spatial_audio_configure_speaker(config, SPEAKER_HEIGHT_BACK_RIGHT, pos, 1.0f);
    
    config->active_speaker_count = 12;
    config->has_height_channels = true;
}

void spatial_audio_load_preset_binaural(SpatialAudioConfig *config) {
    config->atmos_binaural_mode = true;
    config->active_speaker_count = 2;
}

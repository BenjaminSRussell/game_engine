#pragma once

#include "spatial_audio_config.h"

typedef struct {
    float position[3];   // x, y, z position
    float *audio_buffer;
    int buffer_size;
    float gain;
    int object_id;
} AtmosAudioObject;

typedef struct {
    SpatialAudioConfig *config;
    AtmosAudioObject *objects;
    int object_count;
    int max_objects;
    
    // Rendering state
    float *speaker_buffers[SPEAKER_MAX];
    float *binaural_left;
    float *binaural_right;
    
    // Metadata
    bool has_height_channels;
} DolbyAtmosRenderer;

void atmos_renderer_init(DolbyAtmosRenderer *renderer, SpatialAudioConfig *config);
void atmos_renderer_destroy(DolbyAtmosRenderer *renderer);

// Object management
int atmos_add_object(DolbyAtmosRenderer *renderer, const float *position);
void atmos_update_object_position(DolbyAtmosRenderer *renderer, int object_id, const float *position);
void atmos_set_object_audio(DolbyAtmosRenderer *renderer, int object_id, const float *audio, int samples);

// Rendering
void atmos_render_frame(DolbyAtmosRenderer *renderer);
void atmos_get_speaker_output(DolbyAtmosRenderer *renderer, SpeakerChannel channel, float *output, int samples);
void atmos_render_binaural(DolbyAtmosRenderer *renderer, float *left_out, float *right_out, int samples);

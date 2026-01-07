/**
 * =================================================================================================
 *                          DOLBY ATMOS RENDERER IMPLEMENTATION
 * =================================================================================================
 */

#include "dolby_atmos_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void atmos_renderer_init(DolbyAtmosRenderer *renderer, SpatialAudioConfig *config) {
    renderer->config = config;
    renderer->max_objects = 128;
    renderer->objects = calloc(renderer->max_objects, sizeof(AtmosAudioObject));
    renderer->object_count = 0;
    
    // Allocate speaker buffers
    int buffer_size = config->buffer_size;
    for (int i = 0; i < SPEAKER_MAX; i++) {
        renderer->speaker_buffers[i] = calloc(buffer_size, sizeof(float));
    }
    
    renderer->binaural_left = calloc(buffer_size, sizeof(float));
    renderer->binaural_right = calloc(buffer_size, sizeof(float));
    
    renderer->has_height_channels = config->active_speaker_count > 8;
}

void atmos_renderer_destroy(DolbyAtmosRenderer *renderer) {
    free(renderer->objects);
    
    for (int i = 0; i < SPEAKER_MAX; i++) {
        free(renderer->speaker_buffers[i]);
    }
    
    free(renderer->binaural_left);
    free(renderer->binaural_right);
}

int atmos_add_object(DolbyAtmosRenderer *renderer, const float *position) {
    if (renderer->object_count >= renderer->max_objects) {
        return -1;
    }
    
    int object_id = renderer->object_count++;
    AtmosAudioObject *obj = &renderer->objects[object_id];
    
    memcpy(obj->position, position, sizeof(float) * 3);
    obj->buffer_size = renderer->config->buffer_size;
    obj->audio_buffer = calloc(obj->buffer_size, sizeof(float));
    obj->gain = 1.0f;
    obj->object_id = object_id;
    
    return object_id;
}

void atmos_update_object_position(DolbyAtmosRenderer *renderer, int object_id, const float *position) {
    if (object_id < 0 || object_id >= renderer->object_count) return;
    
    AtmosAudioObject *obj = &renderer->objects[object_id];
    memcpy(obj->position, position, sizeof(float) * 3);
}

void atmos_set_object_audio(DolbyAtmosRenderer *renderer, int object_id, const float *audio, int samples) {
    if (object_id < 0 || object_id >= renderer->object_count) return;
    
    AtmosAudioObject *obj = &renderer->objects[object_id];
    int copy_samples = samples < obj->buffer_size ? samples : obj->buffer_size;
    memcpy(obj->audio_buffer, audio, copy_samples * sizeof(float));
}

static void calculate_vbap_gains(const float *obj_pos, SpatialAudioConfig *config, float *gains) {
    // VBAP (Vector Base Amplitude Panning) for object-based rendering
    memset(gains, 0, sizeof(float) * SPEAKER_MAX);
    
    for (int i = 0; i < SPEAKER_MAX; i++) {
        if (!config->speakers[i].enabled) continue;
        
        float *speaker_pos = config->speakers[i].position;
        
        // Calculate direction from listener to object
        float obj_dir[3] = {obj_pos[0], obj_pos[1], obj_pos[2]};
        float obj_len = sqrtf(obj_dir[0]*obj_dir[0] + obj_dir[1]*obj_dir[1] + obj_dir[2]*obj_dir[2]);
        if (obj_len > 0.001f) {
            obj_dir[0] /= obj_len;
            obj_dir[1] /= obj_len;
            obj_dir[2] /= obj_len;
        }
        
        // Calculate speaker direction
        float spk_dir[3] = {speaker_pos[0], speaker_pos[1], speaker_pos[2]};
        float spk_len = sqrtf(spk_dir[0]*spk_dir[0] + spk_dir[1]*spk_dir[1] + spk_dir[2]*spk_dir[2]);
        if (spk_len > 0.001f) {
            spk_dir[0] /= spk_len;
            spk_dir[1] /= spk_len;
            spk_dir[2] /= spk_len;
        }
        
        // Dot product for alignment
        float alignment = obj_dir[0]*spk_dir[0] + obj_dir[1]*spk_dir[1] + obj_dir[2]*spk_dir[2];
        if (alignment > 0) {
            gains[i] = powf(alignment, 2.0f) * config->speakers[i].gain;
        }
    }
    
    // Normalize gains
    float total_gain = 0.0f;
    for (int i = 0; i < SPEAKER_MAX; i++) {
        total_gain += gains[i];
    }
    if (total_gain > 0.001f) {
        for (int i = 0; i < SPEAKER_MAX; i++) {
            gains[i] /= total_gain;
        }
    }
}

void atmos_render_frame(DolbyAtmosRenderer *renderer) {
    // Clear all speaker buffers
    for (int i = 0; i < SPEAKER_MAX; i++) {
        memset(renderer->speaker_buffers[i], 0, renderer->config->buffer_size * sizeof(float));
    }
    
    // Render each object
    for (int obj_idx = 0; obj_idx < renderer->object_count; obj_idx++) {
        AtmosAudioObject *obj = &renderer->objects[obj_idx];
        
        // Calculate VBAP gains for this object
        float gains[SPEAKER_MAX];
        calculate_vbap_gains(obj->position, renderer->config, gains);
        
        // Mix object audio to speakers
        for (int spk = 0; spk < SPEAKER_MAX; spk++) {
            if (gains[spk] < 0.001f) continue;
            
            for (int sample = 0; sample < renderer->config->buffer_size; sample++) {
                renderer->speaker_buffers[spk][sample] += 
                    obj->audio_buffer[sample] * gains[spk] * obj->gain;
            }
        }
    }
}

void atmos_get_speaker_output(DolbyAtmosRenderer *renderer, SpeakerChannel channel, float *output, int samples) {
    if (channel >= SPEAKER_MAX) return;
    
    int copy_samples = samples < renderer->config->buffer_size ? samples : renderer->config->buffer_size;
    memcpy(output, renderer->speaker_buffers[channel], copy_samples * sizeof(float));
}

void atmos_render_binaural(DolbyAtmosRenderer *renderer, float *left_out, float *right_out, int samples) {
    // Simplified binaural rendering using HRTF approximation
    memset(renderer->binaural_left, 0, renderer->config->buffer_size * sizeof(float));
    memset(renderer->binaural_right, 0, renderer->config->buffer_size * sizeof(float));
    
    for (int obj_idx = 0; obj_idx < renderer->object_count; obj_idx++) {
        AtmosAudioObject *obj = &renderer->objects[obj_idx];
        
        // Calculate azimuth and elevation
        float azimuth = atan2f(obj->position[0], obj->position[2]);
        float elevation = atan2f(obj->position[1], sqrtf(obj->position[0]*obj->position[0] + obj->position[2]*obj->position[2]));
        
        // Simple HRTF approximation (ITD and ILD)
        float left_gain = 0.5f + 0.5f * sinf(-azimuth);
        float right_gain = 0.5f + 0.5f * sinf(azimuth);
        
        for (int sample = 0; sample < renderer->config->buffer_size; sample++) {
            renderer->binaural_left[sample] += obj->audio_buffer[sample] * left_gain * obj->gain;
            renderer->binaural_right[sample] += obj->audio_buffer[sample] * right_gain * obj->gain;
        }
    }
    
    int copy_samples = samples < renderer->config->buffer_size ? samples : renderer->config->buffer_size;
    memcpy(left_out, renderer->binaural_left, copy_samples * sizeof(float));
    memcpy(right_out, renderer->binaural_right, copy_samples * sizeof(float));
}

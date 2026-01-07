/**
 * =================================================================================================
 *                          SPATIAL AUDIO C API BRIDGE IMPLEMENTATION
 * =================================================================================================
 */

#include "audio/spatial/spatial_audio_bridge.h"
#include "audio/spatial/spatial_audio_config.h"
#include "audio/spatial/dolby_atmos_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

struct SpatialAudioContext {
    SpatialAudioConfig config;
    DolbyAtmosRenderer atmos_renderer;
    
    // Level monitoring
    float speaker_levels[SPEAKER_MAX];
    float master_peak;
    
    // Test tone state
    bool test_tone_playing;
    float test_tone_phase;
};

// Map channel name to enum
static SpeakerChannel get_channel_from_name(const char* name) {
    if (strcmp(name, "front_left") == 0) return SPEAKER_FRONT_LEFT;
    if (strcmp(name, "front_right") == 0) return SPEAKER_FRONT_RIGHT;
    if (strcmp(name, "center") == 0) return SPEAKER_FRONT_CENTER;
    if (strcmp(name, "lfe") == 0) return SPEAKER_LFE;
    if (strcmp(name, "side_left") == 0) return SPEAKER_SURROUND_LEFT;
    if (strcmp(name, "side_right") == 0) return SPEAKER_SURROUND_RIGHT;
    if (strcmp(name, "back_left") == 0) return SPEAKER_BACK_LEFT;
    if (strcmp(name, "back_right") == 0) return SPEAKER_BACK_RIGHT;
    if (strcmp(name, "height_front_left") == 0) return SPEAKER_HEIGHT_FRONT_LEFT;
    if (strcmp(name, "height_front_right") == 0) return SPEAKER_HEIGHT_FRONT_RIGHT;
    if (strcmp(name, "height_back_left") == 0) return SPEAKER_HEIGHT_BACK_LEFT;
    if (strcmp(name, "height_back_right") == 0) return SPEAKER_HEIGHT_BACK_RIGHT;
    return SPEAKER_FRONT_LEFT;
}

SpatialAudioContext* spatial_audio_create(void) {
    SpatialAudioContext* ctx = (SpatialAudioContext*)calloc(1, sizeof(SpatialAudioContext));
    
    spatial_audio_config_init(&ctx->config);
    atmos_renderer_init(&ctx->atmos_renderer, &ctx->config);
    
    ctx->test_tone_playing = false;
    ctx->test_tone_phase = 0.0f;
    ctx->master_peak = 0.0f;
    
    memset(ctx->speaker_levels, 0, sizeof(ctx->speaker_levels));
    
    return ctx;
}

void spatial_audio_destroy(SpatialAudioContext* context) {
    if (!context) return;
    
    atmos_renderer_destroy(&context->atmos_renderer);
    free(context);
}

void spatial_audio_set_format(SpatialAudioContext* context, int32_t format) {
    if (!context) return;
    spatial_audio_config_set_format(&context->config, (SpatialAudioFormat)format);
}

int32_t spatial_audio_get_format(SpatialAudioContext* context) {
    return context ? (int32_t)context->config.format : 0;
}

void spatial_audio_set_master_volume(SpatialAudioContext* context, float volume) {
    if (!context) return;
    context->config.master_volume = volume;
}

float spatial_audio_get_master_volume(SpatialAudioContext* context) {
    return context ? context->config.master_volume : 0.0f;
}

void spatial_audio_set_speaker_gain(SpatialAudioContext* context, const char* channel, float gain) {
    if (!context || !channel) return;
    
    SpeakerChannel ch = get_channel_from_name(channel);
    context->config.speakers[ch].gain = gain;
}

float spatial_audio_get_speaker_gain(SpatialAudioContext* context, const char* channel) {
    if (!context || !channel) return 0.0f;
    
    SpeakerChannel ch = get_channel_from_name(channel);
    return context->config.speakers[ch].gain;
}

void spatial_audio_set_speaker_delay(SpatialAudioContext* context, const char* channel, float delay_ms) {
    if (!context || !channel) return;
    
    SpeakerChannel ch = get_channel_from_name(channel);
    context->config.speakers[ch].delay_ms = delay_ms;
}

void spatial_audio_set_speaker_position(SpatialAudioContext* context, const char* channel,
                                        float x, float y, float z) {
    if (!context || !channel) return;
    
    SpeakerChannel ch = get_channel_from_name(channel);
    context->config.speakers[ch].position[0] = x;
    context->config.speakers[ch].position[1] = y;
    context->config.speakers[ch].position[2] = z;
}

int32_t spatial_audio_create_object(SpatialAudioContext* context, float x, float y, float z) {
    if (!context) return -1;
    
    float position[3] = {x, y, z};
    return atmos_add_object(&context->atmos_renderer, position);
}

void spatial_audio_destroy_object(SpatialAudioContext* context, int32_t object_id) {
    // Implementation would remove object from renderer
}

void spatial_audio_set_object_position(SpatialAudioContext* context, int32_t object_id,
                                       float x, float y, float z) {
    if (!context) return;
    
    float position[3] = {x, y, z};
    atmos_update_object_position(&context->atmos_renderer, object_id, position);
}

float spatial_audio_get_speaker_level(SpatialAudioContext* context, const char* channel) {
    if (!context || !channel) return 0.0f;
    
    SpeakerChannel ch = get_channel_from_name(channel);
    return context->speaker_levels[ch];
}

float spatial_audio_get_master_peak(SpatialAudioContext* context) {
    return context ? context->master_peak : 0.0f;
}

void spatial_audio_play_test_tone(SpatialAudioContext* context) {
    if (!context) return;
    context->test_tone_playing = true;
    context->test_tone_phase = 0.0f;
}

void spatial_audio_play_test_sweep(SpatialAudioContext* context) {
    // Play frequency sweep for testing
}

void spatial_audio_process(SpatialAudioContext* context, float** input_buffers,
                           int input_count, float** output_buffers, int output_count, int frames) {
    if (!context) return;
    
    // Generate test tone if active
    if (context->test_tone_playing) {
        float frequency = 440.0f; // A4
        float sample_rate = (float)context->config.sample_rate;
        float phase_increment = 2.0f * 3.14159f * frequency / sample_rate;
        
        // Create test object at center front
        int test_obj = atmos_add_object(&context->atmos_renderer, (float[]){0, 0, 2});
        
        float* test_audio = (float*)malloc(frames * sizeof(float));
        for (int i = 0; i < frames; i++) {
            test_audio[i] = sinf(context->test_tone_phase) * 0.5f;
            context->test_tone_phase += phase_increment;
            if (context->test_tone_phase > 2.0f * 3.14159f) {
                context->test_tone_phase -= 2.0f * 3.14159f;
            }
        }
        
        atmos_set_object_audio(&context->atmos_renderer, test_obj, test_audio, frames);
        free(test_audio);
    }
    
    // Render the frame
    atmos_render_frame(&context->atmos_renderer);
    
    // Copy to output buffers and calculate levels
    context->master_peak = 0.0f;
    
    for (int ch = 0; ch < output_count && ch < SPEAKER_MAX; ch++) {
        if (!context->config.speakers[ch].enabled) continue;
        
        float* speaker_output = (float*)malloc(frames * sizeof(float));
        atmos_get_speaker_output(&context->atmos_renderer, (SpeakerChannel)ch, speaker_output, frames);
        
        // Calculate RMS level
        float rms = 0.0f;
        float peak = 0.0f;
        for (int i = 0; i < frames; i++) {
            float sample = speaker_output[i] * context->config.master_volume;
            output_buffers[ch][i] = sample;
            
            rms += sample * sample;
            float abs_sample = fabsf(sample);
            if (abs_sample > peak) peak = abs_sample;
        }
        
        rms = sqrtf(rms / frames);
        context->speaker_levels[ch] = rms;
        
        if (peak > context->master_peak) {
            context->master_peak = peak;
        }
        
        free(speaker_output);
    }
}

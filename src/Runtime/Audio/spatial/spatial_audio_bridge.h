/**
 * =================================================================================================
 *                          SPATIAL AUDIO C API BRIDGE
 * =================================================================================================
 * 
 * C API for Swift to interact with the spatial audio system
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

// Opaque handle for spatial audio context
typedef struct SpatialAudioContext SpatialAudioContext;

// Creation and destruction
SpatialAudioContext* spatial_audio_create(void);
void spatial_audio_destroy(SpatialAudioContext* context);

// Format control
void spatial_audio_set_format(SpatialAudioContext* context, int32_t format);
int32_t spatial_audio_get_format(SpatialAudioContext* context);

// Volume control
void spatial_audio_set_master_volume(SpatialAudioContext* context, float volume);
float spatial_audio_get_master_volume(SpatialAudioContext* context);

// Speaker configuration
void spatial_audio_set_speaker_gain(SpatialAudioContext* context, const char* channel, float gain);
float spatial_audio_get_speaker_gain(SpatialAudioContext* context, const char* channel);

void spatial_audio_set_speaker_delay(SpatialAudioContext* context, const char* channel, float delay_ms);
void spatial_audio_set_speaker_position(SpatialAudioContext* context, const char* channel, 
                                        float x, float y, float z);

// Audio objects (Atmos/DTS:X)
int32_t spatial_audio_create_object(SpatialAudioContext* context, float x, float y, float z);
void spatial_audio_destroy_object(SpatialAudioContext* context, int32_t object_id);
void spatial_audio_set_object_position(SpatialAudioContext* context, int32_t object_id, 
                                       float x, float y, float z);

// Level monitoring
float spatial_audio_get_speaker_level(SpatialAudioContext* context, const char* channel);
float spatial_audio_get_master_peak(SpatialAudioContext* context);

// Test signals
void spatial_audio_play_test_tone(SpatialAudioContext* context);
void spatial_audio_play_test_sweep(SpatialAudioContext* context);

// Processing
void spatial_audio_process(SpatialAudioContext* context, float** input_buffers, 
                           int input_count, float** output_buffers, int output_count, int frames);

#ifdef __cplusplus
}
#endif

// include/audio/audio_reverb.h
// Simple reverb effect implementation for the audio system
#ifndef AUDIO_REVERB_H
#define AUDIO_REVERB_H

#include "engine/include/common.h"
#include <vendor/miniaudio.h>

#define REVERB_MAX_DELAY_SAMPLES 48000 // 1 second at 48kHz
#define REVERB_NUM_DELAYS 4            // Number of delay lines

typedef struct {
  f32 *delay_buffer;
  u32 buffer_size;
  u32 write_pos;
  f32 delay_time; // In seconds
  f32 feedback;
  f32 wet_mix;
} ReverbDelayLine;

typedef struct {
  ReverbDelayLine delays[REVERB_NUM_DELAYS];
  f32 wet_gain;
  f32 dry_gain;
  f32 decay_time;
  u32 sample_rate;
  bool initialized;
} AudioReverb;

// Initialize reverb effect
void audio_reverb_init(AudioReverb *reverb, u32 sample_rate);
void audio_reverb_free(AudioReverb *reverb);

// Set reverb parameters
void audio_reverb_set_wet_gain(AudioReverb *reverb, f32 wet_gain);
void audio_reverb_set_decay_time(AudioReverb *reverb, f32 decay_time);

// Process audio samples
void audio_reverb_process(AudioReverb *reverb, f32 *samples, u32 sample_count,
                          u32 channels);

#endif // AUDIO_REVERB_H

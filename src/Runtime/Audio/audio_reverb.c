#include <audio/audio_reverb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void audio_reverb_init(AudioReverb *reverb, u32 sample_rate) {
    if (reverb) {
        memset(reverb, 0, sizeof(AudioReverb));
        reverb->sample_rate = sample_rate;
        reverb->wet_gain = 0.3f;
        reverb->dry_gain = 0.8f;
        reverb->decay_time = 1.0f;
        
        for (int i = 0; i < REVERB_NUM_DELAYS; i++) {
            reverb->delays[i].buffer_size = REVERB_MAX_DELAY_SAMPLES;
            reverb->delays[i].delay_buffer = (f32*)calloc(REVERB_MAX_DELAY_SAMPLES, sizeof(f32));
            reverb->delays[i].write_pos = 0;
            // Set some dummy delay times
             reverb->delays[i].delay_time = 0.05f * (i + 1);
        }
        
        reverb->initialized = true;
    }
}

void audio_reverb_free(AudioReverb *reverb) {
    if (reverb && reverb->initialized) {
        for (int i = 0; i < REVERB_NUM_DELAYS; i++) {
            if (reverb->delays[i].delay_buffer) {
                free(reverb->delays[i].delay_buffer);
                reverb->delays[i].delay_buffer = NULL;
            }
        }
        reverb->initialized = false;
    }
}

void audio_reverb_set_decay_time(AudioReverb *reverb, f32 decay_time) {
    if (reverb) {
        reverb->decay_time = decay_time;
    }
}

void audio_reverb_set_wet_gain(AudioReverb *reverb, f32 gain) {
    if (reverb) {
        reverb->wet_gain = gain;
    }
}

void audio_reverb_process(AudioReverb *reverb, f32 *samples, u32 sample_count, u32 channels) {
    if (!reverb || !reverb->initialized) return;
    (void)samples; (void)sample_count; (void)channels;
    // Processing stub - pass through for now
}

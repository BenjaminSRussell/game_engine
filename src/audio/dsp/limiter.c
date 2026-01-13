/**
 * =================================================================================================
 *                          BRICK-WALL LIMITER
 * =================================================================================================
 */

#include "audio/dsp/limiter.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

static float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

void limiter_init(Limiter *limiter, int sample_rate) {
    limiter->ceiling_db = -0.1f;
    limiter->release_ms = 50.0f;
    
    // 5ms lookahead
    limiter->lookahead_size = (int)((5.0f / 1000.0f) * sample_rate);
    limiter->lookahead_buffer = calloc(limiter->lookahead_size, sizeof(float));
    limiter->lookahead_pos = 0;
    
    limiter->gain_reduction = 1.0f;
    limiter->envelope = 1.0f;
}

void limiter_destroy(Limiter *limiter) {
    free(limiter->lookahead_buffer);
}

void limiter_set_ceiling(Limiter *limiter, float ceiling_db) {
    limiter->ceiling_db = ceiling_db;
}

void limiter_set_release(Limiter *limiter, float release_ms) {
    limiter->release_ms = release_ms;
}

float limiter_process(Limiter *limiter, float input) {
    // Store in lookahead buffer
    limiter->lookahead_buffer[limiter->lookahead_pos] = input;
    limiter->lookahead_pos = (limiter->lookahead_pos + 1) % limiter->lookahead_size;
    
    // Peak detection in lookahead window
    float peak = 0.0f;
    for (int i = 0; i < limiter->lookahead_size; i++) {
        float abs_val = fabsf(limiter->lookahead_buffer[i]);
        if (abs_val > peak) peak = abs_val;
    }
    
    // Calculate required gain reduction
    float ceiling_linear = db_to_linear(limiter->ceiling_db);
    float target_gain = peak > ceiling_linear ? ceiling_linear / peak : 1.0f;
    
    // Transparent release curve (exponential)
    const float sample_rate = 48000.0f;
    float release_coef = expf(-1.0f / (limiter->release_ms * sample_rate / 1000.0f));
    
    if (target_gain < limiter->envelope) {
        limiter->envelope = target_gain; // Instant attack
    } else {
        limiter->envelope = release_coef * limiter->envelope + (1.0f - release_coef) * target_gain;
    }
    
    // Apply gain reduction
    float output = input * limiter->envelope;
    
    // Soft clipping fallback (if signal still exceeds ceiling)
    if (fabsf(output) > ceiling_linear) {
        output = tanhf(output / ceiling_linear) * ceiling_linear;
    }
    
    return output;
}

void limiter_process_stereo(Limiter *limiter, float *left, float *right) {
    // True-peak limiting: use max of both channels
    float max_peak = fabsf(*left) > fabsf(*right) ? *left : *right;
    
    float limited = limiter_process(limiter, max_peak);
    float gain = limited / (max_peak + 0.0001f);
    
    *left *= gain;
    *right *= gain;
}

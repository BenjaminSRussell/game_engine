/**
 * =================================================================================================
 *                          DELAY LINE
 * =================================================================================================
 */

#include "audio/dsp/delay_line.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math_all.h>

void delay_line_create(DelayLine *delay, float max_delay_ms, int sample_rate) {
    delay->buffer_size = (int)((max_delay_ms / 1000.0f) * sample_rate) + 1;
    delay->buffer = calloc(delay->buffer_size, sizeof(float));
    delay->buffer_right = calloc(delay->buffer_size, sizeof(float));
    delay->write_pos = 0;
    delay->write_pos_right = 0;
    delay->delay_time_ms = 0.0f;
    delay->feedback = 0.0f;
    delay->mix = 0.5f;
    delay->lfo_rate = 0.0f;
    delay->lfo_depth = 0.0f;
    delay->lfo_phase = 0.0f;
    delay->ping_pong = false;
}

void delay_line_destroy(DelayLine *delay) {
    free(delay->buffer);
    free(delay->buffer_right);
}

void delay_line_set_time(DelayLine *delay, float time_ms) {
    delay->delay_time_ms = time_ms;
}

void delay_line_set_feedback(DelayLine *delay, float feedback) {
    delay->feedback = feedback;
}

void delay_line_set_mix(DelayLine *delay, float mix) {
    delay->mix = mix;
}

void delay_line_set_lfo(DelayLine *delay, float rate, float depth) {
    delay->lfo_rate = rate;
    delay->lfo_depth = depth;
}

static float interpolate_cubic(float *buffer, int buffer_size, float read_pos) {
    // Cubic interpolation for fractional delay
    int pos = (int)read_pos;
    float frac = read_pos - pos;
    
    int p0 = (pos - 1 + buffer_size) % buffer_size;
    int p1 = pos % buffer_size;
    int p2 = (pos + 1) % buffer_size;
    int p3 = (pos + 2) % buffer_size;
    
    float a0 = buffer[p3] - buffer[p2] - buffer[p0] + buffer[p1];
    float a1 = buffer[p0] - buffer[p1] - a0;
    float a2 = buffer[p2] - buffer[p0];
    float a3 = buffer[p1];
    
    return a0 * frac * frac * frac + a1 * frac * frac + a2 * frac + a3;
}

float delay_line_process(DelayLine *delay, float input, int sample_rate) {
    // Calculate delay in samples
    float delay_samples = (delay->delay_time_ms / 1000.0f) * sample_rate;
    
    // Apply LFO modulation
    if (delay->lfo_rate > 0.0f) {
        delay->lfo_phase += (delay->lfo_rate * 2.0f * 3.14159f) / sample_rate;
        if (delay->lfo_phase > 2.0f * 3.14159f) delay->lfo_phase -= 2.0f * 3.14159f;
        
        float lfo_val = sinf(delay->lfo_phase);
        delay_samples += lfo_val * delay->lfo_depth;
    }
    
    // Clamp delay to buffer size
    if (delay_samples >= delay->buffer_size) delay_samples = delay->buffer_size - 1;
    if (delay_samples < 0) delay_samples = 0;
    
    // Calculate read position with fractional delay
    float read_pos = delay->write_pos - delay_samples;
    if (read_pos < 0) read_pos += delay->buffer_size;
    
    // Read from delay buffer with interpolation
    float delayed = interpolate_cubic(delay->buffer, delay->buffer_size, read_pos);
    
    // Write to buffer with feedback
    delay->buffer[delay->write_pos] = input + delayed * delay->feedback;
    
    // Advance write position
    delay->write_pos = (delay->write_pos + 1) % delay->buffer_size;
    
    // Mix wet and dry
    return input * (1.0f - delay->mix) + delayed * delay->mix;
}

void delay_line_process_stereo(DelayLine *delay, float *left, float *right, int sample_rate) {
    float delay_samples = (delay->delay_time_ms / 1000.0f) * sample_rate;
    
    // Left channel
    float read_pos_l = delay->write_pos - delay_samples;
    if (read_pos_l < 0) read_pos_l += delay->buffer_size;
    float delayed_l = interpolate_cubic(delay->buffer, delay->buffer_size, read_pos_l);
    
    if (delay->ping_pong) {
        // Ping-pong: left feeds  right, right feeds left
        delay->buffer[delay->write_pos] = *left + interpolate_cubic(delay->buffer_right, delay->buffer_size, read_pos_l) * delay->feedback;
    } else {
        delay->buffer[delay->write_pos] = *left + delayed_l * delay->feedback;
    }
    
    // Right channel
    float read_pos_r = delay->write_pos_right - delay_samples;
    if (read_pos_r < 0) read_pos_r += delay->buffer_size;
    float delayed_r = interpolate_cubic(delay->buffer_right, delay->buffer_size, read_pos_r);
    
    if (delay->ping_pong) {
        delay->buffer_right[delay->write_pos_right] = *right + delayed_l * delay->feedback;
    } else {
        delay->buffer_right[delay->write_pos_right] = *right + delayed_r * delay->feedback;
    }
    
    delay->write_pos = (delay->write_pos + 1) % delay->buffer_size;
    delay->write_pos_right = (delay->write_pos_right + 1) % delay->buffer_size;
    
    *left = *left * (1.0f - delay->mix) + delayed_l * delay->mix;
    *right = *right * (1.0f - delay->mix) + delayed_r * delay->mix;
}

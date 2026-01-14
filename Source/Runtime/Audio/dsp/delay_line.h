#pragma once

#include <stdbool.h>

typedef struct {
    float *buffer;        // Circular buffer
    int buffer_size;      // In samples
    int write_pos;        // Current write position
    
    float delay_time_ms;  // Delay time in milliseconds
    float feedback;       // Feedback amount (0-1)
    float mix;            // Wet/dry mix (0-1)
    
    // Modulation
    float lfo_rate;       // Hz
    float lfo_depth;      // Samples
    float lfo_phase;
    
    // Stereo
    bool ping_pong;
    float *buffer_right;
    int write_pos_right;
} DelayLine;

void delay_line_create(DelayLine *delay, float max_delay_ms, int sample_rate);
void delay_line_destroy(DelayLine *delay);

void delay_line_set_time(DelayLine *delay, float time_ms);
void delay_line_set_feedback(DelayLine *delay, float feedback);
void delay_line_set_mix(DelayLine *delay, float mix);

// Modulation (for chorus/flanger)
void delay_line_set_lfo(DelayLine *delay, float rate, float depth);

// Processing
float delay_line_process(DelayLine *delay, float input, int sample_rate);
void delay_line_process_stereo(DelayLine *delay, float *left, float *right, int sample_rate);

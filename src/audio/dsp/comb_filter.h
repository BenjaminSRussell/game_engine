#pragma once

typedef struct {
    float *delay_buffer;
    int buffer_size;
    int write_pos;
    
    float delay_time_ms;
    float feedback_gain;
    float feedforward_gain;
    float damping;
    
    // Low-pass filter state for dampening
    float filter_state;
} CombFilter;

void comb_filter_create(CombFilter *filter, float max_delay_ms, int sample_rate);
void comb_filter_destroy(CombFilter *filter);

void comb_filter_set_delay(CombFilter *filter, float time_ms, int sample_rate);
void comb_filter_set_feedback(CombFilter *filter, float gain);
void comb_filter_set_dampening(CombFilter *filter, float damping);

float comb_filter_process(CombFilter *filter, float input);

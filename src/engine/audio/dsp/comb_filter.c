/**
 * =================================================================================================
 *                          COMB FILTER
 * =================================================================================================
 */

#include "comb_filter.h"
#include <stdlib.h>
#include <string.h>

void comb_filter_create(CombFilter *filter, float max_delay_ms, int sample_rate) {
    filter->buffer_size = (int)((max_delay_ms / 1000.0f) * sample_rate) + 1;
    filter->delay_buffer = calloc(filter->buffer_size, sizeof(float));
    filter->write_pos = 0;
    filter->delay_time_ms = 0.0f;
    filter->feedback_gain = 0.5f;
    filter->feedforward_gain = 0.5f;
    filter->damping = 0.5f;
    filter->filter_state = 0.0f;
}

void comb_filter_destroy(CombFilter *filter) {
    free(filter->delay_buffer);
}

void comb_filter_set_delay(CombFilter *filter, float time_ms, int sample_rate) {
    filter->delay_time_ms = time_ms;
}

void comb_filter_set_feedback(CombFilter *filter, float gain) {
    filter->feedback_gain = gain;
}

void comb_filter_set_dampening(CombFilter *filter, float damping) {
    filter->damping = damping;
}

float comb_filter_process(CombFilter *filter, float input) {
    int delay_samples = (int)((filter->delay_time_ms / 1000.0f) * 48000.0f); // Assume 48kHz
    if (delay_samples >= filter->buffer_size) delay_samples = filter->buffer_size - 1;
    
    // Read from delay line
    int read_pos = (filter->write_pos - delay_samples + filter->buffer_size) % filter->buffer_size;
    float delayed = filter->delay_buffer[read_pos];
    
    // Low-pass filter in feedback path for dampening
    filter->filter_state = delayed * (1.0f - filter->damping) + filter->filter_state * filter->damping;
    
    // Write new sample with feedback
   filter->delay_buffer[filter->write_pos] = input + filter->filter_state * filter->feedback_gain;
    
    // Advance write position
    filter->write_pos = (filter->write_pos + 1) % filter->buffer_size;
    
    // Output is combination of input and delayed signal
    return input * filter->feedforward_gain + delayed * filter->feedback_gain;
}

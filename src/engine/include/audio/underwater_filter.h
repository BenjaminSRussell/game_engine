#ifndef UNDERWATER_FILTER_H
#define UNDERWATER_FILTER_H

#include "engine/include/common.h"

void underwater_filter_init(void);

// Process interleaved float samples in-place.
// sample_count is number of floats (frames * channels).
void underwater_filter_process_samples(float *samples, u32 sample_count);

// Enable or disable the underwater audio filter
void underwater_filter_set_enabled(bool enabled);

// Update the filter based on water depth (0.0 = surface, 1.0+ = deep)
void underwater_filter_update_depth(f32 depth);

// Get current filter state
bool underwater_filter_is_enabled(void);

#endif // UNDERWATER_FILTER_H

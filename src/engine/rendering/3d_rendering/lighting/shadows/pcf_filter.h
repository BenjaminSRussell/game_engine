/*
 * pcf_filter.h
 * PCF and Poisson disk sampling
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PCF_FILTER_H
#define LIGHTING_PCF_FILTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate Poisson disk samples in unit circle
 * 
 * @param sample_count Number of samples
 * @param out_samples Output buffer (size = sample_count * 2 floats)
 */
void pcf_generate_poisson_disk(uint32_t sample_count, float* out_samples);

/**
 * @brief Calculate PCF kernel size based on depth
 * 
 * @param view_depth Receiver depth
 * @param receiver_depth Shadow map depth
 * @param light_size Light source physical size
 * @return Kernel radius
 */
float pcf_calculate_kernel_size(float view_depth, float receiver_depth, float light_size);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PCF_FILTER_H */

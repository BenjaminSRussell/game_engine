/*
 * stream_priority.h
 * Streaming priority calculation
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_STREAM_PRIORITY_H
#define TEXTURE_STREAM_PRIORITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * API
 * ============================================================================ */

/* Screen space calculation */
float texture_priority_calculate_screen_coverage(const float* bounds_min, const float* bounds_max, const void* camera);
uint32_t texture_priority_calculate_target_mip(float screen_coverage, uint32_t max_mips, float mip_bias);

/* Distance-based priority */
float texture_priority_calculate_distance_score(const float* position, const void* camera);

/* Priority Query Logic */
float texture_priority_calculate_final_score(float coverage, float distance, uint32_t current_mip, uint32_t target_mip);

/* Internal Priority Queue Utilities */
struct stream_request; 
void texture_priority_sort_requests(struct stream_request* requests, uint32_t count);

/* Original stub compatibility */
int texture_stream_priority_init(void);
void texture_stream_priority_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_STREAM_PRIORITY_H */


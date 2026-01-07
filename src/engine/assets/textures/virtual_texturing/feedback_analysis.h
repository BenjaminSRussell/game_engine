/*
 * feedback_analysis.h
 * Virtual texture feedback analysis
 *
 * Part of the Texture subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef TEXTURE_FEEDBACK_ANALYSIS_H
#define TEXTURE_FEEDBACK_ANALYSIS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct feedback_stats {
    uint32_t unique_pages;
    uint32_t requested_pages;
    uint32_t evicted_pages;
} feedback_stats_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
int feedback_analysis_init(void);
void feedback_analysis_shutdown(void);

/* Operations */
int feedback_analysis_process(const void* feedback_data, uint32_t count);
int feedback_analysis_get_stats(feedback_stats_t* out_stats);

/* Original stub compatibility */
int texture_feedback_analysis_init(void);
void texture_feedback_analysis_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_FEEDBACK_ANALYSIS_H */


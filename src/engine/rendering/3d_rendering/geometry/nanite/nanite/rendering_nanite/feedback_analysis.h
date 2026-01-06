/*
 * feedback_analysis.h
 * Rendering feedback
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_FEEDBACK_ANALYSIS_H
#define NANITE_FEEDBACK_ANALYSIS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_feedback_analysis_handle {
    uint32_t id;
} nanite_feedback_analysis_handle_t;

typedef struct nanite_feedback_analysis_desc {
    uint32_t flags;
    void* user_data;
} nanite_feedback_analysis_desc_t;

typedef struct nanite_feedback_analysis_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_feedback_analysis_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_feedback_analysis_init(void);
void nanite_feedback_analysis_shutdown(void);

/* Lifecycle */
int nanite_feedback_analysis_create(nanite_feedback_analysis_handle_t* out_handle, const nanite_feedback_analysis_desc_t* desc);
void nanite_feedback_analysis_destroy(nanite_feedback_analysis_handle_t handle);

/* Operations */
int nanite_feedback_analysis_update(nanite_feedback_analysis_handle_t handle, const void* data, size_t size);
bool nanite_feedback_analysis_is_valid(nanite_feedback_analysis_handle_t handle);
int nanite_feedback_analysis_get_info(nanite_feedback_analysis_handle_t handle, nanite_feedback_analysis_info_t* out_info);
void nanite_feedback_analysis_mark_dirty(nanite_feedback_analysis_handle_t handle);
int nanite_feedback_analysis_process_pending(void);

/* Statistics */
uint32_t nanite_feedback_analysis_get_count(void);
size_t nanite_feedback_analysis_get_memory_usage(void);
void nanite_feedback_analysis_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_FEEDBACK_ANALYSIS_H */

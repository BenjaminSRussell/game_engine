/*
 * feedback_analysis.h
 * Feedback analysis
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

typedef struct texture_feedback_analysis_handle {
    uint32_t id;
} texture_feedback_analysis_handle_t;

typedef struct texture_feedback_analysis_desc {
    uint32_t flags;
    void* user_data;
} texture_feedback_analysis_desc_t;

typedef struct texture_feedback_analysis_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} texture_feedback_analysis_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int texture_feedback_analysis_init(void);
void texture_feedback_analysis_shutdown(void);

/* Lifecycle */
int texture_feedback_analysis_create(texture_feedback_analysis_handle_t* out_handle, const texture_feedback_analysis_desc_t* desc);
void texture_feedback_analysis_destroy(texture_feedback_analysis_handle_t handle);

/* Operations */
int texture_feedback_analysis_update(texture_feedback_analysis_handle_t handle, const void* data, size_t size);
bool texture_feedback_analysis_is_valid(texture_feedback_analysis_handle_t handle);
int texture_feedback_analysis_get_info(texture_feedback_analysis_handle_t handle, texture_feedback_analysis_info_t* out_info);
void texture_feedback_analysis_mark_dirty(texture_feedback_analysis_handle_t handle);
int texture_feedback_analysis_process_pending(void);

/* Statistics */
uint32_t texture_feedback_analysis_get_count(void);
size_t texture_feedback_analysis_get_memory_usage(void);
void texture_feedback_analysis_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* TEXTURE_FEEDBACK_ANALYSIS_H */

/*
 * cull_feedback.h
 * Culling feedback
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_CULL_FEEDBACK_H
#define NANITE_CULL_FEEDBACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_cull_feedback_handle {
    uint32_t id;
} nanite_cull_feedback_handle_t;

typedef struct nanite_cull_feedback_desc {
    uint32_t flags;
    void* user_data;
} nanite_cull_feedback_desc_t;

typedef struct nanite_cull_feedback_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_cull_feedback_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_cull_feedback_init(void);
void nanite_cull_feedback_shutdown(void);

/* Lifecycle */
int nanite_cull_feedback_create(nanite_cull_feedback_handle_t* out_handle, const nanite_cull_feedback_desc_t* desc);
void nanite_cull_feedback_destroy(nanite_cull_feedback_handle_t handle);

/* Operations */
int nanite_cull_feedback_update(nanite_cull_feedback_handle_t handle, const void* data, size_t size);
bool nanite_cull_feedback_is_valid(nanite_cull_feedback_handle_t handle);
int nanite_cull_feedback_get_info(nanite_cull_feedback_handle_t handle, nanite_cull_feedback_info_t* out_info);
void nanite_cull_feedback_mark_dirty(nanite_cull_feedback_handle_t handle);
int nanite_cull_feedback_process_pending(void);

/* Statistics */
uint32_t nanite_cull_feedback_get_count(void);
size_t nanite_cull_feedback_get_memory_usage(void);
void nanite_cull_feedback_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_CULL_FEEDBACK_H */

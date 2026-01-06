/*
 * occlusion_feedback.h
 * GPU cull feedback
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_OCCLUSION_FEEDBACK_H
#define CULLING_OCCLUSION_FEEDBACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_occlusion_feedback_handle {
    uint32_t id;
} culling_occlusion_feedback_handle_t;

typedef struct culling_occlusion_feedback_desc {
    uint32_t flags;
    void* user_data;
} culling_occlusion_feedback_desc_t;

typedef struct culling_occlusion_feedback_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_occlusion_feedback_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_occlusion_feedback_init(void);
void culling_occlusion_feedback_shutdown(void);

/* Lifecycle */
int culling_occlusion_feedback_create(culling_occlusion_feedback_handle_t* out_handle, const culling_occlusion_feedback_desc_t* desc);
void culling_occlusion_feedback_destroy(culling_occlusion_feedback_handle_t handle);

/* Operations */
int culling_occlusion_feedback_update(culling_occlusion_feedback_handle_t handle, const void* data, size_t size);
bool culling_occlusion_feedback_is_valid(culling_occlusion_feedback_handle_t handle);
int culling_occlusion_feedback_get_info(culling_occlusion_feedback_handle_t handle, culling_occlusion_feedback_info_t* out_info);
void culling_occlusion_feedback_mark_dirty(culling_occlusion_feedback_handle_t handle);
int culling_occlusion_feedback_process_pending(void);

/* Statistics */
uint32_t culling_occlusion_feedback_get_count(void);
size_t culling_occlusion_feedback_get_memory_usage(void);
void culling_occlusion_feedback_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_OCCLUSION_FEEDBACK_H */

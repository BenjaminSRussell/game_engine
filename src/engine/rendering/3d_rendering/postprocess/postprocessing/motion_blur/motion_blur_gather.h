/*
 * motion_blur_gather.h
 * Motion blur gather
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_MOTION_BLUR_GATHER_H
#define POSTPROCESSING_MOTION_BLUR_GATHER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_motion_blur_gather_handle {
    uint32_t id;
} postprocessing_motion_blur_gather_handle_t;

typedef struct postprocessing_motion_blur_gather_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_motion_blur_gather_desc_t;

typedef struct postprocessing_motion_blur_gather_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_motion_blur_gather_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_motion_blur_gather_init(void);
void postprocessing_motion_blur_gather_shutdown(void);

/* Lifecycle */
int postprocessing_motion_blur_gather_create(postprocessing_motion_blur_gather_handle_t* out_handle, const postprocessing_motion_blur_gather_desc_t* desc);
void postprocessing_motion_blur_gather_destroy(postprocessing_motion_blur_gather_handle_t handle);

/* Operations */
int postprocessing_motion_blur_gather_update(postprocessing_motion_blur_gather_handle_t handle, const void* data, size_t size);
bool postprocessing_motion_blur_gather_is_valid(postprocessing_motion_blur_gather_handle_t handle);
int postprocessing_motion_blur_gather_get_info(postprocessing_motion_blur_gather_handle_t handle, postprocessing_motion_blur_gather_info_t* out_info);
void postprocessing_motion_blur_gather_mark_dirty(postprocessing_motion_blur_gather_handle_t handle);
int postprocessing_motion_blur_gather_process_pending(void);

/* Statistics */
uint32_t postprocessing_motion_blur_gather_get_count(void);
size_t postprocessing_motion_blur_gather_get_memory_usage(void);
void postprocessing_motion_blur_gather_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_MOTION_BLUR_GATHER_H */

/*
 * motion_blur_quality.h
 * Motion blur quality levels
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_MOTION_BLUR_QUALITY_H
#define POSTPROCESSING_MOTION_BLUR_QUALITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_motion_blur_quality_handle {
    uint32_t id;
} postprocessing_motion_blur_quality_handle_t;

typedef struct postprocessing_motion_blur_quality_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_motion_blur_quality_desc_t;

typedef struct postprocessing_motion_blur_quality_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_motion_blur_quality_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_motion_blur_quality_init(void);
void postprocessing_motion_blur_quality_shutdown(void);

/* Lifecycle */
int postprocessing_motion_blur_quality_create(postprocessing_motion_blur_quality_handle_t* out_handle, const postprocessing_motion_blur_quality_desc_t* desc);
void postprocessing_motion_blur_quality_destroy(postprocessing_motion_blur_quality_handle_t handle);

/* Operations */
int postprocessing_motion_blur_quality_update(postprocessing_motion_blur_quality_handle_t handle, const void* data, size_t size);
bool postprocessing_motion_blur_quality_is_valid(postprocessing_motion_blur_quality_handle_t handle);
int postprocessing_motion_blur_quality_get_info(postprocessing_motion_blur_quality_handle_t handle, postprocessing_motion_blur_quality_info_t* out_info);
void postprocessing_motion_blur_quality_mark_dirty(postprocessing_motion_blur_quality_handle_t handle);
int postprocessing_motion_blur_quality_process_pending(void);

/* Statistics */
uint32_t postprocessing_motion_blur_quality_get_count(void);
size_t postprocessing_motion_blur_quality_get_memory_usage(void);
void postprocessing_motion_blur_quality_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_MOTION_BLUR_QUALITY_H */

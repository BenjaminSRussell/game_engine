/*
 * camera_motion_blur.h
 * Camera motion blur
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_CAMERA_MOTION_BLUR_H
#define POSTPROCESSING_CAMERA_MOTION_BLUR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_camera_motion_blur_handle {
    uint32_t id;
} postprocessing_camera_motion_blur_handle_t;

typedef struct postprocessing_camera_motion_blur_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_camera_motion_blur_desc_t;

typedef struct postprocessing_camera_motion_blur_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_camera_motion_blur_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_camera_motion_blur_init(void);
void postprocessing_camera_motion_blur_shutdown(void);

/* Lifecycle */
int postprocessing_camera_motion_blur_create(postprocessing_camera_motion_blur_handle_t* out_handle, const postprocessing_camera_motion_blur_desc_t* desc);
void postprocessing_camera_motion_blur_destroy(postprocessing_camera_motion_blur_handle_t handle);

/* Operations */
int postprocessing_camera_motion_blur_update(postprocessing_camera_motion_blur_handle_t handle, const void* data, size_t size);
bool postprocessing_camera_motion_blur_is_valid(postprocessing_camera_motion_blur_handle_t handle);
int postprocessing_camera_motion_blur_get_info(postprocessing_camera_motion_blur_handle_t handle, postprocessing_camera_motion_blur_info_t* out_info);
void postprocessing_camera_motion_blur_mark_dirty(postprocessing_camera_motion_blur_handle_t handle);
int postprocessing_camera_motion_blur_process_pending(void);

/* Statistics */
uint32_t postprocessing_camera_motion_blur_get_count(void);
size_t postprocessing_camera_motion_blur_get_memory_usage(void);
void postprocessing_camera_motion_blur_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_CAMERA_MOTION_BLUR_H */

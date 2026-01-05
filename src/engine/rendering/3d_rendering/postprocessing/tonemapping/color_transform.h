/*
 * color_transform.h
 * Color space transforms
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_COLOR_TRANSFORM_H
#define POSTPROCESSING_COLOR_TRANSFORM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_color_transform_handle {
    uint32_t id;
} postprocessing_color_transform_handle_t;

typedef struct postprocessing_color_transform_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_color_transform_desc_t;

typedef struct postprocessing_color_transform_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_color_transform_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_color_transform_init(void);
void postprocessing_color_transform_shutdown(void);

/* Lifecycle */
int postprocessing_color_transform_create(postprocessing_color_transform_handle_t* out_handle, const postprocessing_color_transform_desc_t* desc);
void postprocessing_color_transform_destroy(postprocessing_color_transform_handle_t handle);

/* Operations */
int postprocessing_color_transform_update(postprocessing_color_transform_handle_t handle, const void* data, size_t size);
bool postprocessing_color_transform_is_valid(postprocessing_color_transform_handle_t handle);
int postprocessing_color_transform_get_info(postprocessing_color_transform_handle_t handle, postprocessing_color_transform_info_t* out_info);
void postprocessing_color_transform_mark_dirty(postprocessing_color_transform_handle_t handle);
int postprocessing_color_transform_process_pending(void);

/* Statistics */
uint32_t postprocessing_color_transform_get_count(void);
size_t postprocessing_color_transform_get_memory_usage(void);
void postprocessing_color_transform_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_COLOR_TRANSFORM_H */

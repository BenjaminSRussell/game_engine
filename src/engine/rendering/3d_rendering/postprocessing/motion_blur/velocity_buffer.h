/*
 * velocity_buffer.h
 * Per-pixel velocity buffer
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_VELOCITY_BUFFER_H
#define POSTPROCESSING_VELOCITY_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_velocity_buffer_handle {
    uint32_t id;
} postprocessing_velocity_buffer_handle_t;

typedef struct postprocessing_velocity_buffer_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_velocity_buffer_desc_t;

typedef struct postprocessing_velocity_buffer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_velocity_buffer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_velocity_buffer_init(void);
void postprocessing_velocity_buffer_shutdown(void);

/* Lifecycle */
int postprocessing_velocity_buffer_create(postprocessing_velocity_buffer_handle_t* out_handle, const postprocessing_velocity_buffer_desc_t* desc);
void postprocessing_velocity_buffer_destroy(postprocessing_velocity_buffer_handle_t handle);

/* Operations */
int postprocessing_velocity_buffer_update(postprocessing_velocity_buffer_handle_t handle, const void* data, size_t size);
bool postprocessing_velocity_buffer_is_valid(postprocessing_velocity_buffer_handle_t handle);
int postprocessing_velocity_buffer_get_info(postprocessing_velocity_buffer_handle_t handle, postprocessing_velocity_buffer_info_t* out_info);
void postprocessing_velocity_buffer_mark_dirty(postprocessing_velocity_buffer_handle_t handle);
int postprocessing_velocity_buffer_process_pending(void);

/* Statistics */
uint32_t postprocessing_velocity_buffer_get_count(void);
size_t postprocessing_velocity_buffer_get_memory_usage(void);
void postprocessing_velocity_buffer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_VELOCITY_BUFFER_H */

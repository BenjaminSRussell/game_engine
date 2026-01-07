/*
 * frame_pacing.h
 * Frame timing/pacing
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_FRAME_PACING_H
#define RENDERING_FRAME_PACING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_frame_pacing_handle {
    uint32_t id;
} rendering_frame_pacing_handle_t;

typedef struct rendering_frame_pacing_desc {
    uint32_t flags;
    void* user_data;
} rendering_frame_pacing_desc_t;

typedef struct rendering_frame_pacing_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_frame_pacing_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_frame_pacing_init(void);
void rendering_frame_pacing_shutdown(void);

/* Lifecycle */
int rendering_frame_pacing_create(rendering_frame_pacing_handle_t* out_handle, const rendering_frame_pacing_desc_t* desc);
void rendering_frame_pacing_destroy(rendering_frame_pacing_handle_t handle);

/* Operations */
int rendering_frame_pacing_update(rendering_frame_pacing_handle_t handle, const void* data, size_t size);
bool rendering_frame_pacing_is_valid(rendering_frame_pacing_handle_t handle);
int rendering_frame_pacing_get_info(rendering_frame_pacing_handle_t handle, rendering_frame_pacing_info_t* out_info);
void rendering_frame_pacing_mark_dirty(rendering_frame_pacing_handle_t handle);
int rendering_frame_pacing_process_pending(void);

/* Statistics */
uint32_t rendering_frame_pacing_get_count(void);
size_t rendering_frame_pacing_get_memory_usage(void);
void rendering_frame_pacing_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_FRAME_PACING_H */

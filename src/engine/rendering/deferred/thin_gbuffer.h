/*
 * thin_gbuffer.h
 * Thin G-buffer mode
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_THIN_GBUFFER_H
#define RENDERING_THIN_GBUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_thin_gbuffer_handle {
    uint32_t id;
} rendering_thin_gbuffer_handle_t;

typedef struct rendering_thin_gbuffer_desc {
    uint32_t flags;
    void* user_data;
} rendering_thin_gbuffer_desc_t;

typedef struct rendering_thin_gbuffer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_thin_gbuffer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_thin_gbuffer_init(void);
void rendering_thin_gbuffer_shutdown(void);

/* Lifecycle */
int rendering_thin_gbuffer_create(rendering_thin_gbuffer_handle_t* out_handle, const rendering_thin_gbuffer_desc_t* desc);
void rendering_thin_gbuffer_destroy(rendering_thin_gbuffer_handle_t handle);

/* Operations */
int rendering_thin_gbuffer_update(rendering_thin_gbuffer_handle_t handle, const void* data, size_t size);
bool rendering_thin_gbuffer_is_valid(rendering_thin_gbuffer_handle_t handle);
int rendering_thin_gbuffer_get_info(rendering_thin_gbuffer_handle_t handle, rendering_thin_gbuffer_info_t* out_info);
void rendering_thin_gbuffer_mark_dirty(rendering_thin_gbuffer_handle_t handle);
int rendering_thin_gbuffer_process_pending(void);

/* Statistics */
uint32_t rendering_thin_gbuffer_get_count(void);
size_t rendering_thin_gbuffer_get_memory_usage(void);
void rendering_thin_gbuffer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_THIN_GBUFFER_H */

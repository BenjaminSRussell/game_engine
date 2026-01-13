/*
 * content_adaptive.h
 * Content adaptive VRS
 *
 * Part of the Vrs subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef VRS_CONTENT_ADAPTIVE_H
#define VRS_CONTENT_ADAPTIVE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct vrs_content_adaptive_handle {
    uint32_t id;
} vrs_content_adaptive_handle_t;

typedef struct vrs_content_adaptive_desc {
    uint32_t flags;
    void* user_data;
} vrs_content_adaptive_desc_t;

typedef struct vrs_content_adaptive_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} vrs_content_adaptive_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int vrs_content_adaptive_init(void);
void vrs_content_adaptive_shutdown(void);

/* Lifecycle */
int vrs_content_adaptive_create(vrs_content_adaptive_handle_t* out_handle, const vrs_content_adaptive_desc_t* desc);
void vrs_content_adaptive_destroy(vrs_content_adaptive_handle_t handle);

/* Operations */
int vrs_content_adaptive_update(vrs_content_adaptive_handle_t handle, const void* data, size_t size);
bool vrs_content_adaptive_is_valid(vrs_content_adaptive_handle_t handle);
int vrs_content_adaptive_get_info(vrs_content_adaptive_handle_t handle, vrs_content_adaptive_info_t* out_info);
void vrs_content_adaptive_mark_dirty(vrs_content_adaptive_handle_t handle);
int vrs_content_adaptive_process_pending(void);

/* Statistics */
uint32_t vrs_content_adaptive_get_count(void);
size_t vrs_content_adaptive_get_memory_usage(void);
void vrs_content_adaptive_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* VRS_CONTENT_ADAPTIVE_H */

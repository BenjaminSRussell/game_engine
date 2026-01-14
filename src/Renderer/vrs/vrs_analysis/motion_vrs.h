/*
 * motion_vrs.h
 * Motion-based VRS
 *
 * Part of the Vrs subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef VRS_MOTION_VRS_H
#define VRS_MOTION_VRS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct vrs_motion_vrs_handle {
    uint32_t id;
} vrs_motion_vrs_handle_t;

typedef struct vrs_motion_vrs_desc {
    uint32_t flags;
    void* user_data;
} vrs_motion_vrs_desc_t;

typedef struct vrs_motion_vrs_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} vrs_motion_vrs_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int vrs_motion_vrs_init(void);
void vrs_motion_vrs_shutdown(void);

/* Lifecycle */
int vrs_motion_vrs_create(vrs_motion_vrs_handle_t* out_handle, const vrs_motion_vrs_desc_t* desc);
void vrs_motion_vrs_destroy(vrs_motion_vrs_handle_t handle);

/* Operations */
int vrs_motion_vrs_update(vrs_motion_vrs_handle_t handle, const void* data, size_t size);
bool vrs_motion_vrs_is_valid(vrs_motion_vrs_handle_t handle);
int vrs_motion_vrs_get_info(vrs_motion_vrs_handle_t handle, vrs_motion_vrs_info_t* out_info);
void vrs_motion_vrs_mark_dirty(vrs_motion_vrs_handle_t handle);
int vrs_motion_vrs_process_pending(void);

/* Statistics */
uint32_t vrs_motion_vrs_get_count(void);
size_t vrs_motion_vrs_get_memory_usage(void);
void vrs_motion_vrs_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* VRS_MOTION_VRS_H */

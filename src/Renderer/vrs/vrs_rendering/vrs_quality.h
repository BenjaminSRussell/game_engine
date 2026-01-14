/*
 * vrs_quality.h
 * VRS quality modes
 *
 * Part of the Vrs subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef VRS_VRS_QUALITY_H
#define VRS_VRS_QUALITY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct vrs_vrs_quality_handle {
    uint32_t id;
} vrs_vrs_quality_handle_t;

typedef struct vrs_vrs_quality_desc {
    uint32_t flags;
    void* user_data;
} vrs_vrs_quality_desc_t;

typedef struct vrs_vrs_quality_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} vrs_vrs_quality_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int vrs_vrs_quality_init(void);
void vrs_vrs_quality_shutdown(void);

/* Lifecycle */
int vrs_vrs_quality_create(vrs_vrs_quality_handle_t* out_handle, const vrs_vrs_quality_desc_t* desc);
void vrs_vrs_quality_destroy(vrs_vrs_quality_handle_t handle);

/* Operations */
int vrs_vrs_quality_update(vrs_vrs_quality_handle_t handle, const void* data, size_t size);
bool vrs_vrs_quality_is_valid(vrs_vrs_quality_handle_t handle);
int vrs_vrs_quality_get_info(vrs_vrs_quality_handle_t handle, vrs_vrs_quality_info_t* out_info);
void vrs_vrs_quality_mark_dirty(vrs_vrs_quality_handle_t handle);
int vrs_vrs_quality_process_pending(void);

/* Statistics */
uint32_t vrs_vrs_quality_get_count(void);
size_t vrs_vrs_quality_get_memory_usage(void);
void vrs_vrs_quality_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* VRS_VRS_QUALITY_H */

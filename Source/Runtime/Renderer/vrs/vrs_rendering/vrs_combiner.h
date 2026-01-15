/*
 * vrs_combiner.h
 * VRS combiner
 *
 * Part of the Vrs subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef VRS_VRS_COMBINER_H
#define VRS_VRS_COMBINER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct vrs_vrs_combiner_handle {
    uint32_t id;
} vrs_vrs_combiner_handle_t;

typedef struct vrs_vrs_combiner_desc {
    uint32_t flags;
    void* user_data;
} vrs_vrs_combiner_desc_t;

typedef struct vrs_vrs_combiner_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} vrs_vrs_combiner_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int vrs_vrs_combiner_init(void);
void vrs_vrs_combiner_shutdown(void);

/* Lifecycle */
int vrs_vrs_combiner_create(vrs_vrs_combiner_handle_t* out_handle, const vrs_vrs_combiner_desc_t* desc);
void vrs_vrs_combiner_destroy(vrs_vrs_combiner_handle_t handle);

/* Operations */
int vrs_vrs_combiner_update(vrs_vrs_combiner_handle_t handle, const void* data, size_t size);
bool vrs_vrs_combiner_is_valid(vrs_vrs_combiner_handle_t handle);
int vrs_vrs_combiner_get_info(vrs_vrs_combiner_handle_t handle, vrs_vrs_combiner_info_t* out_info);
void vrs_vrs_combiner_mark_dirty(vrs_vrs_combiner_handle_t handle);
int vrs_vrs_combiner_process_pending(void);

/* Statistics */
uint32_t vrs_vrs_combiner_get_count(void);
size_t vrs_vrs_combiner_get_memory_usage(void);
void vrs_vrs_combiner_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* VRS_VRS_COMBINER_H */

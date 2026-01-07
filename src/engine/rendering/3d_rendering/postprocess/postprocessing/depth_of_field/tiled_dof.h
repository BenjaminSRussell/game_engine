/*
 * tiled_dof.h
 * Tiled DOF optimization
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_TILED_DOF_H
#define POSTPROCESSING_TILED_DOF_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_tiled_dof_handle {
    uint32_t id;
} postprocessing_tiled_dof_handle_t;

typedef struct postprocessing_tiled_dof_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_tiled_dof_desc_t;

typedef struct postprocessing_tiled_dof_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_tiled_dof_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_tiled_dof_init(void);
void postprocessing_tiled_dof_shutdown(void);

/* Lifecycle */
int postprocessing_tiled_dof_create(postprocessing_tiled_dof_handle_t* out_handle, const postprocessing_tiled_dof_desc_t* desc);
void postprocessing_tiled_dof_destroy(postprocessing_tiled_dof_handle_t handle);

/* Operations */
int postprocessing_tiled_dof_update(postprocessing_tiled_dof_handle_t handle, const void* data, size_t size);
bool postprocessing_tiled_dof_is_valid(postprocessing_tiled_dof_handle_t handle);
int postprocessing_tiled_dof_get_info(postprocessing_tiled_dof_handle_t handle, postprocessing_tiled_dof_info_t* out_info);
void postprocessing_tiled_dof_mark_dirty(postprocessing_tiled_dof_handle_t handle);
int postprocessing_tiled_dof_process_pending(void);

/* Statistics */
uint32_t postprocessing_tiled_dof_get_count(void);
size_t postprocessing_tiled_dof_get_memory_usage(void);
void postprocessing_tiled_dof_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_TILED_DOF_H */

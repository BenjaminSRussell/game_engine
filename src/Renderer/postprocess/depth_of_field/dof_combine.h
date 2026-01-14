/*
 * dof_combine.h
 * DOF combine pass
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_DOF_COMBINE_H
#define POSTPROCESSING_DOF_COMBINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_dof_combine_handle {
    uint32_t id;
} postprocessing_dof_combine_handle_t;

typedef struct postprocessing_dof_combine_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_dof_combine_desc_t;

typedef struct postprocessing_dof_combine_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_dof_combine_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_dof_combine_init(void);
void postprocessing_dof_combine_shutdown(void);

/* Lifecycle */
int postprocessing_dof_combine_create(postprocessing_dof_combine_handle_t* out_handle, const postprocessing_dof_combine_desc_t* desc);
void postprocessing_dof_combine_destroy(postprocessing_dof_combine_handle_t handle);

/* Operations */
int postprocessing_dof_combine_update(postprocessing_dof_combine_handle_t handle, const void* data, size_t size);
bool postprocessing_dof_combine_is_valid(postprocessing_dof_combine_handle_t handle);
int postprocessing_dof_combine_get_info(postprocessing_dof_combine_handle_t handle, postprocessing_dof_combine_info_t* out_info);
void postprocessing_dof_combine_mark_dirty(postprocessing_dof_combine_handle_t handle);
int postprocessing_dof_combine_process_pending(void);

/* Statistics */
uint32_t postprocessing_dof_combine_get_count(void);
size_t postprocessing_dof_combine_get_memory_usage(void);
void postprocessing_dof_combine_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_DOF_COMBINE_H */

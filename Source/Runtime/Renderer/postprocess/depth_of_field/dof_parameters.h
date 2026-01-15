/*
 * dof_parameters.h
 * DOF camera settings
 *
 * Part of the Postprocessing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef POSTPROCESSING_DOF_PARAMETERS_H
#define POSTPROCESSING_DOF_PARAMETERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct postprocessing_dof_parameters_handle {
    uint32_t id;
} postprocessing_dof_parameters_handle_t;

typedef struct postprocessing_dof_parameters_desc {
    uint32_t flags;
    void* user_data;
} postprocessing_dof_parameters_desc_t;

typedef struct postprocessing_dof_parameters_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} postprocessing_dof_parameters_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int postprocessing_dof_parameters_init(void);
void postprocessing_dof_parameters_shutdown(void);

/* Lifecycle */
int postprocessing_dof_parameters_create(postprocessing_dof_parameters_handle_t* out_handle, const postprocessing_dof_parameters_desc_t* desc);
void postprocessing_dof_parameters_destroy(postprocessing_dof_parameters_handle_t handle);

/* Operations */
int postprocessing_dof_parameters_update(postprocessing_dof_parameters_handle_t handle, const void* data, size_t size);
bool postprocessing_dof_parameters_is_valid(postprocessing_dof_parameters_handle_t handle);
int postprocessing_dof_parameters_get_info(postprocessing_dof_parameters_handle_t handle, postprocessing_dof_parameters_info_t* out_info);
void postprocessing_dof_parameters_mark_dirty(postprocessing_dof_parameters_handle_t handle);
int postprocessing_dof_parameters_process_pending(void);

/* Statistics */
uint32_t postprocessing_dof_parameters_get_count(void);
size_t postprocessing_dof_parameters_get_memory_usage(void);
void postprocessing_dof_parameters_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* POSTPROCESSING_DOF_PARAMETERS_H */

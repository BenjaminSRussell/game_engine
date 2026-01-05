/*
 * ccd_solver.h
 * CCD IK solver
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_CCD_SOLVER_H
#define ANIMATION_CCD_SOLVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_ccd_solver_handle {
    uint32_t id;
} animation_ccd_solver_handle_t;

typedef struct animation_ccd_solver_desc {
    uint32_t flags;
    void* user_data;
} animation_ccd_solver_desc_t;

typedef struct animation_ccd_solver_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_ccd_solver_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_ccd_solver_init(void);
void animation_ccd_solver_shutdown(void);

/* Lifecycle */
int animation_ccd_solver_create(animation_ccd_solver_handle_t* out_handle, const animation_ccd_solver_desc_t* desc);
void animation_ccd_solver_destroy(animation_ccd_solver_handle_t handle);

/* Operations */
int animation_ccd_solver_update(animation_ccd_solver_handle_t handle, const void* data, size_t size);
bool animation_ccd_solver_is_valid(animation_ccd_solver_handle_t handle);
int animation_ccd_solver_get_info(animation_ccd_solver_handle_t handle, animation_ccd_solver_info_t* out_info);
void animation_ccd_solver_mark_dirty(animation_ccd_solver_handle_t handle);
int animation_ccd_solver_process_pending(void);

/* Statistics */
uint32_t animation_ccd_solver_get_count(void);
size_t animation_ccd_solver_get_memory_usage(void);
void animation_ccd_solver_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_CCD_SOLVER_H */

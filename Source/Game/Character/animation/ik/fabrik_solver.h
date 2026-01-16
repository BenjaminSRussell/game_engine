/*
 * fabrik_solver.h
 * FABRIK IK solver
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_FABRIK_SOLVER_H
#define ANIMATION_FABRIK_SOLVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "include/math/math_all.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_fabrik_solver_handle {
    uint32_t id;
} animation_fabrik_solver_handle_t;

typedef struct animation_fabrik_solver_desc {
    uint32_t flags;
    void* user_data;
} animation_fabrik_solver_desc_t;

typedef struct animation_fabrik_solver_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_fabrik_solver_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_fabrik_solver_init(void);
void animation_fabrik_solver_shutdown(void);

/* Lifecycle */
int animation_fabrik_solver_create(animation_fabrik_solver_handle_t* out_handle, const animation_fabrik_solver_desc_t* desc);
void animation_fabrik_solver_destroy(animation_fabrik_solver_handle_t handle);

/* Operations */
int animation_fabrik_solver_update(animation_fabrik_solver_handle_t handle, const void* data, size_t size);
bool animation_fabrik_solver_is_valid(animation_fabrik_solver_handle_t handle);
int animation_fabrik_solver_get_info(animation_fabrik_solver_handle_t handle, animation_fabrik_solver_info_t* out_info);
void animation_fabrik_solver_mark_dirty(animation_fabrik_solver_handle_t handle);
int animation_fabrik_solver_process_pending(void);

/* Statistics */
uint32_t animation_fabrik_solver_get_count(void);
size_t animation_fabrik_solver_get_memory_usage(void);
void animation_fabrik_solver_debug_print(void);

/* FABRIK solving functions */
uint32_t animation_fabrik_solver_add_chain(animation_fabrik_solver_handle_t handle, 
                                           const Vec3* positions, uint32_t joint_count);
bool animation_fabrik_solver_solve_chain(animation_fabrik_solver_handle_t handle, 
                                        uint32_t chain_id, const Vec3* target, 
                                        int max_iterations);
Vec3 animation_fabrik_solver_get_joint_position(animation_fabrik_solver_handle_t handle, 
                                                uint32_t chain_id, uint32_t joint_index);
void animation_fabrik_solver_set_joint_constraint(animation_fabrik_solver_handle_t handle, 
                                                  uint32_t chain_id, uint32_t joint_index,
                                                  float min_angle, float max_angle);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_FABRIK_SOLVER_H */

/*
 * foot_ik.h
 * Foot placement IK
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_FOOT_IK_H
#define ANIMATION_FOOT_IK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_foot_ik_handle {
    uint32_t id;
} animation_foot_ik_handle_t;

typedef struct animation_foot_ik_desc {
    uint32_t flags;
    void* user_data;
} animation_foot_ik_desc_t;

typedef struct animation_foot_ik_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_foot_ik_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_foot_ik_init(void);
void animation_foot_ik_shutdown(void);

/* Lifecycle */
int animation_foot_ik_create(animation_foot_ik_handle_t* out_handle, const animation_foot_ik_desc_t* desc);
void animation_foot_ik_destroy(animation_foot_ik_handle_t handle);

/* Operations */
int animation_foot_ik_update(animation_foot_ik_handle_t handle, const void* data, size_t size);
bool animation_foot_ik_is_valid(animation_foot_ik_handle_t handle);
int animation_foot_ik_get_info(animation_foot_ik_handle_t handle, animation_foot_ik_info_t* out_info);
void animation_foot_ik_mark_dirty(animation_foot_ik_handle_t handle);
int animation_foot_ik_process_pending(void);

/* Statistics */
uint32_t animation_foot_ik_get_count(void);
size_t animation_foot_ik_get_memory_usage(void);
void animation_foot_ik_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_FOOT_IK_H */

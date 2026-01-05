/*
 * look_at_ik.h
 * Look-at constraint
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_LOOK_AT_IK_H
#define ANIMATION_LOOK_AT_IK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_look_at_ik_handle {
    uint32_t id;
} animation_look_at_ik_handle_t;

typedef struct animation_look_at_ik_desc {
    uint32_t flags;
    void* user_data;
} animation_look_at_ik_desc_t;

typedef struct animation_look_at_ik_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_look_at_ik_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_look_at_ik_init(void);
void animation_look_at_ik_shutdown(void);

/* Lifecycle */
int animation_look_at_ik_create(animation_look_at_ik_handle_t* out_handle, const animation_look_at_ik_desc_t* desc);
void animation_look_at_ik_destroy(animation_look_at_ik_handle_t handle);

/* Operations */
int animation_look_at_ik_update(animation_look_at_ik_handle_t handle, const void* data, size_t size);
bool animation_look_at_ik_is_valid(animation_look_at_ik_handle_t handle);
int animation_look_at_ik_get_info(animation_look_at_ik_handle_t handle, animation_look_at_ik_info_t* out_info);
void animation_look_at_ik_mark_dirty(animation_look_at_ik_handle_t handle);
int animation_look_at_ik_process_pending(void);

/* Statistics */
uint32_t animation_look_at_ik_get_count(void);
size_t animation_look_at_ik_get_memory_usage(void);
void animation_look_at_ik_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_LOOK_AT_IK_H */

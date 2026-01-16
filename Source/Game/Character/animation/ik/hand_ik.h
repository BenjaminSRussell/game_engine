/*
 * hand_ik.h
 * Hand IK for grabbing
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_HAND_IK_H
#define ANIMATION_HAND_IK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_hand_ik_handle {
    uint32_t id;
} animation_hand_ik_handle_t;

typedef struct animation_hand_ik_desc {
    uint32_t flags;
    void* user_data;
} animation_hand_ik_desc_t;

typedef struct animation_hand_ik_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_hand_ik_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_hand_ik_init(void);
void animation_hand_ik_shutdown(void);

/* Lifecycle */
int animation_hand_ik_create(animation_hand_ik_handle_t* out_handle, const animation_hand_ik_desc_t* desc);
void animation_hand_ik_destroy(animation_hand_ik_handle_t handle);

/* Operations */
int animation_hand_ik_update(animation_hand_ik_handle_t handle, const void* data, size_t size);
bool animation_hand_ik_is_valid(animation_hand_ik_handle_t handle);
int animation_hand_ik_get_info(animation_hand_ik_handle_t handle, animation_hand_ik_info_t* out_info);
void animation_hand_ik_mark_dirty(animation_hand_ik_handle_t handle);
int animation_hand_ik_process_pending(void);

/* Statistics */
uint32_t animation_hand_ik_get_count(void);
size_t animation_hand_ik_get_memory_usage(void);
void animation_hand_ik_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_HAND_IK_H */

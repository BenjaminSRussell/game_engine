/*
 * sub_state_machine.h
 * Nested state machines
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_SUB_STATE_MACHINE_H
#define ANIMATION_SUB_STATE_MACHINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_sub_state_machine_handle {
    uint32_t id;
} animation_sub_state_machine_handle_t;

typedef struct animation_sub_state_machine_desc {
    uint32_t flags;
    void* user_data;
} animation_sub_state_machine_desc_t;

typedef struct animation_sub_state_machine_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_sub_state_machine_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_sub_state_machine_init(void);
void animation_sub_state_machine_shutdown(void);

/* Lifecycle */
int animation_sub_state_machine_create(animation_sub_state_machine_handle_t* out_handle, const animation_sub_state_machine_desc_t* desc);
void animation_sub_state_machine_destroy(animation_sub_state_machine_handle_t handle);

/* Operations */
int animation_sub_state_machine_update(animation_sub_state_machine_handle_t handle, const void* data, size_t size);
bool animation_sub_state_machine_is_valid(animation_sub_state_machine_handle_t handle);
int animation_sub_state_machine_get_info(animation_sub_state_machine_handle_t handle, animation_sub_state_machine_info_t* out_info);
void animation_sub_state_machine_mark_dirty(animation_sub_state_machine_handle_t handle);
int animation_sub_state_machine_process_pending(void);

/* Statistics */
uint32_t animation_sub_state_machine_get_count(void);
size_t animation_sub_state_machine_get_memory_usage(void);
void animation_sub_state_machine_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_SUB_STATE_MACHINE_H */

/*
 * transition_conditions.h
 * Transition parameters
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_TRANSITION_CONDITIONS_H
#define ANIMATION_TRANSITION_CONDITIONS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_transition_conditions_handle {
    uint32_t id;
} animation_transition_conditions_handle_t;

typedef struct animation_transition_conditions_desc {
    uint32_t flags;
    void* user_data;
} animation_transition_conditions_desc_t;

typedef struct animation_transition_conditions_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_transition_conditions_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_transition_conditions_init(void);
void animation_transition_conditions_shutdown(void);

/* Lifecycle */
int animation_transition_conditions_create(animation_transition_conditions_handle_t* out_handle, const animation_transition_conditions_desc_t* desc);
void animation_transition_conditions_destroy(animation_transition_conditions_handle_t handle);

/* Operations */
int animation_transition_conditions_update(animation_transition_conditions_handle_t handle, const void* data, size_t size);
bool animation_transition_conditions_is_valid(animation_transition_conditions_handle_t handle);
int animation_transition_conditions_get_info(animation_transition_conditions_handle_t handle, animation_transition_conditions_info_t* out_info);
void animation_transition_conditions_mark_dirty(animation_transition_conditions_handle_t handle);
int animation_transition_conditions_process_pending(void);

/* Statistics */
uint32_t animation_transition_conditions_get_count(void);
size_t animation_transition_conditions_get_memory_usage(void);
void animation_transition_conditions_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_TRANSITION_CONDITIONS_H */

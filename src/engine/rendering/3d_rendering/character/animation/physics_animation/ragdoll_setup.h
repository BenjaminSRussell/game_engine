/*
 * ragdoll_setup.h
 * Ragdoll skeleton setup
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_RAGDOLL_SETUP_H
#define ANIMATION_RAGDOLL_SETUP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_ragdoll_setup_handle {
    uint32_t id;
} animation_ragdoll_setup_handle_t;

typedef struct animation_ragdoll_setup_desc {
    uint32_t flags;
    void* user_data;
} animation_ragdoll_setup_desc_t;

typedef struct animation_ragdoll_setup_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_ragdoll_setup_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_ragdoll_setup_init(void);
void animation_ragdoll_setup_shutdown(void);

/* Lifecycle */
int animation_ragdoll_setup_create(animation_ragdoll_setup_handle_t* out_handle, const animation_ragdoll_setup_desc_t* desc);
void animation_ragdoll_setup_destroy(animation_ragdoll_setup_handle_t handle);

/* Operations */
int animation_ragdoll_setup_update(animation_ragdoll_setup_handle_t handle, const void* data, size_t size);
bool animation_ragdoll_setup_is_valid(animation_ragdoll_setup_handle_t handle);
int animation_ragdoll_setup_get_info(animation_ragdoll_setup_handle_t handle, animation_ragdoll_setup_info_t* out_info);
void animation_ragdoll_setup_mark_dirty(animation_ragdoll_setup_handle_t handle);
int animation_ragdoll_setup_process_pending(void);

/* Statistics */
uint32_t animation_ragdoll_setup_get_count(void);
size_t animation_ragdoll_setup_get_memory_usage(void);
void animation_ragdoll_setup_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_RAGDOLL_SETUP_H */

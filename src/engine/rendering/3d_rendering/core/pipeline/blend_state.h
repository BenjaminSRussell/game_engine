/*
 * blend_state.h
 * Color blending configuration
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_BLEND_STATE_H
#define CORE_BLEND_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_blend_state_handle {
    uint32_t id;
} core_blend_state_handle_t;

typedef struct core_blend_state_desc {
    uint32_t flags;
    void* user_data;
} core_blend_state_desc_t;

typedef struct core_blend_state_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_blend_state_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_blend_state_init(void);
void core_blend_state_shutdown(void);

/* Lifecycle */
int core_blend_state_create(core_blend_state_handle_t* out_handle, const core_blend_state_desc_t* desc);
void core_blend_state_destroy(core_blend_state_handle_t handle);

/* Operations */
int core_blend_state_update(core_blend_state_handle_t handle, const void* data, size_t size);
bool core_blend_state_is_valid(core_blend_state_handle_t handle);
int core_blend_state_get_info(core_blend_state_handle_t handle, core_blend_state_info_t* out_info);
void core_blend_state_mark_dirty(core_blend_state_handle_t handle);
int core_blend_state_process_pending(void);

/* Statistics */
uint32_t core_blend_state_get_count(void);
size_t core_blend_state_get_memory_usage(void);
void core_blend_state_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BLEND_STATE_H */

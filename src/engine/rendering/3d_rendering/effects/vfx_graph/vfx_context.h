/*
 * vfx_context.h
 * VFX graph context
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_CONTEXT_H
#define EFFECTS_VFX_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_vfx_context_handle {
    uint32_t id;
} effects_vfx_context_handle_t;

typedef struct effects_vfx_context_desc {
    uint32_t flags;
    void* user_data;
} effects_vfx_context_desc_t;

typedef struct effects_vfx_context_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_vfx_context_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_vfx_context_init(void);
void effects_vfx_context_shutdown(void);

/* Lifecycle */
int effects_vfx_context_create(effects_vfx_context_handle_t* out_handle, const effects_vfx_context_desc_t* desc);
void effects_vfx_context_destroy(effects_vfx_context_handle_t handle);

/* Operations */
int effects_vfx_context_update(effects_vfx_context_handle_t handle, const void* data, size_t size);
bool effects_vfx_context_is_valid(effects_vfx_context_handle_t handle);
int effects_vfx_context_get_info(effects_vfx_context_handle_t handle, effects_vfx_context_info_t* out_info);
void effects_vfx_context_mark_dirty(effects_vfx_context_handle_t handle);
int effects_vfx_context_process_pending(void);

/* Statistics */
uint32_t effects_vfx_context_get_count(void);
size_t effects_vfx_context_get_memory_usage(void);
void effects_vfx_context_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_CONTEXT_H */

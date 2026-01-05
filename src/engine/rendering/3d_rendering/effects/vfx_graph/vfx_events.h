/*
 * vfx_events.h
 * VFX event triggers
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_VFX_EVENTS_H
#define EFFECTS_VFX_EVENTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_vfx_events_handle {
    uint32_t id;
} effects_vfx_events_handle_t;

typedef struct effects_vfx_events_desc {
    uint32_t flags;
    void* user_data;
} effects_vfx_events_desc_t;

typedef struct effects_vfx_events_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_vfx_events_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_vfx_events_init(void);
void effects_vfx_events_shutdown(void);

/* Lifecycle */
int effects_vfx_events_create(effects_vfx_events_handle_t* out_handle, const effects_vfx_events_desc_t* desc);
void effects_vfx_events_destroy(effects_vfx_events_handle_t handle);

/* Operations */
int effects_vfx_events_update(effects_vfx_events_handle_t handle, const void* data, size_t size);
bool effects_vfx_events_is_valid(effects_vfx_events_handle_t handle);
int effects_vfx_events_get_info(effects_vfx_events_handle_t handle, effects_vfx_events_info_t* out_info);
void effects_vfx_events_mark_dirty(effects_vfx_events_handle_t handle);
int effects_vfx_events_process_pending(void);

/* Statistics */
uint32_t effects_vfx_events_get_count(void);
size_t effects_vfx_events_get_memory_usage(void);
void effects_vfx_events_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_VFX_EVENTS_H */

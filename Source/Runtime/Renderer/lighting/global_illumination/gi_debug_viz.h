/*
 * gi_debug_viz.h
 * GI visualization/debugging
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_GI_DEBUG_VIZ_H
#define LIGHTING_GI_DEBUG_VIZ_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_gi_debug_viz_handle {
    uint32_t id;
} lighting_gi_debug_viz_handle_t;

typedef struct lighting_gi_debug_viz_desc {
    uint32_t flags;
    void* user_data;
} lighting_gi_debug_viz_desc_t;

typedef struct lighting_gi_debug_viz_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_gi_debug_viz_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_gi_debug_viz_init(void);
void lighting_gi_debug_viz_shutdown(void);

/* Lifecycle */
int lighting_gi_debug_viz_create(lighting_gi_debug_viz_handle_t* out_handle, const lighting_gi_debug_viz_desc_t* desc);
void lighting_gi_debug_viz_destroy(lighting_gi_debug_viz_handle_t handle);

/* Operations */
int lighting_gi_debug_viz_update(lighting_gi_debug_viz_handle_t handle, const void* data, size_t size);
bool lighting_gi_debug_viz_is_valid(lighting_gi_debug_viz_handle_t handle);
int lighting_gi_debug_viz_get_info(lighting_gi_debug_viz_handle_t handle, lighting_gi_debug_viz_info_t* out_info);
void lighting_gi_debug_viz_mark_dirty(lighting_gi_debug_viz_handle_t handle);
int lighting_gi_debug_viz_process_pending(void);

/* Statistics */
uint32_t lighting_gi_debug_viz_get_count(void);
size_t lighting_gi_debug_viz_get_memory_usage(void);
void lighting_gi_debug_viz_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_GI_DEBUG_VIZ_H */

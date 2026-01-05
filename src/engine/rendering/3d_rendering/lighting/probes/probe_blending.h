/*
 * probe_blending.h
 * Multi-probe blending
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PROBE_BLENDING_H
#define LIGHTING_PROBE_BLENDING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_probe_blending_handle {
    uint32_t id;
} lighting_probe_blending_handle_t;

typedef struct lighting_probe_blending_desc {
    uint32_t flags;
    void* user_data;
} lighting_probe_blending_desc_t;

typedef struct lighting_probe_blending_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_probe_blending_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_probe_blending_init(void);
void lighting_probe_blending_shutdown(void);

/* Lifecycle */
int lighting_probe_blending_create(lighting_probe_blending_handle_t* out_handle, const lighting_probe_blending_desc_t* desc);
void lighting_probe_blending_destroy(lighting_probe_blending_handle_t handle);

/* Operations */
int lighting_probe_blending_update(lighting_probe_blending_handle_t handle, const void* data, size_t size);
bool lighting_probe_blending_is_valid(lighting_probe_blending_handle_t handle);
int lighting_probe_blending_get_info(lighting_probe_blending_handle_t handle, lighting_probe_blending_info_t* out_info);
void lighting_probe_blending_mark_dirty(lighting_probe_blending_handle_t handle);
int lighting_probe_blending_process_pending(void);

/* Statistics */
uint32_t lighting_probe_blending_get_count(void);
size_t lighting_probe_blending_get_memory_usage(void);
void lighting_probe_blending_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PROBE_BLENDING_H */

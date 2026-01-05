/*
 * probe_parallax.h
 * Parallax-corrected probes
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_PROBE_PARALLAX_H
#define LIGHTING_PROBE_PARALLAX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_probe_parallax_handle {
    uint32_t id;
} lighting_probe_parallax_handle_t;

typedef struct lighting_probe_parallax_desc {
    uint32_t flags;
    void* user_data;
} lighting_probe_parallax_desc_t;

typedef struct lighting_probe_parallax_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_probe_parallax_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_probe_parallax_init(void);
void lighting_probe_parallax_shutdown(void);

/* Lifecycle */
int lighting_probe_parallax_create(lighting_probe_parallax_handle_t* out_handle, const lighting_probe_parallax_desc_t* desc);
void lighting_probe_parallax_destroy(lighting_probe_parallax_handle_t handle);

/* Operations */
int lighting_probe_parallax_update(lighting_probe_parallax_handle_t handle, const void* data, size_t size);
bool lighting_probe_parallax_is_valid(lighting_probe_parallax_handle_t handle);
int lighting_probe_parallax_get_info(lighting_probe_parallax_handle_t handle, lighting_probe_parallax_info_t* out_info);
void lighting_probe_parallax_mark_dirty(lighting_probe_parallax_handle_t handle);
int lighting_probe_parallax_process_pending(void);

/* Statistics */
uint32_t lighting_probe_parallax_get_count(void);
size_t lighting_probe_parallax_get_memory_usage(void);
void lighting_probe_parallax_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_PROBE_PARALLAX_H */

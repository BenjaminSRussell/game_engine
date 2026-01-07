/*
 * probe_octahedron.h
 * Octahedral probe encoding
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_PROBE_OCTAHEDRON_H
#define LUMEN_PROBE_OCTAHEDRON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_probe_octahedron_handle {
    uint32_t id;
} lumen_probe_octahedron_handle_t;

typedef struct lumen_probe_octahedron_desc {
    uint32_t flags;
    void* user_data;
} lumen_probe_octahedron_desc_t;

typedef struct lumen_probe_octahedron_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_probe_octahedron_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_probe_octahedron_init(void);
void lumen_probe_octahedron_shutdown(void);

/* Lifecycle */
int lumen_probe_octahedron_create(lumen_probe_octahedron_handle_t* out_handle, const lumen_probe_octahedron_desc_t* desc);
void lumen_probe_octahedron_destroy(lumen_probe_octahedron_handle_t handle);

/* Operations */
int lumen_probe_octahedron_update(lumen_probe_octahedron_handle_t handle, const void* data, size_t size);
bool lumen_probe_octahedron_is_valid(lumen_probe_octahedron_handle_t handle);
int lumen_probe_octahedron_get_info(lumen_probe_octahedron_handle_t handle, lumen_probe_octahedron_info_t* out_info);
void lumen_probe_octahedron_mark_dirty(lumen_probe_octahedron_handle_t handle);
int lumen_probe_octahedron_process_pending(void);

/* Statistics */
uint32_t lumen_probe_octahedron_get_count(void);
size_t lumen_probe_octahedron_get_memory_usage(void);
void lumen_probe_octahedron_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_PROBE_OCTAHEDRON_H */

/*
 * cascade_stabilization.h
 * Temporal stabilization
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_CASCADE_STABILIZATION_H
#define LIGHTING_CASCADE_STABILIZATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_cascade_stabilization_handle {
    uint32_t id;
} lighting_cascade_stabilization_handle_t;

typedef struct lighting_cascade_stabilization_desc {
    uint32_t flags;
    void* user_data;
} lighting_cascade_stabilization_desc_t;

typedef struct lighting_cascade_stabilization_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_cascade_stabilization_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_cascade_stabilization_init(void);
void lighting_cascade_stabilization_shutdown(void);

/* Lifecycle */
int lighting_cascade_stabilization_create(lighting_cascade_stabilization_handle_t* out_handle, const lighting_cascade_stabilization_desc_t* desc);
void lighting_cascade_stabilization_destroy(lighting_cascade_stabilization_handle_t handle);

/* Operations */
int lighting_cascade_stabilization_update(lighting_cascade_stabilization_handle_t handle, const void* data, size_t size);
bool lighting_cascade_stabilization_is_valid(lighting_cascade_stabilization_handle_t handle);
int lighting_cascade_stabilization_get_info(lighting_cascade_stabilization_handle_t handle, lighting_cascade_stabilization_info_t* out_info);
void lighting_cascade_stabilization_mark_dirty(lighting_cascade_stabilization_handle_t handle);
int lighting_cascade_stabilization_process_pending(void);

/* Statistics */
uint32_t lighting_cascade_stabilization_get_count(void);
size_t lighting_cascade_stabilization_get_memory_usage(void);
void lighting_cascade_stabilization_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_CASCADE_STABILIZATION_H */

/*
 * triangle_cull.h
 * Triangle-level culling
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_TRIANGLE_CULL_H
#define NANITE_TRIANGLE_CULL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_triangle_cull_handle {
    uint32_t id;
} nanite_triangle_cull_handle_t;

typedef struct nanite_triangle_cull_desc {
    uint32_t flags;
    void* user_data;
} nanite_triangle_cull_desc_t;

typedef struct nanite_triangle_cull_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_triangle_cull_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_triangle_cull_init(void);
void nanite_triangle_cull_shutdown(void);

/* Lifecycle */
int nanite_triangle_cull_create(nanite_triangle_cull_handle_t* out_handle, const nanite_triangle_cull_desc_t* desc);
void nanite_triangle_cull_destroy(nanite_triangle_cull_handle_t handle);

/* Operations */
int nanite_triangle_cull_update(nanite_triangle_cull_handle_t handle, const void* data, size_t size);
bool nanite_triangle_cull_is_valid(nanite_triangle_cull_handle_t handle);
int nanite_triangle_cull_get_info(nanite_triangle_cull_handle_t handle, nanite_triangle_cull_info_t* out_info);
void nanite_triangle_cull_mark_dirty(nanite_triangle_cull_handle_t handle);
int nanite_triangle_cull_process_pending(void);

/* Statistics */
uint32_t nanite_triangle_cull_get_count(void);
size_t nanite_triangle_cull_get_memory_usage(void);
void nanite_triangle_cull_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_TRIANGLE_CULL_H */

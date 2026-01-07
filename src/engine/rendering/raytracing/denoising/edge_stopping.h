/*
 * edge_stopping.h
 * Edge-aware filtering
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_EDGE_STOPPING_H
#define RAYTRACING_EDGE_STOPPING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_edge_stopping_handle {
    uint32_t id;
} raytracing_edge_stopping_handle_t;

typedef struct raytracing_edge_stopping_desc {
    uint32_t flags;
    void* user_data;
} raytracing_edge_stopping_desc_t;

typedef struct raytracing_edge_stopping_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_edge_stopping_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_edge_stopping_init(void);
void raytracing_edge_stopping_shutdown(void);

/* Lifecycle */
int raytracing_edge_stopping_create(raytracing_edge_stopping_handle_t* out_handle, const raytracing_edge_stopping_desc_t* desc);
void raytracing_edge_stopping_destroy(raytracing_edge_stopping_handle_t handle);

/* Operations */
int raytracing_edge_stopping_update(raytracing_edge_stopping_handle_t handle, const void* data, size_t size);
bool raytracing_edge_stopping_is_valid(raytracing_edge_stopping_handle_t handle);
int raytracing_edge_stopping_get_info(raytracing_edge_stopping_handle_t handle, raytracing_edge_stopping_info_t* out_info);
void raytracing_edge_stopping_mark_dirty(raytracing_edge_stopping_handle_t handle);
int raytracing_edge_stopping_process_pending(void);

/* Statistics */
uint32_t raytracing_edge_stopping_get_count(void);
size_t raytracing_edge_stopping_get_memory_usage(void);
void raytracing_edge_stopping_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_EDGE_STOPPING_H */

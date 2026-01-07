/*
 * tlas_builder.h
 * Top-level AS construction
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_TLAS_BUILDER_H
#define RAYTRACING_TLAS_BUILDER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_tlas_builder_handle {
    uint32_t id;
} raytracing_tlas_builder_handle_t;

typedef struct raytracing_tlas_builder_desc {
    uint32_t flags;
    void* user_data;
} raytracing_tlas_builder_desc_t;

typedef struct raytracing_tlas_builder_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_tlas_builder_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_tlas_builder_init(void);
void raytracing_tlas_builder_shutdown(void);

/* Lifecycle */
int raytracing_tlas_builder_create(raytracing_tlas_builder_handle_t* out_handle, const raytracing_tlas_builder_desc_t* desc);
void raytracing_tlas_builder_destroy(raytracing_tlas_builder_handle_t handle);

/* Operations */
int raytracing_tlas_builder_update(raytracing_tlas_builder_handle_t handle, const void* data, size_t size);
bool raytracing_tlas_builder_is_valid(raytracing_tlas_builder_handle_t handle);
int raytracing_tlas_builder_get_info(raytracing_tlas_builder_handle_t handle, raytracing_tlas_builder_info_t* out_info);
void raytracing_tlas_builder_mark_dirty(raytracing_tlas_builder_handle_t handle);
int raytracing_tlas_builder_process_pending(void);

/* Statistics */
uint32_t raytracing_tlas_builder_get_count(void);
size_t raytracing_tlas_builder_get_memory_usage(void);
void raytracing_tlas_builder_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_TLAS_BUILDER_H */

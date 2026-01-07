/*
 * path_integrator.h
 * Path tracing integrator
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_PATH_INTEGRATOR_H
#define RAYTRACING_PATH_INTEGRATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_path_integrator_handle {
    uint32_t id;
} raytracing_path_integrator_handle_t;

typedef struct raytracing_path_integrator_desc {
    uint32_t flags;
    void* user_data;
} raytracing_path_integrator_desc_t;

typedef struct raytracing_path_integrator_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_path_integrator_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_path_integrator_init(void);
void raytracing_path_integrator_shutdown(void);

/* Lifecycle */
int raytracing_path_integrator_create(raytracing_path_integrator_handle_t* out_handle, const raytracing_path_integrator_desc_t* desc);
void raytracing_path_integrator_destroy(raytracing_path_integrator_handle_t handle);

/* Operations */
int raytracing_path_integrator_update(raytracing_path_integrator_handle_t handle, const void* data, size_t size);
bool raytracing_path_integrator_is_valid(raytracing_path_integrator_handle_t handle);
int raytracing_path_integrator_get_info(raytracing_path_integrator_handle_t handle, raytracing_path_integrator_info_t* out_info);
void raytracing_path_integrator_mark_dirty(raytracing_path_integrator_handle_t handle);
int raytracing_path_integrator_process_pending(void);

/* Statistics */
uint32_t raytracing_path_integrator_get_count(void);
size_t raytracing_path_integrator_get_memory_usage(void);
void raytracing_path_integrator_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_PATH_INTEGRATOR_H */

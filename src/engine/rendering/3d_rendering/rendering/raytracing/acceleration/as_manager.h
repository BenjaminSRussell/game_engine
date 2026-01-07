/*
 * as_manager.h
 * AS lifetime management
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_AS_MANAGER_H
#define RAYTRACING_AS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_as_manager_handle {
    uint32_t id;
} raytracing_as_manager_handle_t;

typedef struct raytracing_as_manager_desc {
    uint32_t flags;
    void* user_data;
} raytracing_as_manager_desc_t;

typedef struct raytracing_as_manager_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_as_manager_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_as_manager_init(void);
void raytracing_as_manager_shutdown(void);

/* Lifecycle */
int raytracing_as_manager_create(raytracing_as_manager_handle_t* out_handle, const raytracing_as_manager_desc_t* desc);
void raytracing_as_manager_destroy(raytracing_as_manager_handle_t handle);

/* Operations */
int raytracing_as_manager_update(raytracing_as_manager_handle_t handle, const void* data, size_t size);
bool raytracing_as_manager_is_valid(raytracing_as_manager_handle_t handle);
int raytracing_as_manager_get_info(raytracing_as_manager_handle_t handle, raytracing_as_manager_info_t* out_info);
void raytracing_as_manager_mark_dirty(raytracing_as_manager_handle_t handle);
int raytracing_as_manager_process_pending(void);

/* Statistics */
uint32_t raytracing_as_manager_get_count(void);
size_t raytracing_as_manager_get_memory_usage(void);
void raytracing_as_manager_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_AS_MANAGER_H */

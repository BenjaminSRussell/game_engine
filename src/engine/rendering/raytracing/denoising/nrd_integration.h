/*
 * nrd_integration.h
 * NRD library integration
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_NRD_INTEGRATION_H
#define RAYTRACING_NRD_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_nrd_integration_handle {
    uint32_t id;
} raytracing_nrd_integration_handle_t;

typedef struct raytracing_nrd_integration_desc {
    uint32_t flags;
    void* user_data;
} raytracing_nrd_integration_desc_t;

typedef struct raytracing_nrd_integration_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_nrd_integration_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_nrd_integration_init(void);
void raytracing_nrd_integration_shutdown(void);

/* Lifecycle */
int raytracing_nrd_integration_create(raytracing_nrd_integration_handle_t* out_handle, const raytracing_nrd_integration_desc_t* desc);
void raytracing_nrd_integration_destroy(raytracing_nrd_integration_handle_t handle);

/* Operations */
int raytracing_nrd_integration_update(raytracing_nrd_integration_handle_t handle, const void* data, size_t size);
bool raytracing_nrd_integration_is_valid(raytracing_nrd_integration_handle_t handle);
int raytracing_nrd_integration_get_info(raytracing_nrd_integration_handle_t handle, raytracing_nrd_integration_info_t* out_info);
void raytracing_nrd_integration_mark_dirty(raytracing_nrd_integration_handle_t handle);
int raytracing_nrd_integration_process_pending(void);

/* Statistics */
uint32_t raytracing_nrd_integration_get_count(void);
size_t raytracing_nrd_integration_get_memory_usage(void);
void raytracing_nrd_integration_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_NRD_INTEGRATION_H */

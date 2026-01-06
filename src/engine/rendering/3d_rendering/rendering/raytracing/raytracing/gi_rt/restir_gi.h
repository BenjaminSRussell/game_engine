/*
 * restir_gi.h
 * ReSTIR GI
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_RESTIR_GI_H
#define RAYTRACING_RESTIR_GI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_restir_gi_handle {
    uint32_t id;
} raytracing_restir_gi_handle_t;

typedef struct raytracing_restir_gi_desc {
    uint32_t flags;
    void* user_data;
} raytracing_restir_gi_desc_t;

typedef struct raytracing_restir_gi_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_restir_gi_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_restir_gi_init(void);
void raytracing_restir_gi_shutdown(void);

/* Lifecycle */
int raytracing_restir_gi_create(raytracing_restir_gi_handle_t* out_handle, const raytracing_restir_gi_desc_t* desc);
void raytracing_restir_gi_destroy(raytracing_restir_gi_handle_t handle);

/* Operations */
int raytracing_restir_gi_update(raytracing_restir_gi_handle_t handle, const void* data, size_t size);
bool raytracing_restir_gi_is_valid(raytracing_restir_gi_handle_t handle);
int raytracing_restir_gi_get_info(raytracing_restir_gi_handle_t handle, raytracing_restir_gi_info_t* out_info);
void raytracing_restir_gi_mark_dirty(raytracing_restir_gi_handle_t handle);
int raytracing_restir_gi_process_pending(void);

/* Statistics */
uint32_t raytracing_restir_gi_get_count(void);
size_t raytracing_restir_gi_get_memory_usage(void);
void raytracing_restir_gi_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_RESTIR_GI_H */

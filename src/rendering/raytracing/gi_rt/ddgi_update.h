/*
 * ddgi_update.h
 * DDGI probe update
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_DDGI_UPDATE_H
#define RAYTRACING_DDGI_UPDATE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_ddgi_update_handle {
    uint32_t id;
} raytracing_ddgi_update_handle_t;

typedef struct raytracing_ddgi_update_desc {
    uint32_t flags;
    void* user_data;
} raytracing_ddgi_update_desc_t;

typedef struct raytracing_ddgi_update_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_ddgi_update_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_ddgi_update_init(void);
void raytracing_ddgi_update_shutdown(void);

/* Lifecycle */
int raytracing_ddgi_update_create(raytracing_ddgi_update_handle_t* out_handle, const raytracing_ddgi_update_desc_t* desc);
void raytracing_ddgi_update_destroy(raytracing_ddgi_update_handle_t handle);

/* Operations */
int raytracing_ddgi_update_update(raytracing_ddgi_update_handle_t handle, const void* data, size_t size);
bool raytracing_ddgi_update_is_valid(raytracing_ddgi_update_handle_t handle);
int raytracing_ddgi_update_get_info(raytracing_ddgi_update_handle_t handle, raytracing_ddgi_update_info_t* out_info);
void raytracing_ddgi_update_mark_dirty(raytracing_ddgi_update_handle_t handle);
int raytracing_ddgi_update_process_pending(void);

/* Statistics */
uint32_t raytracing_ddgi_update_get_count(void);
size_t raytracing_ddgi_update_get_memory_usage(void);
void raytracing_ddgi_update_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_DDGI_UPDATE_H */

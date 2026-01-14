/*
 * lighting_gradient.h
 * Time-of-day lighting
 *
 * Part of the Atmosphere subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ATMOSPHERE_LIGHTING_GRADIENT_H
#define ATMOSPHERE_LIGHTING_GRADIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct atmosphere_lighting_gradient_handle {
    uint32_t id;
} atmosphere_lighting_gradient_handle_t;

typedef struct atmosphere_lighting_gradient_desc {
    uint32_t flags;
    void* user_data;
} atmosphere_lighting_gradient_desc_t;

typedef struct atmosphere_lighting_gradient_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} atmosphere_lighting_gradient_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int atmosphere_lighting_gradient_init(void);
void atmosphere_lighting_gradient_shutdown(void);

/* Lifecycle */
int atmosphere_lighting_gradient_create(atmosphere_lighting_gradient_handle_t* out_handle, const atmosphere_lighting_gradient_desc_t* desc);
void atmosphere_lighting_gradient_destroy(atmosphere_lighting_gradient_handle_t handle);

/* Operations */
int atmosphere_lighting_gradient_update(atmosphere_lighting_gradient_handle_t handle, const void* data, size_t size);
bool atmosphere_lighting_gradient_is_valid(atmosphere_lighting_gradient_handle_t handle);
int atmosphere_lighting_gradient_get_info(atmosphere_lighting_gradient_handle_t handle, atmosphere_lighting_gradient_info_t* out_info);
void atmosphere_lighting_gradient_mark_dirty(atmosphere_lighting_gradient_handle_t handle);
int atmosphere_lighting_gradient_process_pending(void);

/* Statistics */
uint32_t atmosphere_lighting_gradient_get_count(void);
size_t atmosphere_lighting_gradient_get_memory_usage(void);
void atmosphere_lighting_gradient_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ATMOSPHERE_LIGHTING_GRADIENT_H */

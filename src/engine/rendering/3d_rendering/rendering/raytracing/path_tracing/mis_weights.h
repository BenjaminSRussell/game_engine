/*
 * mis_weights.h
 * Multiple importance sampling
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_MIS_WEIGHTS_H
#define RAYTRACING_MIS_WEIGHTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_mis_weights_handle {
    uint32_t id;
} raytracing_mis_weights_handle_t;

typedef struct raytracing_mis_weights_desc {
    uint32_t flags;
    void* user_data;
} raytracing_mis_weights_desc_t;

typedef struct raytracing_mis_weights_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_mis_weights_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_mis_weights_init(void);
void raytracing_mis_weights_shutdown(void);

/* Lifecycle */
int raytracing_mis_weights_create(raytracing_mis_weights_handle_t* out_handle, const raytracing_mis_weights_desc_t* desc);
void raytracing_mis_weights_destroy(raytracing_mis_weights_handle_t handle);

/* Operations */
int raytracing_mis_weights_update(raytracing_mis_weights_handle_t handle, const void* data, size_t size);
bool raytracing_mis_weights_is_valid(raytracing_mis_weights_handle_t handle);
int raytracing_mis_weights_get_info(raytracing_mis_weights_handle_t handle, raytracing_mis_weights_info_t* out_info);
void raytracing_mis_weights_mark_dirty(raytracing_mis_weights_handle_t handle);
int raytracing_mis_weights_process_pending(void);

/* Statistics */
uint32_t raytracing_mis_weights_get_count(void);
size_t raytracing_mis_weights_get_memory_usage(void);
void raytracing_mis_weights_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_MIS_WEIGHTS_H */

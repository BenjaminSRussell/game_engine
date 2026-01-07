/*
 * temporal_accumulation.h
 * Temporal sample accumulation
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_TEMPORAL_ACCUMULATION_H
#define RAYTRACING_TEMPORAL_ACCUMULATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_temporal_accumulation_handle {
    uint32_t id;
} raytracing_temporal_accumulation_handle_t;

typedef struct raytracing_temporal_accumulation_desc {
    uint32_t flags;
    void* user_data;
} raytracing_temporal_accumulation_desc_t;

typedef struct raytracing_temporal_accumulation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_temporal_accumulation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_temporal_accumulation_init(void);
void raytracing_temporal_accumulation_shutdown(void);

/* Lifecycle */
int raytracing_temporal_accumulation_create(raytracing_temporal_accumulation_handle_t* out_handle, const raytracing_temporal_accumulation_desc_t* desc);
void raytracing_temporal_accumulation_destroy(raytracing_temporal_accumulation_handle_t handle);

/* Operations */
int raytracing_temporal_accumulation_update(raytracing_temporal_accumulation_handle_t handle, const void* data, size_t size);
bool raytracing_temporal_accumulation_is_valid(raytracing_temporal_accumulation_handle_t handle);
int raytracing_temporal_accumulation_get_info(raytracing_temporal_accumulation_handle_t handle, raytracing_temporal_accumulation_info_t* out_info);
void raytracing_temporal_accumulation_mark_dirty(raytracing_temporal_accumulation_handle_t handle);
int raytracing_temporal_accumulation_process_pending(void);

/* Statistics */
uint32_t raytracing_temporal_accumulation_get_count(void);
size_t raytracing_temporal_accumulation_get_memory_usage(void);
void raytracing_temporal_accumulation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_TEMPORAL_ACCUMULATION_H */

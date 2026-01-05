/*
 * russian_roulette.h
 * Russian roulette termination
 *
 * Part of the Raytracing subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RAYTRACING_RUSSIAN_ROULETTE_H
#define RAYTRACING_RUSSIAN_ROULETTE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct raytracing_russian_roulette_handle {
    uint32_t id;
} raytracing_russian_roulette_handle_t;

typedef struct raytracing_russian_roulette_desc {
    uint32_t flags;
    void* user_data;
} raytracing_russian_roulette_desc_t;

typedef struct raytracing_russian_roulette_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} raytracing_russian_roulette_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int raytracing_russian_roulette_init(void);
void raytracing_russian_roulette_shutdown(void);

/* Lifecycle */
int raytracing_russian_roulette_create(raytracing_russian_roulette_handle_t* out_handle, const raytracing_russian_roulette_desc_t* desc);
void raytracing_russian_roulette_destroy(raytracing_russian_roulette_handle_t handle);

/* Operations */
int raytracing_russian_roulette_update(raytracing_russian_roulette_handle_t handle, const void* data, size_t size);
bool raytracing_russian_roulette_is_valid(raytracing_russian_roulette_handle_t handle);
int raytracing_russian_roulette_get_info(raytracing_russian_roulette_handle_t handle, raytracing_russian_roulette_info_t* out_info);
void raytracing_russian_roulette_mark_dirty(raytracing_russian_roulette_handle_t handle);
int raytracing_russian_roulette_process_pending(void);

/* Statistics */
uint32_t raytracing_russian_roulette_get_count(void);
size_t raytracing_russian_roulette_get_memory_usage(void);
void raytracing_russian_roulette_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RAYTRACING_RUSSIAN_ROULETTE_H */

/*
 * gerstner_waves.h
 * Gerstner wave superposition
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_GERSTNER_WAVES_H
#define WATER_GERSTNER_WAVES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <math/vec2.h>
#include <math/vec3.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_gerstner_waves_handle {
    uint32_t id;
} water_gerstner_waves_handle_t;

typedef struct water_gerstner_waves_desc {
    uint32_t flags;
    void* user_data;
} water_gerstner_waves_desc_t;

typedef struct water_gerstner_waves_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_gerstner_waves_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_gerstner_waves_init(void);
void water_gerstner_waves_shutdown(void);

/* Lifecycle */
int water_gerstner_waves_create(water_gerstner_waves_handle_t* out_handle, const water_gerstner_waves_desc_t* desc);
void water_gerstner_waves_destroy(water_gerstner_waves_handle_t handle);

/* Operations */
int water_gerstner_waves_update(water_gerstner_waves_handle_t handle, const void* data, size_t size);
Vec3 water_gerstner_waves_get_displacement(water_gerstner_waves_handle_t handle, Vec2 pos, float time);
Vec3 water_gerstner_waves_get_normal(water_gerstner_waves_handle_t handle, Vec2 pos, float time);
bool water_gerstner_waves_is_valid(water_gerstner_waves_handle_t handle);
int water_gerstner_waves_get_info(water_gerstner_waves_handle_t handle, water_gerstner_waves_info_t* out_info);
void water_gerstner_waves_mark_dirty(water_gerstner_waves_handle_t handle);
int water_gerstner_waves_process_pending(void);

/* Statistics */
uint32_t water_gerstner_waves_get_count(void);
size_t water_gerstner_waves_get_memory_usage(void);
void water_gerstner_waves_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_GERSTNER_WAVES_H */

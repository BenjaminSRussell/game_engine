/*
 * underwater_fog.h
 * Underwater fog/scattering
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_UNDERWATER_FOG_H
#define WATER_UNDERWATER_FOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "engine/include/math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_underwater_fog_handle {
    uint32_t id;
} water_underwater_fog_handle_t;

typedef struct water_underwater_fog_desc {
    uint32_t flags;
    void* user_data;
} water_underwater_fog_desc_t;

typedef struct water_underwater_fog_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_underwater_fog_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_underwater_fog_init(void);
void water_underwater_fog_shutdown(void);

/* Lifecycle */
int water_underwater_fog_create(water_underwater_fog_handle_t* out_handle, const water_underwater_fog_desc_t* desc);
void water_underwater_fog_destroy(water_underwater_fog_handle_t handle);

/* Operations */
int water_underwater_fog_update(water_underwater_fog_handle_t handle, float time);
bool water_underwater_fog_is_valid(water_underwater_fog_handle_t handle);
int water_underwater_fog_get_info(water_underwater_fog_handle_t handle, water_underwater_fog_info_t* out_info);
void water_underwater_fog_mark_dirty(water_underwater_fog_handle_t handle);
int water_underwater_fog_process_pending(void);

/* Statistics */
uint32_t water_underwater_fog_get_count(void);
size_t water_underwater_fog_get_memory_usage(void);
void water_underwater_fog_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_UNDERWATER_FOG_H */

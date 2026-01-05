/*
 * buoyancy.h
 * Object buoyancy
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_BUOYANCY_H
#define WATER_BUOYANCY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_buoyancy_handle {
    uint32_t id;
} water_buoyancy_handle_t;

typedef struct water_buoyancy_desc {
    uint32_t flags;
    void* user_data;
} water_buoyancy_desc_t;

typedef struct water_buoyancy_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_buoyancy_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_buoyancy_init(void);
void water_buoyancy_shutdown(void);

/* Lifecycle */
int water_buoyancy_create(water_buoyancy_handle_t* out_handle, const water_buoyancy_desc_t* desc);
void water_buoyancy_destroy(water_buoyancy_handle_t handle);

/* Operations */
int water_buoyancy_update(water_buoyancy_handle_t handle, const void* data, size_t size);
bool water_buoyancy_is_valid(water_buoyancy_handle_t handle);
int water_buoyancy_get_info(water_buoyancy_handle_t handle, water_buoyancy_info_t* out_info);
void water_buoyancy_mark_dirty(water_buoyancy_handle_t handle);
int water_buoyancy_process_pending(void);

/* Statistics */
uint32_t water_buoyancy_get_count(void);
size_t water_buoyancy_get_memory_usage(void);
void water_buoyancy_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_BUOYANCY_H */

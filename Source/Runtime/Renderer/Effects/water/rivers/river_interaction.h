/*
 * river_interaction.h
 * River interaction
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_RIVER_INTERACTION_H
#define WATER_RIVER_INTERACTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_river_interaction_handle {
    uint32_t id;
} water_river_interaction_handle_t;

typedef struct water_river_interaction_desc {
    uint32_t flags;
    void* user_data;
} water_river_interaction_desc_t;

typedef struct water_river_interaction_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_river_interaction_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_river_interaction_init(void);
void water_river_interaction_shutdown(void);

/* Lifecycle */
int water_river_interaction_create(water_river_interaction_handle_t* out_handle, const water_river_interaction_desc_t* desc);
void water_river_interaction_destroy(water_river_interaction_handle_t handle);

/* Operations */
int water_river_interaction_update(water_river_interaction_handle_t handle, const void* data, size_t size);
bool water_river_interaction_is_valid(water_river_interaction_handle_t handle);
int water_river_interaction_get_info(water_river_interaction_handle_t handle, water_river_interaction_info_t* out_info);
void water_river_interaction_mark_dirty(water_river_interaction_handle_t handle);
int water_river_interaction_process_pending(void);

/* Statistics */
uint32_t water_river_interaction_get_count(void);
size_t water_river_interaction_get_memory_usage(void);
void water_river_interaction_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_RIVER_INTERACTION_H */

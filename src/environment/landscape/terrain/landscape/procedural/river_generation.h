/*
 * river_generation.h
 * Procedural rivers
 *
 * Part of the Landscape subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LANDSCAPE_RIVER_GENERATION_H
#define LANDSCAPE_RIVER_GENERATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct landscape_river_generation_handle {
    uint32_t id;
} landscape_river_generation_handle_t;

typedef struct landscape_river_generation_desc {
    uint32_t flags;
    void* user_data;
} landscape_river_generation_desc_t;

typedef struct landscape_river_generation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} landscape_river_generation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int landscape_river_generation_init(void);
void landscape_river_generation_shutdown(void);

/* Lifecycle */
int landscape_river_generation_create(landscape_river_generation_handle_t* out_handle, const landscape_river_generation_desc_t* desc);
void landscape_river_generation_destroy(landscape_river_generation_handle_t handle);

/* Operations */
int landscape_river_generation_update(landscape_river_generation_handle_t handle, const void* data, size_t size);
bool landscape_river_generation_is_valid(landscape_river_generation_handle_t handle);
int landscape_river_generation_get_info(landscape_river_generation_handle_t handle, landscape_river_generation_info_t* out_info);
void landscape_river_generation_mark_dirty(landscape_river_generation_handle_t handle);
int landscape_river_generation_process_pending(void);

/* Statistics */
uint32_t landscape_river_generation_get_count(void);
size_t landscape_river_generation_get_memory_usage(void);
void landscape_river_generation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LANDSCAPE_RIVER_GENERATION_H */

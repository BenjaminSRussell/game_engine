/*
 * waterfall.h
 * Waterfall effects
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_WATERFALL_H
#define WATER_WATERFALL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_waterfall_handle {
    uint32_t id;
} water_waterfall_handle_t;

typedef struct water_waterfall_desc {
    uint32_t flags;
    void* user_data;
} water_waterfall_desc_t;

typedef struct water_waterfall_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_waterfall_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_waterfall_init(void);
void water_waterfall_shutdown(void);

/* Lifecycle */
int water_waterfall_create(water_waterfall_handle_t* out_handle, const water_waterfall_desc_t* desc);
void water_waterfall_destroy(water_waterfall_handle_t handle);

/* Operations */
int water_waterfall_update(water_waterfall_handle_t handle, const void* data, size_t size);
bool water_waterfall_is_valid(water_waterfall_handle_t handle);
int water_waterfall_get_info(water_waterfall_handle_t handle, water_waterfall_info_t* out_info);
void water_waterfall_mark_dirty(water_waterfall_handle_t handle);
int water_waterfall_process_pending(void);

/* Statistics */
uint32_t water_waterfall_get_count(void);
size_t water_waterfall_get_memory_usage(void);
void water_waterfall_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_WATERFALL_H */

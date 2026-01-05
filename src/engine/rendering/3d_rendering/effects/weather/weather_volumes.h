/*
 * weather_volumes.h
 * Weather zone volumes
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_WEATHER_VOLUMES_H
#define EFFECTS_WEATHER_VOLUMES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_weather_volumes_handle {
    uint32_t id;
} effects_weather_volumes_handle_t;

typedef struct effects_weather_volumes_desc {
    uint32_t flags;
    void* user_data;
} effects_weather_volumes_desc_t;

typedef struct effects_weather_volumes_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_weather_volumes_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_weather_volumes_init(void);
void effects_weather_volumes_shutdown(void);

/* Lifecycle */
int effects_weather_volumes_create(effects_weather_volumes_handle_t* out_handle, const effects_weather_volumes_desc_t* desc);
void effects_weather_volumes_destroy(effects_weather_volumes_handle_t handle);

/* Operations */
int effects_weather_volumes_update(effects_weather_volumes_handle_t handle, const void* data, size_t size);
bool effects_weather_volumes_is_valid(effects_weather_volumes_handle_t handle);
int effects_weather_volumes_get_info(effects_weather_volumes_handle_t handle, effects_weather_volumes_info_t* out_info);
void effects_weather_volumes_mark_dirty(effects_weather_volumes_handle_t handle);
int effects_weather_volumes_process_pending(void);

/* Statistics */
uint32_t effects_weather_volumes_get_count(void);
size_t effects_weather_volumes_get_memory_usage(void);
void effects_weather_volumes_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_WEATHER_VOLUMES_H */

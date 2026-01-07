/*
 * snow_system.h
 * Snow particle system
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_SNOW_SYSTEM_H
#define EFFECTS_SNOW_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_snow_system_handle {
    uint32_t id;
} effects_snow_system_handle_t;

typedef struct effects_snow_system_desc {
    uint32_t flags;
    void* user_data;
} effects_snow_system_desc_t;

typedef struct effects_snow_system_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_snow_system_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_snow_system_init(void);
void effects_snow_system_shutdown(void);

/* Lifecycle */
int effects_snow_system_create(effects_snow_system_handle_t* out_handle, const effects_snow_system_desc_t* desc);
void effects_snow_system_destroy(effects_snow_system_handle_t handle);

/* Operations */
int effects_snow_system_update(effects_snow_system_handle_t handle, const void* data, size_t size);
bool effects_snow_system_is_valid(effects_snow_system_handle_t handle);
int effects_snow_system_get_info(effects_snow_system_handle_t handle, effects_snow_system_info_t* out_info);
void effects_snow_system_mark_dirty(effects_snow_system_handle_t handle);
int effects_snow_system_process_pending(void);

/* Statistics */
uint32_t effects_snow_system_get_count(void);
size_t effects_snow_system_get_memory_usage(void);
void effects_snow_system_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_SNOW_SYSTEM_H */

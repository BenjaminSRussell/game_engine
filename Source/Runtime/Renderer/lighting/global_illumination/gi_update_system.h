/*
 * gi_update_system.h
 * GI update scheduling
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_GI_UPDATE_SYSTEM_H
#define LIGHTING_GI_UPDATE_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_gi_update_system_handle {
    uint32_t id;
} lighting_gi_update_system_handle_t;

typedef struct lighting_gi_update_system_desc {
    uint32_t flags;
    void* user_data;
} lighting_gi_update_system_desc_t;

typedef struct lighting_gi_update_system_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_gi_update_system_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_gi_update_system_init(void);
void lighting_gi_update_system_shutdown(void);

/* Lifecycle */
int lighting_gi_update_system_create(lighting_gi_update_system_handle_t* out_handle, const lighting_gi_update_system_desc_t* desc);
void lighting_gi_update_system_destroy(lighting_gi_update_system_handle_t handle);

/* Operations */
int lighting_gi_update_system_update(lighting_gi_update_system_handle_t handle, const void* data, size_t size);
bool lighting_gi_update_system_is_valid(lighting_gi_update_system_handle_t handle);
int lighting_gi_update_system_get_info(lighting_gi_update_system_handle_t handle, lighting_gi_update_system_info_t* out_info);
void lighting_gi_update_system_mark_dirty(lighting_gi_update_system_handle_t handle);
int lighting_gi_update_system_process_pending(void);

/* Statistics */
uint32_t lighting_gi_update_system_get_count(void);
size_t lighting_gi_update_system_get_memory_usage(void);
void lighting_gi_update_system_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_GI_UPDATE_SYSTEM_H */

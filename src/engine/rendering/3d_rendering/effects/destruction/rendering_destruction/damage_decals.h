/*
 * damage_decals.h
 * Damage decals
 *
 * Part of the Destruction subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DESTRUCTION_DAMAGE_DECALS_H
#define DESTRUCTION_DAMAGE_DECALS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct destruction_damage_decals_handle {
    uint32_t id;
} destruction_damage_decals_handle_t;

typedef struct destruction_damage_decals_desc {
    uint32_t flags;
    void* user_data;
} destruction_damage_decals_desc_t;

typedef struct destruction_damage_decals_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} destruction_damage_decals_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int destruction_damage_decals_init(void);
void destruction_damage_decals_shutdown(void);

/* Lifecycle */
int destruction_damage_decals_create(destruction_damage_decals_handle_t* out_handle, const destruction_damage_decals_desc_t* desc);
void destruction_damage_decals_destroy(destruction_damage_decals_handle_t handle);

/* Operations */
int destruction_damage_decals_update(destruction_damage_decals_handle_t handle, const void* data, size_t size);
bool destruction_damage_decals_is_valid(destruction_damage_decals_handle_t handle);
int destruction_damage_decals_get_info(destruction_damage_decals_handle_t handle, destruction_damage_decals_info_t* out_info);
void destruction_damage_decals_mark_dirty(destruction_damage_decals_handle_t handle);
int destruction_damage_decals_process_pending(void);

/* Statistics */
uint32_t destruction_damage_decals_get_count(void);
size_t destruction_damage_decals_get_memory_usage(void);
void destruction_damage_decals_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* DESTRUCTION_DAMAGE_DECALS_H */

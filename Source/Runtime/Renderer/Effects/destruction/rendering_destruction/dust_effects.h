/*
 * dust_effects.h
 * Dust/smoke effects
 *
 * Part of the Destruction subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DESTRUCTION_DUST_EFFECTS_H
#define DESTRUCTION_DUST_EFFECTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct destruction_dust_effects_handle {
    uint32_t id;
} destruction_dust_effects_handle_t;

typedef struct destruction_dust_effects_desc {
    uint32_t flags;
    void* user_data;
} destruction_dust_effects_desc_t;

typedef struct destruction_dust_effects_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} destruction_dust_effects_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int destruction_dust_effects_init(void);
void destruction_dust_effects_shutdown(void);

/* Lifecycle */
int destruction_dust_effects_create(destruction_dust_effects_handle_t* out_handle, const destruction_dust_effects_desc_t* desc);
void destruction_dust_effects_destroy(destruction_dust_effects_handle_t handle);

/* Operations */
int destruction_dust_effects_update(destruction_dust_effects_handle_t handle, const void* data, size_t size);
bool destruction_dust_effects_is_valid(destruction_dust_effects_handle_t handle);
int destruction_dust_effects_get_info(destruction_dust_effects_handle_t handle, destruction_dust_effects_info_t* out_info);
void destruction_dust_effects_mark_dirty(destruction_dust_effects_handle_t handle);
int destruction_dust_effects_process_pending(void);

/* Statistics */
uint32_t destruction_dust_effects_get_count(void);
size_t destruction_dust_effects_get_memory_usage(void);
void destruction_dust_effects_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* DESTRUCTION_DUST_EFFECTS_H */

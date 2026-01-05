/*
 * splash_effects.h
 * Splash particle effects
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_SPLASH_EFFECTS_H
#define WATER_SPLASH_EFFECTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_splash_effects_handle {
    uint32_t id;
} water_splash_effects_handle_t;

typedef struct water_splash_effects_desc {
    uint32_t flags;
    void* user_data;
} water_splash_effects_desc_t;

typedef struct water_splash_effects_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_splash_effects_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_splash_effects_init(void);
void water_splash_effects_shutdown(void);

/* Lifecycle */
int water_splash_effects_create(water_splash_effects_handle_t* out_handle, const water_splash_effects_desc_t* desc);
void water_splash_effects_destroy(water_splash_effects_handle_t handle);

/* Operations */
int water_splash_effects_update(water_splash_effects_handle_t handle, const void* data, size_t size);
bool water_splash_effects_is_valid(water_splash_effects_handle_t handle);
int water_splash_effects_get_info(water_splash_effects_handle_t handle, water_splash_effects_info_t* out_info);
void water_splash_effects_mark_dirty(water_splash_effects_handle_t handle);
int water_splash_effects_process_pending(void);

/* Statistics */
uint32_t water_splash_effects_get_count(void);
size_t water_splash_effects_get_memory_usage(void);
void water_splash_effects_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_SPLASH_EFFECTS_H */

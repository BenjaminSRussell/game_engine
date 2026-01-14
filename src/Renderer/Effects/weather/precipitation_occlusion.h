/*
 * precipitation_occlusion.h
 * Indoor/outdoor detection
 *
 * Part of the Effects subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef EFFECTS_PRECIPITATION_OCCLUSION_H
#define EFFECTS_PRECIPITATION_OCCLUSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct effects_precipitation_occlusion_handle {
    uint32_t id;
} effects_precipitation_occlusion_handle_t;

typedef struct effects_precipitation_occlusion_desc {
    uint32_t flags;
    void* user_data;
} effects_precipitation_occlusion_desc_t;

typedef struct effects_precipitation_occlusion_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} effects_precipitation_occlusion_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int effects_precipitation_occlusion_init(void);
void effects_precipitation_occlusion_shutdown(void);

/* Lifecycle */
int effects_precipitation_occlusion_create(effects_precipitation_occlusion_handle_t* out_handle, const effects_precipitation_occlusion_desc_t* desc);
void effects_precipitation_occlusion_destroy(effects_precipitation_occlusion_handle_t handle);

/* Operations */
int effects_precipitation_occlusion_update(effects_precipitation_occlusion_handle_t handle, const void* data, size_t size);
bool effects_precipitation_occlusion_is_valid(effects_precipitation_occlusion_handle_t handle);
int effects_precipitation_occlusion_get_info(effects_precipitation_occlusion_handle_t handle, effects_precipitation_occlusion_info_t* out_info);
void effects_precipitation_occlusion_mark_dirty(effects_precipitation_occlusion_handle_t handle);
int effects_precipitation_occlusion_process_pending(void);

/* Statistics */
uint32_t effects_precipitation_occlusion_get_count(void);
size_t effects_precipitation_occlusion_get_memory_usage(void);
void effects_precipitation_occlusion_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* EFFECTS_PRECIPITATION_OCCLUSION_H */

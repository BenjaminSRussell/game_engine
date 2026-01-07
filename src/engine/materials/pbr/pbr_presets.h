/*
 * pbr_presets.h
 * Common PBR presets
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_PBR_PRESETS_H
#define MATERIALS_PBR_PRESETS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_pbr_presets_handle {
    uint32_t id;
} materials_pbr_presets_handle_t;

typedef struct materials_pbr_presets_desc {
    uint32_t flags;
    void* user_data;
} materials_pbr_presets_desc_t;

typedef struct materials_pbr_presets_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_pbr_presets_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_pbr_presets_init(void);
void materials_pbr_presets_shutdown(void);

/* Lifecycle */
int materials_pbr_presets_create(materials_pbr_presets_handle_t* out_handle, const materials_pbr_presets_desc_t* desc);
void materials_pbr_presets_destroy(materials_pbr_presets_handle_t handle);

/* Operations */
int materials_pbr_presets_update(materials_pbr_presets_handle_t handle, const void* data, size_t size);
bool materials_pbr_presets_is_valid(materials_pbr_presets_handle_t handle);
int materials_pbr_presets_get_info(materials_pbr_presets_handle_t handle, materials_pbr_presets_info_t* out_info);
void materials_pbr_presets_mark_dirty(materials_pbr_presets_handle_t handle);
int materials_pbr_presets_process_pending(void);

/* Statistics */
uint32_t materials_pbr_presets_get_count(void);
size_t materials_pbr_presets_get_memory_usage(void);
void materials_pbr_presets_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_PBR_PRESETS_H */

/*
 * pbr_validation.h
 * PBR parameter validation
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_PBR_VALIDATION_H
#define MATERIALS_PBR_VALIDATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_pbr_validation_handle {
    uint32_t id;
} materials_pbr_validation_handle_t;

typedef struct materials_pbr_validation_desc {
    uint32_t flags;
    void* user_data;
} materials_pbr_validation_desc_t;

typedef struct materials_pbr_validation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_pbr_validation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_pbr_validation_init(void);
void materials_pbr_validation_shutdown(void);

/* Lifecycle */
int materials_pbr_validation_create(materials_pbr_validation_handle_t* out_handle, const materials_pbr_validation_desc_t* desc);
void materials_pbr_validation_destroy(materials_pbr_validation_handle_t handle);

/* Operations */
int materials_pbr_validation_update(materials_pbr_validation_handle_t handle, const void* data, size_t size);
bool materials_pbr_validation_is_valid(materials_pbr_validation_handle_t handle);
int materials_pbr_validation_get_info(materials_pbr_validation_handle_t handle, materials_pbr_validation_info_t* out_info);
void materials_pbr_validation_mark_dirty(materials_pbr_validation_handle_t handle);
int materials_pbr_validation_process_pending(void);

/* Statistics */
uint32_t materials_pbr_validation_get_count(void);
size_t materials_pbr_validation_get_memory_usage(void);
void materials_pbr_validation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_PBR_VALIDATION_H */

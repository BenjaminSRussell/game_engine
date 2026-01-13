/*
 * pbr_parameters.h
 * PBR material parameters
 *
 * Part of the Materials subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MATERIALS_PBR_PARAMETERS_H
#define MATERIALS_PBR_PARAMETERS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct materials_pbr_parameters_handle {
    uint32_t id;
} materials_pbr_parameters_handle_t;

typedef struct materials_pbr_parameters_desc {
    uint32_t flags;
    void* user_data;
} materials_pbr_parameters_desc_t;

typedef struct materials_pbr_parameters_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} materials_pbr_parameters_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int materials_pbr_parameters_init(void);
void materials_pbr_parameters_shutdown(void);

/* Lifecycle */
int materials_pbr_parameters_create(materials_pbr_parameters_handle_t* out_handle, const materials_pbr_parameters_desc_t* desc);
void materials_pbr_parameters_destroy(materials_pbr_parameters_handle_t handle);

/* Operations */
int materials_pbr_parameters_update(materials_pbr_parameters_handle_t handle, const void* data, size_t size);
bool materials_pbr_parameters_is_valid(materials_pbr_parameters_handle_t handle);
int materials_pbr_parameters_get_info(materials_pbr_parameters_handle_t handle, materials_pbr_parameters_info_t* out_info);
void materials_pbr_parameters_mark_dirty(materials_pbr_parameters_handle_t handle);
int materials_pbr_parameters_process_pending(void);

/* Statistics */
uint32_t materials_pbr_parameters_get_count(void);
size_t materials_pbr_parameters_get_memory_usage(void);
void materials_pbr_parameters_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* MATERIALS_PBR_PARAMETERS_H */

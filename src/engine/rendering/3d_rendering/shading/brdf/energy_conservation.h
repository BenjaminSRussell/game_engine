/*
 * energy_conservation.h
 * Energy conservation
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_ENERGY_CONSERVATION_H
#define SHADING_ENERGY_CONSERVATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_energy_conservation_handle {
    uint32_t id;
} shading_energy_conservation_handle_t;

typedef struct shading_energy_conservation_desc {
    uint32_t flags;
    void* user_data;
} shading_energy_conservation_desc_t;

typedef struct shading_energy_conservation_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_energy_conservation_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_energy_conservation_init(void);
void shading_energy_conservation_shutdown(void);

/* Lifecycle */
int shading_energy_conservation_create(shading_energy_conservation_handle_t* out_handle, const shading_energy_conservation_desc_t* desc);
void shading_energy_conservation_destroy(shading_energy_conservation_handle_t handle);

/* Operations */
int shading_energy_conservation_update(shading_energy_conservation_handle_t handle, const void* data, size_t size);
bool shading_energy_conservation_is_valid(shading_energy_conservation_handle_t handle);
int shading_energy_conservation_get_info(shading_energy_conservation_handle_t handle, shading_energy_conservation_info_t* out_info);
void shading_energy_conservation_mark_dirty(shading_energy_conservation_handle_t handle);
int shading_energy_conservation_process_pending(void);

/* Statistics */
uint32_t shading_energy_conservation_get_count(void);
size_t shading_energy_conservation_get_memory_usage(void);
void shading_energy_conservation_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_ENERGY_CONSERVATION_H */

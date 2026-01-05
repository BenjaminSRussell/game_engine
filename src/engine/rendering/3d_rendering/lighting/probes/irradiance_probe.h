/*
 * irradiance_probe.h
 * Irradiance probe sampling
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_IRRADIANCE_PROBE_H
#define LIGHTING_IRRADIANCE_PROBE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lighting_irradiance_probe_handle {
    uint32_t id;
} lighting_irradiance_probe_handle_t;

typedef struct lighting_irradiance_probe_desc {
    uint32_t flags;
    void* user_data;
} lighting_irradiance_probe_desc_t;

typedef struct lighting_irradiance_probe_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lighting_irradiance_probe_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lighting_irradiance_probe_init(void);
void lighting_irradiance_probe_shutdown(void);

/* Lifecycle */
int lighting_irradiance_probe_create(lighting_irradiance_probe_handle_t* out_handle, const lighting_irradiance_probe_desc_t* desc);
void lighting_irradiance_probe_destroy(lighting_irradiance_probe_handle_t handle);

/* Operations */
int lighting_irradiance_probe_update(lighting_irradiance_probe_handle_t handle, const void* data, size_t size);
bool lighting_irradiance_probe_is_valid(lighting_irradiance_probe_handle_t handle);
int lighting_irradiance_probe_get_info(lighting_irradiance_probe_handle_t handle, lighting_irradiance_probe_info_t* out_info);
void lighting_irradiance_probe_mark_dirty(lighting_irradiance_probe_handle_t handle);
int lighting_irradiance_probe_process_pending(void);

/* Statistics */
uint32_t lighting_irradiance_probe_get_count(void);
size_t lighting_irradiance_probe_get_memory_usage(void);
void lighting_irradiance_probe_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_IRRADIANCE_PROBE_H */

/*
 * hardware_raster.h
 * Hardware rasterization path
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_HARDWARE_RASTER_H
#define NANITE_HARDWARE_RASTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_hardware_raster_handle {
    uint32_t id;
} nanite_hardware_raster_handle_t;

typedef struct nanite_hardware_raster_desc {
    uint32_t flags;
    void* user_data;
} nanite_hardware_raster_desc_t;

typedef struct nanite_hardware_raster_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_hardware_raster_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_hardware_raster_init(void);
void nanite_hardware_raster_shutdown(void);

/* Lifecycle */
int nanite_hardware_raster_create(nanite_hardware_raster_handle_t* out_handle, const nanite_hardware_raster_desc_t* desc);
void nanite_hardware_raster_destroy(nanite_hardware_raster_handle_t handle);

/* Operations */
int nanite_hardware_raster_update(nanite_hardware_raster_handle_t handle, const void* data, size_t size);
bool nanite_hardware_raster_is_valid(nanite_hardware_raster_handle_t handle);
int nanite_hardware_raster_get_info(nanite_hardware_raster_handle_t handle, nanite_hardware_raster_info_t* out_info);
void nanite_hardware_raster_mark_dirty(nanite_hardware_raster_handle_t handle);
int nanite_hardware_raster_process_pending(void);

/* Statistics */
uint32_t nanite_hardware_raster_get_count(void);
size_t nanite_hardware_raster_get_memory_usage(void);
void nanite_hardware_raster_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_HARDWARE_RASTER_H */

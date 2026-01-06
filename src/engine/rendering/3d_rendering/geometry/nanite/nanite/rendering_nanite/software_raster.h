/*
 * software_raster.h
 * Software rasterization
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_SOFTWARE_RASTER_H
#define NANITE_SOFTWARE_RASTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_software_raster_handle {
    uint32_t id;
} nanite_software_raster_handle_t;

typedef struct nanite_software_raster_desc {
    uint32_t flags;
    void* user_data;
} nanite_software_raster_desc_t;

typedef struct nanite_software_raster_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_software_raster_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_software_raster_init(void);
void nanite_software_raster_shutdown(void);

/* Lifecycle */
int nanite_software_raster_create(nanite_software_raster_handle_t* out_handle, const nanite_software_raster_desc_t* desc);
void nanite_software_raster_destroy(nanite_software_raster_handle_t handle);

/* Operations */
int nanite_software_raster_update(nanite_software_raster_handle_t handle, const void* data, size_t size);
bool nanite_software_raster_is_valid(nanite_software_raster_handle_t handle);
int nanite_software_raster_get_info(nanite_software_raster_handle_t handle, nanite_software_raster_info_t* out_info);
void nanite_software_raster_mark_dirty(nanite_software_raster_handle_t handle);
int nanite_software_raster_process_pending(void);

/* Statistics */
uint32_t nanite_software_raster_get_count(void);
size_t nanite_software_raster_get_memory_usage(void);
void nanite_software_raster_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_SOFTWARE_RASTER_H */

/*
 * software_rasterizer.h
 * Software depth rasterizer
 *
 * Part of the Culling subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CULLING_SOFTWARE_RASTERIZER_H
#define CULLING_SOFTWARE_RASTERIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct culling_software_rasterizer_handle {
    uint32_t id;
} culling_software_rasterizer_handle_t;

typedef struct culling_software_rasterizer_desc {
    uint32_t flags;
    void* user_data;
} culling_software_rasterizer_desc_t;

typedef struct culling_software_rasterizer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} culling_software_rasterizer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int culling_software_rasterizer_init(void);
void culling_software_rasterizer_shutdown(void);

/* Lifecycle */
int culling_software_rasterizer_create(culling_software_rasterizer_handle_t* out_handle, const culling_software_rasterizer_desc_t* desc);
void culling_software_rasterizer_destroy(culling_software_rasterizer_handle_t handle);

/* Operations */
int culling_software_rasterizer_update(culling_software_rasterizer_handle_t handle, const void* data, size_t size);
bool culling_software_rasterizer_is_valid(culling_software_rasterizer_handle_t handle);
int culling_software_rasterizer_get_info(culling_software_rasterizer_handle_t handle, culling_software_rasterizer_info_t* out_info);
void culling_software_rasterizer_mark_dirty(culling_software_rasterizer_handle_t handle);
int culling_software_rasterizer_process_pending(void);

/* Statistics */
uint32_t culling_software_rasterizer_get_count(void);
size_t culling_software_rasterizer_get_memory_usage(void);
void culling_software_rasterizer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CULLING_SOFTWARE_RASTERIZER_H */

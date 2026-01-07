/*
 * sw_rasterizer.h
 * Software rasterizer
 *
 * Part of the Occlusion subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef OCCLUSION_SW_RASTERIZER_H
#define OCCLUSION_SW_RASTERIZER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct occlusion_sw_rasterizer_handle {
    uint32_t id;
} occlusion_sw_rasterizer_handle_t;

typedef struct occlusion_sw_rasterizer_desc {
    uint32_t flags;
    void* user_data;
} occlusion_sw_rasterizer_desc_t;

typedef struct occlusion_sw_rasterizer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} occlusion_sw_rasterizer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int occlusion_sw_rasterizer_init(void);
void occlusion_sw_rasterizer_shutdown(void);

/* Lifecycle */
int occlusion_sw_rasterizer_create(occlusion_sw_rasterizer_handle_t* out_handle, const occlusion_sw_rasterizer_desc_t* desc);
void occlusion_sw_rasterizer_destroy(occlusion_sw_rasterizer_handle_t handle);

/* Operations */
int occlusion_sw_rasterizer_update(occlusion_sw_rasterizer_handle_t handle, const void* data, size_t size);
bool occlusion_sw_rasterizer_is_valid(occlusion_sw_rasterizer_handle_t handle);
int occlusion_sw_rasterizer_get_info(occlusion_sw_rasterizer_handle_t handle, occlusion_sw_rasterizer_info_t* out_info);
void occlusion_sw_rasterizer_mark_dirty(occlusion_sw_rasterizer_handle_t handle);
int occlusion_sw_rasterizer_process_pending(void);

/* Statistics */
uint32_t occlusion_sw_rasterizer_get_count(void);
size_t occlusion_sw_rasterizer_get_memory_usage(void);
void occlusion_sw_rasterizer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSION_SW_RASTERIZER_H */

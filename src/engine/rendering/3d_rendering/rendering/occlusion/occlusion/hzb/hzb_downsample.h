/*
 * hzb_downsample.h
 * HZB downsampling
 *
 * Part of the Occlusion subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef OCCLUSION_HZB_DOWNSAMPLE_H
#define OCCLUSION_HZB_DOWNSAMPLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct occlusion_hzb_downsample_handle {
    uint32_t id;
} occlusion_hzb_downsample_handle_t;

typedef struct occlusion_hzb_downsample_desc {
    uint32_t flags;
    void* user_data;
} occlusion_hzb_downsample_desc_t;

typedef struct occlusion_hzb_downsample_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} occlusion_hzb_downsample_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int occlusion_hzb_downsample_init(void);
void occlusion_hzb_downsample_shutdown(void);

/* Lifecycle */
int occlusion_hzb_downsample_create(occlusion_hzb_downsample_handle_t* out_handle, const occlusion_hzb_downsample_desc_t* desc);
void occlusion_hzb_downsample_destroy(occlusion_hzb_downsample_handle_t handle);

/* Operations */
int occlusion_hzb_downsample_update(occlusion_hzb_downsample_handle_t handle, const void* data, size_t size);
bool occlusion_hzb_downsample_is_valid(occlusion_hzb_downsample_handle_t handle);
int occlusion_hzb_downsample_get_info(occlusion_hzb_downsample_handle_t handle, occlusion_hzb_downsample_info_t* out_info);
void occlusion_hzb_downsample_mark_dirty(occlusion_hzb_downsample_handle_t handle);
int occlusion_hzb_downsample_process_pending(void);

/* Statistics */
uint32_t occlusion_hzb_downsample_get_count(void);
size_t occlusion_hzb_downsample_get_memory_usage(void);
void occlusion_hzb_downsample_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSION_HZB_DOWNSAMPLE_H */

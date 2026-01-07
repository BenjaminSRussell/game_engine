/*
 * gpu_scene.h
 * GPU scene representation
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_GPU_SCENE_H
#define RENDERING_GPU_SCENE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct rendering_gpu_scene_handle {
    uint32_t id;
} rendering_gpu_scene_handle_t;

typedef struct rendering_gpu_scene_desc {
    uint32_t flags;
    void* user_data;
} rendering_gpu_scene_desc_t;

typedef struct rendering_gpu_scene_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} rendering_gpu_scene_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int rendering_gpu_scene_init(void);
void rendering_gpu_scene_shutdown(void);

/* Lifecycle */
int rendering_gpu_scene_create(rendering_gpu_scene_handle_t* out_handle, const rendering_gpu_scene_desc_t* desc);
void rendering_gpu_scene_destroy(rendering_gpu_scene_handle_t handle);

/* Operations */
int rendering_gpu_scene_update(rendering_gpu_scene_handle_t handle, const void* data, size_t size);
bool rendering_gpu_scene_is_valid(rendering_gpu_scene_handle_t handle);
int rendering_gpu_scene_get_info(rendering_gpu_scene_handle_t handle, rendering_gpu_scene_info_t* out_info);
void rendering_gpu_scene_mark_dirty(rendering_gpu_scene_handle_t handle);
int rendering_gpu_scene_process_pending(void);

/* Statistics */
uint32_t rendering_gpu_scene_get_count(void);
size_t rendering_gpu_scene_get_memory_usage(void);
void rendering_gpu_scene_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* RENDERING_GPU_SCENE_H */

/*
 * gpu_skinning.h
 * GPU bone matrix upload
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef ANIMATION_GPU_SKINNING_H
#define ANIMATION_GPU_SKINNING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct animation_gpu_skinning_handle {
    uint32_t id;
} animation_gpu_skinning_handle_t;

typedef struct animation_gpu_skinning_desc {
    uint32_t flags;
    void* user_data;
} animation_gpu_skinning_desc_t;

typedef struct animation_gpu_skinning_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} animation_gpu_skinning_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int animation_gpu_skinning_init(void);
void animation_gpu_skinning_shutdown(void);

/* Lifecycle */
int animation_gpu_skinning_create(animation_gpu_skinning_handle_t* out_handle, const animation_gpu_skinning_desc_t* desc);
void animation_gpu_skinning_destroy(animation_gpu_skinning_handle_t handle);

/* Operations */
int animation_gpu_skinning_update(animation_gpu_skinning_handle_t handle, const void* data, size_t size);
bool animation_gpu_skinning_is_valid(animation_gpu_skinning_handle_t handle);
int animation_gpu_skinning_get_info(animation_gpu_skinning_handle_t handle, animation_gpu_skinning_info_t* out_info);
void animation_gpu_skinning_mark_dirty(animation_gpu_skinning_handle_t handle);
int animation_gpu_skinning_process_pending(void);

/* Statistics */
uint32_t animation_gpu_skinning_get_count(void);
size_t animation_gpu_skinning_get_memory_usage(void);
void animation_gpu_skinning_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_GPU_SKINNING_H */

/*
 * gpu_allocator.h
 * GPU memory allocation strategies
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_GPU_ALLOCATOR_H
#define CORE_GPU_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_gpu_allocator_handle {
    uint32_t id;
} core_gpu_allocator_handle_t;

typedef struct core_gpu_allocator_desc {
    uint32_t flags;
    void* user_data;
} core_gpu_allocator_desc_t;

typedef struct core_gpu_allocator_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} core_gpu_allocator_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_gpu_allocator_init(void);
void core_gpu_allocator_shutdown(void);

/* Lifecycle */
int core_gpu_allocator_create(core_gpu_allocator_handle_t* out_handle, const core_gpu_allocator_desc_t* desc);
void core_gpu_allocator_destroy(core_gpu_allocator_handle_t handle);

/* Operations */
int core_gpu_allocator_update(core_gpu_allocator_handle_t handle, const void* data, size_t size);
bool core_gpu_allocator_is_valid(core_gpu_allocator_handle_t handle);
int core_gpu_allocator_get_info(core_gpu_allocator_handle_t handle, core_gpu_allocator_info_t* out_info);
void core_gpu_allocator_mark_dirty(core_gpu_allocator_handle_t handle);
int core_gpu_allocator_process_pending(void);

/* Statistics */
uint32_t core_gpu_allocator_get_count(void);
size_t core_gpu_allocator_get_memory_usage(void);
void core_gpu_allocator_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_GPU_ALLOCATOR_H */

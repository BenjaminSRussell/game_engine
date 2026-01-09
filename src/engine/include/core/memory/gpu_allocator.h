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

typedef enum gpu_allocation_flags {
    GPU_ALLOCATION_FLAG_DEVICE_LOCAL = 1 << 0,
    GPU_ALLOCATION_FLAG_HOST_VISIBLE = 1 << 1,
    GPU_ALLOCATION_FLAG_HOST_COHERENT = 1 << 2,
    GPU_ALLOCATION_FLAG_MAPPED = 1 << 3,
    GPU_ALLOCATION_FLAG_DEDICATED = 1 << 4
} gpu_allocation_flags_t;

typedef struct gpu_allocation {
    uint64_t offset;
    uint64_t size;
    uint32_t heap_index;
    uint32_t flags;
    void* mapped_ptr;  // NULL if not mapped
    void* backend_handle; // VkDeviceMemory, etc.
} gpu_allocation_t;

typedef struct render_memory_allocator {
    void* backend_handle;
    uint64_t total_size;
    uint64_t allocated_size;
    uint32_t allocation_count;
} render_memory_allocator_t;

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
int core_gpu_allocator_alloc(core_gpu_allocator_handle_t handle, uint64_t size, uint32_t alignment, gpu_allocation_t* out_allocation);
void core_gpu_allocator_free(core_gpu_allocator_handle_t handle, gpu_allocation_t* allocation);

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

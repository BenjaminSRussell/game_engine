/*
 * mtl_memory_heap.h
 * Metal memory heap management interface
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_MEMORY_HEAP_H
#define PLATFORM_MTL_MEMORY_HEAP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Opaque heap handle */
typedef void* mtl_heap_t;          /* Maps to id<MTLHeap> */
typedef void* mtl_buffer_t;        /* Maps to id<MTLBuffer> */
typedef void* mtl_texture_t;       /* Maps to id<MTLTexture> */

/* Storage Mode */
typedef enum mtl_storage_mode {
    MTL_STORAGE_SHARED = 0,        /* CPU & GPU accessible, coherent */
    MTL_STORAGE_PRIVATE = 1,       /* GPU-only, best performance */
    MTL_STORAGE_MEMORYLESS = 2,    /* Tile memory, Apple Silicon only */
} mtl_storage_mode_t;

/* CPU Cache Mode */
typedef enum mtl_cpu_cache_mode {
    MTL_CPU_CACHE_DEFAULT_CACHE = 0,
    MTL_CPU_CACHE_WRITE_COMBINED = 1,
} mtl_cpu_cache_mode_t;

typedef struct mtl_memory_info {
    uint64_t current_allocated_size;
    uint64_t recommended_working_set_size;
    bool has_unified_memory;
    
    // Heap Usage Statistics
    uint64_t shared_heap_size;
    uint64_t private_heap_size;
    uint64_t memoryless_heap_size;
    
    // Memory Pressure
    // 0 = Normal, 1 = Warning, 2 = Critical
    uint32_t memory_pressure_level; 
} mtl_memory_info_t;

/* Heap Descriptor */
typedef struct mtl_heap_desc {
    uint64_t size;
    mtl_storage_mode_t storage_mode;
    mtl_cpu_cache_mode_t cpu_cache_mode;
    const char* label;  /* Optional debug label */
} mtl_heap_desc_t;

/* Heap Usage Tracking */
typedef struct mtl_heap_usage {
    uint64_t total_size;
    uint64_t used_size;
    uint64_t available_size;
    uint32_t num_allocations;
    uint64_t peak_usage;
} mtl_heap_usage_t;

/* Memory Budget */
typedef struct mtl_memory_budget {
    uint64_t total_budget;
    uint64_t current_usage;
    uint64_t available;
    bool is_over_budget;
} mtl_memory_budget_t;

/* Memory Warning Callback */
typedef void (*mtl_memory_warning_callback_t)(void* user_data, uint32_t pressure_level);

/* ============================================================================
 * API - Memory Information
 * ============================================================================ */

/**
 * Updates and retrieves the current memory statistics for the device.
 * @param device The metal device (opaque pointer from mtl_device.h)
 * @param out_info Pointer to the structure to fill with memory info.
 */
// This is implemented in mtl_device.c: metal_device_get_memory_info()

/* ============================================================================
 * API - Heap Management
 * ============================================================================ */

/**
 * Creates a memory heap for resource pooling.
 * @param device The metal device.
 * @param desc Heap descriptor.
 * @return Heap handle, or NULL on failure.
 */
mtl_heap_t mtl_heap_create(void* device, const mtl_heap_desc_t* desc);

/**
 * Destroys a memory heap and all resources allocated from it.
 * @param heap Heap handle.
 */
void mtl_heap_destroy(mtl_heap_t heap);

/**
 * Gets the maximum available size for new allocations from the heap.
 * @param heap Heap handle.
 * @return Available size in bytes.
 */
uint64_t mtl_heap_get_available_size(mtl_heap_t heap);

/**
 * Gets the total size of the heap.
 * @param heap Heap handle.
 * @return Total size in bytes.
 */
uint64_t mtl_heap_get_size(mtl_heap_t heap);

/**
 * Gets the current usage of the heap.
 * @param heap Heap handle.
 * @return Used size in bytes.
 */
uint64_t mtl_heap_get_used_size(mtl_heap_t heap);

/**
 * Retrieves detailed heap usage statistics.
 * @param heap Heap handle.
 * @param out_usage Pointer to structure to fill.
 */
void mtl_heap_get_usage(mtl_heap_t heap, mtl_heap_usage_t* out_usage);

/**
 * Validates heap integrity.
 * @param heap Heap handle.
 * @return true if valid, false if corrupted or invalid.
 */
bool mtl_heap_validate(mtl_heap_t heap);

/* ============================================================================
 * API - Heap-Based Allocation
 * ============================================================================ */

/**
 * Allocates a buffer from the heap.
 * @param heap Heap handle.
 * @param size Buffer size in bytes.
 * @param alignment Alignment requirement (0 for default).
 * @return Buffer handle, or NULL on failure.
 */
mtl_buffer_t mtl_heap_allocate_buffer(mtl_heap_t heap, uint64_t size, uint64_t alignment);

/**
 * Allocates a texture from the heap.
 * @param heap Heap handle.
 * @param descriptor Texture descriptor (platform-specific, opaque).
 * @return Texture handle, or NULL on failure.
 */
mtl_texture_t mtl_heap_allocate_texture(mtl_heap_t heap, void* descriptor);

/* ============================================================================
 * API - Memory Budget Management
 * ============================================================================ */

/**
 * Sets the memory budget for the application.
 * @param device The metal device.
 * @param budget_bytes Maximum allowed memory usage in bytes.
 */
void mtl_memory_set_budget(void* device, uint64_t budget_bytes);

/**
 * Gets the current memory budget status.
 * @param device The metal device.
 * @param out_budget Pointer to structure to fill.
 */
void mtl_memory_get_budget(void* device, mtl_memory_budget_t* out_budget);

/**
 * Checks if an allocation of given size would exceed the budget.
 * @param device The metal device.
 * @param size Allocation size in bytes.
 * @return true if allocation would exceed budget.
 */
bool mtl_memory_would_exceed_budget(void* device, uint64_t size);

/**
 * Registers a callback for memory pressure warnings.
 * @param device The metal device.
 * @param callback Callback function.
 * @param user_data User data to pass to callback.
 */
void mtl_memory_register_warning_callback(void* device, mtl_memory_warning_callback_t callback, void* user_data);

/* ============================================================================
 * API - Memory Statistics
 * ============================================================================ */

/**
 * Prints memory statistics to a buffer.
 * @param device The metal device.
 * @param buffer Output buffer.
 * @param buffer_size Size of buffer.
 * @return Number of bytes written.
 */
size_t mtl_memory_export_stats(void* device, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_MEMORY_HEAP_H */

#ifndef GPU_MEMORY_VALIDATION_H
#define GPU_MEMORY_VALIDATION_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// GPU memory types
typedef enum {
    GPU_MEMORY_TYPE_VERTEX_BUFFER,
    GPU_MEMORY_TYPE_INDEX_BUFFER,
    GPU_MEMORY_TYPE_UNIFORM_BUFFER,
    GPU_MEMORY_TYPE_STORAGE_BUFFER,
    GPU_MEMORY_TYPE_TEXTURE,
    GPU_MEMORY_TYPE_RENDER_TARGET,
    GPU_MEMORY_TYPE_DEPTH_STENCIL,
    GPU_MEMORY_TYPE_COMPUTE_BUFFER,
    GPU_MEMORY_TYPE_STREAMING,
    GPU_MEMORY_TYPE_STAGING
} gpu_memory_type_t;

// GPU memory allocation flags
typedef enum {
    GPU_MEMORY_FLAG_NONE = 0x0,
    GPU_MEMORY_FLAG_DEVICE_LOCAL = 0x1,
    GPU_MEMORY_FLAG_HOST_VISIBLE = 0x2,
    GPU_MEMORY_FLAG_HOST_COHERENT = 0x4,
    GPU_MEMORY_FLAG_HOST_CACHED = 0x8,
    GPU_MEMORY_FLAG_LAZILY_ALLOCATED = 0x10,
    GPU_MEMORY_FLAG_PROTECTED = 0x20,
    GPU_MEMORY_FLAG_TRANSIENT = 0x40
} gpu_memory_flags_t;

// GPU memory heap types
typedef enum {
    GPU_MEMORY_HEAP_DEVICE_LOCAL,
    GPU_MEMORY_HEAP_HOST_VISIBLE,
    GPU_MEMORY_HEAP_HOST_COHERENT,
    GPU_MEMORY_HEAP_HOST_CACHED,
    GPU_MEMORY_HEAP_LAZILY_ALLOCATED,
    GPU_MEMORY_HEAP_COUNT
} gpu_memory_heap_t;

// Forward declarations
typedef struct gpu_memory_allocator_t gpu_memory_allocator_t;
typedef struct gpu_memory_allocation_t gpu_memory_allocation_t;
typedef struct gpu_memory_pool_t gpu_memory_pool_t;

// GPU memory allocation descriptor
typedef struct {
    uint64_t size;
    uint64_t alignment;
    gpu_memory_type_t type;
    gpu_memory_flags_t flags;
    gpu_memory_heap_t preferred_heap;
    const char* name;
    const char* file;
    uint32_t line;
} gpu_memory_alloc_desc_t;

// GPU memory allocation information
typedef struct {
    uint64_t allocation_id;
    uint64_t size;
    uint64_t aligned_size;
    gpu_memory_type_t type;
    gpu_memory_flags_t flags;
    gpu_memory_heap_t heap;
    char* name;
    char* file;
    uint32_t line;
    uint64_t timestamp;
    void* device_pointer;
    void* host_pointer;
    bool is_mapped;
    uint32_t ref_count;
} gpu_memory_allocation_info_t;

// GPU memory heap statistics
typedef struct {
    uint64_t total_size;
    uint64_t used_size;
    uint64_t free_size;
    uint64_t peak_usage;
    uint32_t allocation_count;
    uint32_t fragmentation_count;
    double fragmentation_ratio;
} gpu_memory_heap_stats_t;

// GPU memory allocator statistics
typedef struct {
    uint64_t total_allocated;
    uint64_t total_freed;
    uint64_t current_usage;
    uint64_t peak_usage;
    uint32_t total_allocations;
    uint32_t active_allocations;
    uint32_t failed_allocations;
    uint64_t total_allocation_time_ns;
    uint64_t total_deallocation_time_ns;
    gpu_memory_heap_stats_t heap_stats[GPU_MEMORY_HEAP_COUNT];
} gpu_memory_allocator_stats_t;

// Memory leak detection
typedef struct {
    gpu_memory_allocation_info_t* allocations;
    uint32_t count;
    uint32_t capacity;
} gpu_memory_leak_report_t;

// Error codes
typedef enum {
    GPU_MEMORY_SUCCESS = 0,
    GPU_MEMORY_ERROR_INVALID_PARAM = -1,
    GPU_MEMORY_ERROR_OUT_OF_MEMORY = -2,
    GPU_MEMORY_ERROR_INVALID_ALIGNMENT = -3,
    GPU_MEMORY_ERROR_INVALID_HEAP = -4,
    GPU_MEMORY_ERROR_ALLOCATION_FAILED = -5,
    GPU_MEMORY_ERROR_NOT_FOUND = -6,
    GPU_MEMORY_ERROR_ALREADY_MAPPED = -7,
    GPU_MEMORY_ERROR_NOT_MAPPED = -8,
    GPU_MEMORY_ERROR_INVALID_FLAGS = -9,
    GPU_MEMORY_ERROR_NOT_INITIALIZED = -10
} gpu_memory_error_t;

// Allocator lifecycle
gpu_memory_error_t gpu_memory_allocator_init(gpu_memory_allocator_t** allocator, uint64_t total_memory);
void gpu_memory_allocator_shutdown(gpu_memory_allocator_t* allocator);

// Memory allocation
gpu_memory_error_t gpu_memory_allocate(gpu_memory_allocator_t* allocator, 
                                      const gpu_memory_alloc_desc_t* desc, 
                                      gpu_memory_allocation_t** allocation);
gpu_memory_error_t gpu_memory_free(gpu_memory_allocator_t* allocator, gpu_memory_allocation_t* allocation);
gpu_memory_error_t gpu_memory_reallocate(gpu_memory_allocator_t* allocator,
                                        gpu_memory_allocation_t* allocation,
                                        uint64_t new_size);

// Memory mapping
gpu_memory_error_t gpu_memory_map(gpu_memory_allocator_t* allocator, 
                                 gpu_memory_allocation_t* allocation, 
                                 void** mapped_ptr);
gpu_memory_error_t gpu_memory_unmap(gpu_memory_allocator_t* allocator, 
                                   gpu_memory_allocation_t* allocation);
gpu_memory_error_t gpu_memory_flush_range(gpu_memory_allocator_t* allocator,
                                        gpu_memory_allocation_t* allocation,
                                        uint64_t offset,
                                        uint64_t size);
gpu_memory_error_t gpu_memory_invalidate_range(gpu_memory_allocator_t* allocator,
                                             gpu_memory_allocation_t* allocation,
                                             uint64_t offset,
                                             uint64_t size);

// Memory validation
gpu_memory_error_t gpu_memory_validate_allocation(gpu_memory_allocator_t* allocator,
                                                gpu_memory_allocation_t* allocation);
gpu_memory_error_t gpu_memory_validate_all_allocations(gpu_memory_allocator_t* allocator);
gpu_memory_error_t gpu_memory_check_corruption(gpu_memory_allocator_t* allocator,
                                              gpu_memory_allocation_t* allocation);

// Memory pools
gpu_memory_error_t gpu_memory_pool_create(gpu_memory_allocator_t* allocator,
                                         uint64_t pool_size,
                                         uint64_t block_size,
                                         gpu_memory_type_t type,
                                         gpu_memory_pool_t** pool);
gpu_memory_error_t gpu_memory_pool_destroy(gpu_memory_allocator_t* allocator,
                                          gpu_memory_pool_t* pool);
gpu_memory_error_t gpu_memory_pool_allocate(gpu_memory_pool_t* pool,
                                           gpu_memory_allocation_t** allocation);
gpu_memory_error_t gpu_memory_pool_free(gpu_memory_pool_t* pool,
                                       gpu_memory_allocation_t* allocation);

// Statistics and monitoring
gpu_memory_error_t gpu_memory_get_statistics(gpu_memory_allocator_t* allocator,
                                            gpu_memory_allocator_stats_t* stats);
gpu_memory_error_t gpu_memory_reset_statistics(gpu_memory_allocator_t* allocator);
gpu_memory_error_t gpu_memory_get_allocation_info(gpu_memory_allocator_t* allocator,
                                                gpu_memory_allocation_t* allocation,
                                                gpu_memory_allocation_info_t* info);

// Leak detection
gpu_memory_error_t gpu_memory_detect_leaks(gpu_memory_allocator_t* allocator,
                                          gpu_memory_leak_report_t* report);
gpu_memory_error_t gpu_memory_free_leak_report(gpu_memory_leak_report_t* report);

// Debugging and diagnostics
gpu_memory_error_t gpu_memory_debug_print_allocations(gpu_memory_allocator_t* allocator);
gpu_memory_error_t gpu_memory_debug_print_heap_info(gpu_memory_allocator_t* allocator);
gpu_memory_error_t gpu_memory_debug_dump_memory_map(gpu_memory_allocator_t* allocator);
gpu_memory_error_t gpu_memory_debug_write_memory_pattern(gpu_memory_allocator_t* allocator,
                                                        gpu_memory_allocation_t* allocation,
                                                        uint8_t pattern);

// Memory budgeting
gpu_memory_error_t gpu_memory_set_budget(gpu_memory_allocator_t* allocator,
                                        gpu_memory_heap_t heap,
                                        uint64_t budget);
gpu_memory_error_t gpu_memory_get_budget(gpu_memory_allocator_t* allocator,
                                        gpu_memory_heap_t heap,
                                        uint64_t* budget);
gpu_memory_error_t gpu_memory_get_usage(gpu_memory_allocator_t* allocator,
                                      gpu_memory_heap_t heap,
                                      uint64_t* usage);

// Memory fragmentation analysis
gpu_memory_error_t gpu_memory_analyze_fragmentation(gpu_memory_allocator_t* allocator,
                                                   gpu_memory_heap_t heap,
                                                   double* fragmentation_ratio);
gpu_memory_error_t gpu_memory_defragment(gpu_memory_allocator_t* allocator,
                                        gpu_memory_heap_t heap);

// Reference counting
gpu_memory_error_t gpu_memory_add_ref(gpu_memory_allocation_t* allocation);
gpu_memory_error_t gpu_memory_release(gpu_memory_allocator_t* allocator,
                                     gpu_memory_allocation_t* allocation);
uint32_t gpu_memory_get_ref_count(gpu_memory_allocation_t* allocation);

// Error handling
const char* gpu_memory_error_string(gpu_memory_error_t error);

// Utility macros
#define GPU_MEMORY_ALLOCATE(allocator, size, type, allocation) \
    gpu_memory_allocate(allocator, &(gpu_memory_alloc_desc_t){ \
        .size = size, \
        .type = type, \
        .name = #allocation, \
        .file = __FILE__, \
        .line = __LINE__ \
    }, allocation)

#define GPU_MEMORY_VALIDATE(allocator, allocation) \
    gpu_memory_validate_allocation(allocator, allocation)

#ifdef __cplusplus
}
#endif

#endif // GPU_MEMORY_VALIDATION_H

/*
 * unified_memory.h
 * Unified memory management system - consolidates memory allocation across all subsystems
 * 
 * This header provides common memory management utilities, tracking, and allocation
 * strategies to eliminate code duplication across all engine systems.
 */

#ifndef UNIFIED_MEMORY_H
#define UNIFIED_MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * MEMORY MANAGEMENT CONSTANTS
 * ============================================================================ */

#define UNIFIED_MEMORY_ALIGNMENT 16
#define UNIFIED_MEMORY_DEFAULT_POOL_SIZE (64 * 1024 * 1024)  /* 64MB */
#define UNIFIED_MEMORY_MAX_POOLS 32
#define UNIFIED_MEMORY_MAX_ALLOCATIONS 1000000
#define UNIFIED_MEMORY_GUARD_MAGIC 0xDEADBEEF
#define UNIFIED_MEMORY_CANARY_MAGIC 0xFEEDFACE
#define UNIFIED_MEMORY_CALLSTACK_DEPTH 16

/* ============================================================================
 * MEMORY ALLOCATION TYPES
 * ============================================================================ */

typedef enum {
    MEMORY_ALLOC_TYPE_STANDARD = 0,
    MEMORY_ALLOC_TYPE_POOLED,
    MEMORY_ALLOC_TYPE_GPU,
    MEMORY_ALLOC_TYPE_STREAMING,
    MEMORY_ALLOC_TYPE_TEMPORARY,
    MEMORY_ALLOC_TYPE_COUNT
} memory_alloc_type_t;

typedef enum {
    MEMORY_USAGE_GENERAL = 0,
    MEMORY_USAGE_VERTEX_BUFFER,
    MEMORY_USAGE_INDEX_BUFFER,
    MEMORY_USAGE_UNIFORM_BUFFER,
    MEMORY_USAGE_TEXTURE,
    MEMORY_USAGE_AUDIO,
    MEMORY_USAGE_PHYSICS,
    MEMORY_USAGE_ANIMATION,
    MEMORY_USAGE_AI,
    MEMORY_USAGE_COUNT
} memory_usage_type_t;

/* ============================================================================
 * MEMORY ALLOCATION TRACKING
 * ============================================================================ */

typedef struct {
    void *ptr;
    size_t size;
    memory_alloc_type_t alloc_type;
    memory_usage_type_t usage_type;
    const char *file;
    int line;
    const char *function;
    uint64_t timestamp;
    uint32_t callstack[UNIFIED_MEMORY_CALLSTACK_DEPTH];
    uint32_t callstack_depth;
    uint32_t guard_magic;
    uint32_t canary_magic;
} memory_allocation_t;

typedef struct {
    size_t total_allocated;
    size_t peak_allocated;
    size_t current_allocated;
    uint32_t allocation_count;
    uint32_t peak_allocation_count;
    uint32_t failed_allocations;
    size_t fragmentation_bytes;
    float fragmentation_ratio;
} memory_stats_t;

typedef struct {
    memory_usage_type_t usage_type;
    size_t total_allocated;
    size_t peak_allocated;
    size_t current_allocated;
    uint32_t allocation_count;
    char name[64];
} memory_usage_stats_t;

/* ============================================================================
 * MEMORY POOL SYSTEM
 * ============================================================================ */

typedef struct memory_pool {
    void *memory;
    size_t size;
    size_t used;
    size_t peak_used;
    uint32_t allocation_count;
    uint32_t alignment;
    bool is_dynamic;
    char name[64];
    struct memory_pool *next;
} memory_pool_t;

/* ============================================================================
 * MEMORY BUDGET SYSTEM
 * ============================================================================ */

typedef struct {
    size_t budget_limit;
    size_t current_usage;
    float usage_ratio;
    bool enforcement_enabled;
    memory_usage_type_t usage_type;
} memory_budget_t;

/* ============================================================================
 * UNIFIED MEMORY API
 * ============================================================================ */

/* Core allocation functions */
void* unified_malloc(size_t size, memory_alloc_type_t alloc_type, memory_usage_type_t usage_type,
                     const char *file, int line, const char *function);
void* unified_calloc(size_t count, size_t size, memory_alloc_type_t alloc_type, memory_usage_type_t usage_type,
                     const char *file, int line, const char *function);
void* unified_realloc(void *ptr, size_t new_size, const char *file, int line, const char *function);
void unified_free(void *ptr, const char *file, int line, const char *function);

/* Memory pool functions */
memory_pool_t* unified_pool_create(size_t size, uint32_t alignment, const char *name);
void unified_pool_destroy(memory_pool_t *pool);
void* unified_pool_alloc(memory_pool_t *pool, size_t size, const char *file, int line, const char *function);
void unified_pool_reset(memory_pool_t *pool);

/* Memory budget functions */
void unified_memory_set_budget(memory_usage_type_t usage_type, size_t budget);
bool unified_memory_check_budget(memory_usage_type_t usage_type, size_t requested_size);
void unified_memory_get_budget(memory_usage_type_t usage_type, memory_budget_t *budget);

/* Statistics and monitoring */
void unified_memory_get_stats(memory_stats_t *stats);
void unified_memory_get_usage_stats(memory_usage_stats_t *stats, uint32_t *count);
void unified_memory_reset_stats(void);
void unified_memory_dump_leaks(void);
void unified_memory_dump_stats(void);

/* Memory validation */
bool unified_memory_validate_allocation(void *ptr);
bool unified_memory_validate_all(void);
void unified_memory_check_corruption(void);

/* Memory optimization */
void unified_memory_defragment(void);
void unified_memory_garbage_collect(void);
size_t unified_memory_get_largest_free_block(void);

/* ============================================================================
 * CONVENIENCE MACROS (Consolidated from all systems)
 * ============================================================================ */

#define UNIFIED_MALLOC(size, usage) unified_malloc(size, MEMORY_ALLOC_TYPE_STANDARD, usage, __FILE__, __LINE__, __FUNCTION__)
#define UNIFIED_CALLOC(count, size, usage) unified_calloc(count, size, MEMORY_ALLOC_TYPE_STANDARD, usage, __FILE__, __LINE__, __FUNCTION__)
#define UNIFIED_REALLOC(ptr, size) unified_realloc(ptr, size, __FILE__, __LINE__, __FUNCTION__)
#define UNIFIED_FREE(ptr) unified_free(ptr, __FILE__, __LINE__, __FUNCTION__)

#define UNIFIED_POOL_ALLOC(pool, size) unified_pool_alloc(pool, size, __FILE__, __LINE__, __FUNCTION__)

#define UNIFIED_MALLOC_GPU(size) unified_malloc(size, MEMORY_ALLOC_TYPE_GPU, MEMORY_USAGE_VERTEX_BUFFER, __FILE__, __LINE__, __FUNCTION__)
#define UNIFIED_MALLOC_TEMP(size) unified_malloc(size, MEMORY_ALLOC_TYPE_TEMPORARY, MEMORY_USAGE_GENERAL, __FILE__, __LINE__, __FUNCTION__)

/* ============================================================================
 * THREAD-SAFE MEMORY OPERATIONS
 * ============================================================================ */

#ifdef _WIN32
#include <windows.h>
#define MEMORY_MUTEX_TYPE CRITICAL_SECTION
#define MEMORY_MUTEX_INIT(mutex) InitializeCriticalSection(&(mutex))
#define MEMORY_MUTEX_LOCK(mutex) EnterCriticalSection(&(mutex))
#define MEMORY_MUTEX_UNLOCK(mutex) LeaveCriticalSection(&(mutex))
#define MEMORY_MUTEX_DESTROY(mutex) DeleteCriticalSection(&(mutex))
#else
#include <pthread.h>
#define MEMORY_MUTEX_TYPE pthread_mutex_t
#define MEMORY_MUTEX_INIT(mutex) pthread_mutex_init(&(mutex), NULL)
#define MEMORY_MUTEX_LOCK(mutex) pthread_mutex_lock(&(mutex))
#define MEMORY_MUTEX_UNLOCK(mutex) pthread_mutex_unlock(&(mutex))
#define MEMORY_MUTEX_DESTROY(mutex) pthread_mutex_destroy(&(mutex))
#endif

/* Thread-safe allocation functions */
void* unified_malloc_thread_safe(size_t size, memory_alloc_type_t alloc_type, memory_usage_type_t usage_type);
void unified_free_thread_safe(void *ptr);

/* ============================================================================
 * MEMORY DEBUGGING UTILITIES
 * ============================================================================ */

/* Call stack capture */
void unified_memory_capture_callstack(uint32_t *callstack, uint32_t *depth);
void unified_memory_print_callstack(const uint32_t *callstack, uint32_t depth);

/* Memory corruption detection */
void unified_memory_set_guard(void *ptr, size_t size);
bool unified_memory_check_guard(void *ptr);
void unified_memory_set_canary(void *ptr, size_t size);
bool unified_memory_check_canary(void *ptr);

/* Memory profiling */
void unified_memory_start_profiling(void);
void unified_memory_stop_profiling(void);
void unified_memory_dump_profile(void);

/* ============================================================================
 * COMPATIBILITY LAYER (For existing code)
 * ============================================================================ */

/* Provide compatibility with existing malloc/free patterns */
static inline void* safe_malloc(size_t size) {
    return UNIFIED_MALLOC(size, MEMORY_USAGE_GENERAL);
}

static inline void* safe_calloc(size_t count, size_t size) {
    return UNIFIED_CALLOC(count, size, MEMORY_USAGE_GENERAL);
}

static inline void* safe_realloc(void *ptr, size_t size) {
    return UNIFIED_REALLOC(ptr, size);
}

static inline void safe_free(void *ptr) {
    UNIFIED_FREE(ptr);
}

/* ============================================================================
 * GPU MEMORY MANAGEMENT
 * ============================================================================ */

typedef struct {
    void *device_ptr;
    void *host_ptr;
    size_t size;
    uint32_t gpu_id;
    bool is_mapped;
} gpu_memory_block_t;

gpu_memory_block_t* unified_gpu_malloc(size_t size, uint32_t gpu_id);
void unified_gpu_free(gpu_memory_block_t *block);
void* unified_gpu_map(gpu_memory_block_t *block);
void unified_gpu_unmap(gpu_memory_block_t *block);

/* ============================================================================
 * STREAMING MEMORY MANAGEMENT
 * ============================================================================ */

typedef struct {
    void *memory;
    size_t size;
    size_t read_offset;
    size_t write_offset;
    bool is_circular;
    uint64_t access_count;
} streaming_buffer_t;

streaming_buffer_t* unified_streaming_buffer_create(size_t size, bool circular);
void unified_streaming_buffer_destroy(streaming_buffer_t *buffer);
void* unified_streaming_buffer_alloc(streaming_buffer_t *buffer, size_t size);
void unified_streaming_buffer_reset(streaming_buffer_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* UNIFIED_MEMORY_H */

/**
 * @file unified_allocator.c
 * @brief Implementation of unified memory allocator
 * @details Manages multiple allocation strategies with statistics and debugging
 */

#include "core/memory/unified_allocator.h"
#include "core/logging/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * ============================================================================
 * ALLOCATOR STATE
 * ============================================================================
 */

typedef struct {
    void* base;
    size_t capacity;
    size_t used;
    size_t peak;
    uint32_t allocation_count;
    uint32_t free_count;
    uint64_t total_allocated;
    uint64_t total_freed;
} LinearAllocatorImpl;

typedef struct {
    uint8_t* objects;
    bool* available;
    size_t object_size;
    uint32_t pool_size;
    uint32_t available_count;
    uint64_t total_allocated;
    uint64_t total_freed;
} PoolAllocatorImpl;

struct LinearAllocator {
    LinearAllocatorImpl impl;
};

struct PoolAllocator {
    PoolAllocatorImpl impl;
};

// Global allocator state
static struct {
    LinearAllocator linear_allocators[ALLOC_COUNT];
    PoolAllocator pool_allocators[ALLOC_COUNT];
    bool initialized;
    uint64_t init_time;
} g_allocator_state = { 0 };

/**
 * ============================================================================
 * INITIALIZATION
 * ============================================================================
 */

bool allocator_init(size_t initial_heap_size) {
    if (g_allocator_state.initialized) {
        LOG_WARN("MEMORY", "Allocator already initialized");
        return false;
    }

    // Initialize each allocator type with reasonable defaults
    const size_t allocator_sizes[] = {
        initial_heap_size / 2,      // ALLOC_GENERAL
        initial_heap_size / 4,      // ALLOC_POOL
        initial_heap_size / 8,      // ALLOC_LINEAR
        initial_heap_size / 8,      // ALLOC_TEMP
        initial_heap_size / 16,     // ALLOC_PERSISTENT
    };

    for (int i = 0; i < ALLOC_COUNT; i++) {
        void* base = malloc(allocator_sizes[i]);
        if (!base) {
            LOG_ERROR("MEMORY", "Failed to allocate memory for allocator %d", i);
            return false;
        }

        g_allocator_state.linear_allocators[i].impl.base = base;
        g_allocator_state.linear_allocators[i].impl.capacity = allocator_sizes[i];
        g_allocator_state.linear_allocators[i].impl.used = 0;
        g_allocator_state.linear_allocators[i].impl.peak = 0;
        memset(base, 0, allocator_sizes[i]);
    }

    g_allocator_state.initialized = true;
    g_allocator_state.init_time = (uint64_t)time(NULL);

    LOG_INFO("MEMORY", "Allocator initialized: %zu MB total", initial_heap_size / (1024 * 1024));
    return true;
}

void allocator_shutdown(void) {
    if (!g_allocator_state.initialized) {
        return;
    }

    for (int i = 0; i < ALLOC_COUNT; i++) {
        LinearAllocatorImpl* impl = &g_allocator_state.linear_allocators[i].impl;
        if (impl->base) {
            free(impl->base);
            impl->base = NULL;
        }
    }

    g_allocator_state.initialized = false;
    LOG_INFO("MEMORY", "Allocator shutdown complete");
}

/**
 * ============================================================================
 * GENERAL ALLOCATION
 * ============================================================================
 */

void* alloc(size_t size, AllocationType type) {
    if (!g_allocator_state.initialized) {
        LOG_ERROR("MEMORY", "Allocator not initialized");
        return NULL;
    }

    if (size == 0) {
        LOG_WARN("MEMORY", "Attempted to allocate 0 bytes");
        return NULL;
    }

    LinearAllocatorImpl* impl = &g_allocator_state.linear_allocators[type].impl;

    // Check if we have space
    if (impl->used + size > impl->capacity) {
        LOG_ERROR("MEMORY", "Allocator %d out of space: %zu + %zu > %zu",
                  type, impl->used, size, impl->capacity);
        return NULL;
    }

    void* ptr = (uint8_t*)impl->base + impl->used;
    impl->used += size;
    impl->peak = impl->used > impl->peak ? impl->used : impl->peak;
    impl->allocation_count++;
    impl->total_allocated += size;

    // Zero-initialize
    memset(ptr, 0, size);

    return ptr;
}

void* alloc_aligned(size_t size, size_t alignment, AllocationType type) {
    if (!g_allocator_state.initialized) {
        LOG_ERROR("MEMORY", "Allocator not initialized");
        return NULL;
    }

    LinearAllocatorImpl* impl = &g_allocator_state.linear_allocators[type].impl;

    // Calculate padding needed
    uintptr_t current = (uintptr_t)impl->base + impl->used;
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;

    if (impl->used + padding + size > impl->capacity) {
        LOG_ERROR("MEMORY", "Allocator %d out of space for aligned allocation", type);
        return NULL;
    }

    impl->used += padding + size;
    impl->peak = impl->used > impl->peak ? impl->used : impl->peak;
    impl->allocation_count++;
    impl->total_allocated += size;

    void* ptr = (void*)aligned;
    memset(ptr, 0, size);

    return ptr;
}

void free_alloc(void* ptr, AllocationType type) {
    if (!ptr || !g_allocator_state.initialized) {
        return;
    }

    LinearAllocatorImpl* impl = &g_allocator_state.linear_allocators[type].impl;
    impl->free_count++;
    // Note: Linear allocators don't actually free, they just track stats
}

void* realloc_alloc(void* ptr, size_t new_size, AllocationType type) {
    if (!ptr) {
        return alloc(new_size, type);
    }

    // For linear allocators, allocate new block
    void* new_ptr = alloc(new_size, type);
    if (!new_ptr) {
        return NULL;
    }

    // Copy old data (we don't know old size, so this is conservative)
    // In a production system, would track allocation sizes
    memcpy(new_ptr, ptr, new_size / 2);  // Copy half to be safe

    return new_ptr;
}

/**
 * ============================================================================
 * STATISTICS
 * ============================================================================
 */

AllocatorStats allocator_get_stats(AllocationType type) {
    AllocatorStats stats = { 0 };

    if (!g_allocator_state.initialized) {
        return stats;
    }

    LinearAllocatorImpl* impl = &g_allocator_state.linear_allocators[type].impl;

    stats.total_allocated = impl->total_allocated;
    stats.total_freed = impl->total_freed;
    stats.current_usage = impl->used;
    stats.peak_usage = impl->peak;
    stats.allocation_count = impl->allocation_count;
    stats.free_count = impl->free_count;
    stats.fragmentation_ratio = impl->capacity > 0 ?
        (float)(impl->capacity - impl->used) / impl->capacity : 0.0f;
    stats.largest_free_block = impl->capacity - impl->used;

    return stats;
}

AllocationMetrics allocator_get_metrics(void) {
    AllocationMetrics metrics = { 0 };

    if (!g_allocator_state.initialized) {
        return metrics;
    }

    metrics.total_engine_memory = 0;
    metrics.active_allocations = 0;

    for (int i = 0; i < ALLOC_COUNT; i++) {
        metrics.stats[i] = allocator_get_stats((AllocationType)i);
        metrics.total_engine_memory += metrics.stats[i].current_usage;
        metrics.active_allocations += metrics.stats[i].allocation_count;
    }

    return metrics;
}

void allocator_reset(AllocationType type) {
    if (!g_allocator_state.initialized) {
        return;
    }

    LinearAllocatorImpl* impl = &g_allocator_state.linear_allocators[type].impl;
    impl->used = 0;
    impl->allocation_count = 0;
    impl->free_count = 0;
    memset(impl->base, 0, impl->capacity);

    LOG_DEBUG("MEMORY", "Allocator %d reset", type);
}

void allocator_debug_print(void) {
    if (!g_allocator_state.initialized) {
        LOG_ERROR("MEMORY", "Allocator not initialized");
        return;
    }

    AllocationMetrics metrics = allocator_get_metrics();

    LOG_INFO("MEMORY", "=== ALLOCATION METRICS ===");
    LOG_INFO("MEMORY", "Total engine memory: %zu KB", metrics.total_engine_memory / 1024);
    LOG_INFO("MEMORY", "Active allocations: %u", metrics.active_allocations);

    const char* type_names[] = { "GENERAL", "POOL", "LINEAR", "TEMP", "PERSISTENT" };

    for (int i = 0; i < ALLOC_COUNT; i++) {
        AllocatorStats* stats = &metrics.stats[i];
        LOG_INFO("MEMORY", "  %s: %zu KB / %u allocations (peak: %zu KB)",
                 type_names[i],
                 stats->current_usage / 1024,
                 stats->allocation_count,
                 stats->peak_usage / 1024);
    }
}

void allocator_dump_stats(const char* filepath) {
    // Implementation would write to file
    LOG_INFO("MEMORY", "Dumping allocator stats to %s", filepath);
}

bool allocator_verify(void) {
    if (!g_allocator_state.initialized) {
        return false;
    }

    // Verify integrity of all allocators
    for (int i = 0; i < ALLOC_COUNT; i++) {
        LinearAllocatorImpl* impl = &g_allocator_state.linear_allocators[i].impl;
        if (!impl->base || impl->used > impl->capacity) {
            LOG_ERROR("MEMORY", "Allocator %d corruption detected", i);
            return false;
        }
    }

    return true;
}

/**
 * ============================================================================
 * LINEAR ALLOCATOR (PUBLIC API)
 * ============================================================================
 */

LinearAllocator* linear_allocator_create(size_t capacity) {
    LinearAllocator* alloc = malloc(sizeof(LinearAllocator));
    if (!alloc) return NULL;

    alloc->impl.base = malloc(capacity);
    if (!alloc->impl.base) {
        free(alloc);
        return NULL;
    }

    alloc->impl.capacity = capacity;
    alloc->impl.used = 0;
    alloc->impl.peak = 0;
    alloc->impl.allocation_count = 0;
    alloc->impl.total_allocated = 0;
    memset(alloc->impl.base, 0, capacity);

    return alloc;
}

void linear_allocator_destroy(LinearAllocator* alloc) {
    if (alloc) {
        if (alloc->impl.base) {
            free(alloc->impl.base);
        }
        free(alloc);
    }
}

void* linear_alloc(LinearAllocator* alloc, size_t size) {
    if (!alloc || size == 0) return NULL;

    if (alloc->impl.used + size > alloc->impl.capacity) {
        return NULL;
    }

    void* ptr = (uint8_t*)alloc->impl.base + alloc->impl.used;
    alloc->impl.used += size;
    alloc->impl.peak = alloc->impl.used > alloc->impl.peak ? alloc->impl.used : alloc->impl.peak;
    alloc->impl.allocation_count++;
    alloc->impl.total_allocated += size;

    memset(ptr, 0, size);
    return ptr;
}

void* linear_alloc_aligned(LinearAllocator* alloc, size_t size, size_t alignment) {
    if (!alloc || size == 0) return NULL;

    uintptr_t current = (uintptr_t)alloc->impl.base + alloc->impl.used;
    uintptr_t aligned = (current + alignment - 1) & ~(alignment - 1);
    size_t padding = aligned - current;

    if (alloc->impl.used + padding + size > alloc->impl.capacity) {
        return NULL;
    }

    alloc->impl.used += padding + size;
    alloc->impl.peak = alloc->impl.used > alloc->impl.peak ? alloc->impl.used : alloc->impl.peak;
    alloc->impl.allocation_count++;
    alloc->impl.total_allocated += size;

    void* ptr = (void*)aligned;
    memset(ptr, 0, size);
    return ptr;
}

void linear_allocator_reset(LinearAllocator* alloc) {
    if (alloc) {
        alloc->impl.used = 0;
        memset(alloc->impl.base, 0, alloc->impl.capacity);
    }
}

/**
 * ============================================================================
 * POOL ALLOCATOR (PUBLIC API)
 * ============================================================================
 */

PoolAllocator* pool_allocator_create(size_t object_size, uint32_t pool_size) {
    PoolAllocator* pool = malloc(sizeof(PoolAllocator));
    if (!pool) return NULL;

    pool->impl.object_size = object_size;
    pool->impl.pool_size = pool_size;
    pool->impl.available_count = pool_size;
    pool->impl.objects = malloc(object_size * pool_size);
    pool->impl.available = malloc(sizeof(bool) * pool_size);

    if (!pool->impl.objects || !pool->impl.available) {
        free(pool->impl.objects);
        free(pool->impl.available);
        free(pool);
        return NULL;
    }

    memset(pool->impl.available, 1, pool_size);
    return pool;
}

void pool_allocator_destroy(PoolAllocator* pool) {
    if (pool) {
        free(pool->impl.objects);
        free(pool->impl.available);
        free(pool);
    }
}

void* pool_alloc(PoolAllocator* pool) {
    if (!pool || pool->impl.available_count == 0) {
        return NULL;
    }

    for (uint32_t i = 0; i < pool->impl.pool_size; i++) {
        if (pool->impl.available[i]) {
            pool->impl.available[i] = false;
            pool->impl.available_count--;
            pool->impl.total_allocated += pool->impl.object_size;
            return (uint8_t*)pool->impl.objects + (i * pool->impl.object_size);
        }
    }

    return NULL;
}

void pool_free(PoolAllocator* pool, void* ptr) {
    if (!pool || !ptr) return;

    uintptr_t offset = (uintptr_t)ptr - (uintptr_t)pool->impl.objects;
    uint32_t index = offset / pool->impl.object_size;

    if (index < pool->impl.pool_size && !pool->impl.available[index]) {
        pool->impl.available[index] = true;
        pool->impl.available_count++;
        pool->impl.total_freed += pool->impl.object_size;
    }
}

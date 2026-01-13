// src/engine/core/memory/unified_allocator.c
//
// Purpose: Unified memory allocator implementation consolidating all allocation strategies
// Provides high-performance allocation with comprehensive tracking and debugging features

#include <core/memory/unified_allocator.h>
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================

static struct {
    bool initialized;
    Allocator* default_allocator;
    Allocator* temp_allocator;
    Allocator* persistent_allocator;
    Allocator* asset_allocator;
    MemoryProfiler profiler;
} g_allocator_system = {0};

// Global allocator instances
Allocator* g_default_allocator = NULL;
Allocator* g_temp_allocator = NULL;
Allocator* g_persistent_allocator = NULL;
Allocator* g_asset_allocator = NULL;

// Thread-local allocator
#ifdef _WIN32
__declspec(thread) Allocator* g_thread_local_allocator = NULL;
#else
__thread Allocator* g_thread_local_allocator = NULL;
#endif

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

size_t align_down(size_t size, size_t alignment) {
    return size & ~(alignment - 1);
}

bool is_aligned(size_t size, size_t alignment) {
    return (size & (alignment - 1)) == 0;
}

void* align_pointer(void* ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    return (void*)((addr + alignment - 1) & ~(alignment - 1));
}

// ============================================================================
// LINEAR ALLOCATOR IMPLEMENTATION
// ============================================================================

static void* linear_alloc(Allocator* allocator, size_t size, size_t alignment) {
    LinearAllocator* linear = (LinearAllocator*)allocator->impl;
    
    // Align size
    size = align_up(size, alignment);
    
    // Check if we have enough space in current buffer
    void* current_base = linear->buffers[linear->current_buffer];
    size_t current_offset = linear->offset;
    
    if (current_offset + size > linear->size) {
        // Try next buffer if available
        if (linear->buffer_count > 1) {
            linear->current_buffer = (linear->current_buffer + 1) % linear->buffer_count;
            linear->offset = 0;
            current_base = linear->buffers[linear->current_buffer];
            current_offset = 0;
        } else {
            // Out of memory
            return NULL;
        }
    }
    
    void* ptr = (u8*)current_base + current_offset;
    linear->offset += size;
    
    if (linear->offset > linear->peak_usage) {
        linear->peak_usage = linear->offset;
    }
    
    return ptr;
}

static void* linear_realloc(Allocator* allocator, void* ptr, size_t new_size) {
    // Linear allocator doesn't support reallocation
    return NULL;
}

static void linear_free(Allocator* allocator, void* ptr) {
    // Linear allocator doesn't support individual frees
}

static void linear_reset(Allocator* allocator) {
    LinearAllocator* linear = (LinearAllocator*)allocator->impl;
    linear->offset = 0;
    linear->current_buffer = 0;
}

static size_t linear_get_usage(Allocator* allocator) {
    LinearAllocator* linear = (LinearAllocator*)allocator->impl;
    return linear->offset;
}

static size_t linear_get_capacity(Allocator* allocator) {
    LinearAllocator* linear = (LinearAllocator*)allocator->impl;
    return linear->size * linear->buffer_count;
}

static void linear_get_stats(Allocator* allocator, char* buffer, size_t buffer_size) {
    LinearAllocator* linear = (LinearAllocator*)allocator->impl;
    snprintf(buffer, buffer_size,
             "Linear Allocator '%s': %zu/%zu bytes used (%.1f%%), peak: %zu bytes",
             allocator->name, linear->offset, linear->size,
             (f32)linear->offset / linear->size * 100.0f, linear->peak_usage);
}

static void linear_destroy(Allocator* allocator) {
    LinearAllocator* linear = (LinearAllocator*)allocator->impl;
    if (linear) {
        if (linear->buffers) {
            for (u32 i = 0; i < linear->buffer_count; i++) {
                if (linear->buffers[i]) {
                    free(linear->buffers[i]);
                }
            }
            free(linear->buffers);
        }
        free(linear);
    }
    free(allocator);
}

static const AllocatorVTable linear_vtable = {
    .alloc = linear_alloc,
    .realloc = linear_realloc,
    .free = linear_free,
    .reset = linear_reset,
    .get_usage = linear_get_usage,
    .get_capacity = linear_get_capacity,
    .get_stats = linear_get_stats,
    .destroy = linear_destroy
};

// ============================================================================
// STACK ALLOCATOR IMPLEMENTATION
// ============================================================================

typedef struct StackMarker {
    size_t offset;
    u32 magic; // For validation
} StackMarker;

#define STACK_MARKER_MAGIC 0xDEADBEEF

static void* stack_alloc(Allocator* allocator, size_t size, size_t alignment) {
    StackAllocator* stack = (StackAllocator*)allocator->impl;
    
    // Align size
    size = align_up(size, alignment);
    
    // Check if we have enough space
    if (stack->offset + size > stack->size) {
        return NULL;
    }
    
    void* ptr = (u8*)stack->base + stack->offset;
    stack->offset += size;
    
    if (stack->offset > stack->peak_usage) {
        stack->peak_usage = stack->offset;
    }
    
    return ptr;
}

static void* stack_realloc(Allocator* allocator, void* ptr, size_t new_size) {
    // Stack allocator doesn't support reallocation
    return NULL;
}

static void stack_free(Allocator* allocator, void* ptr) {
    // Stack allocator uses markers, not individual frees
}

static void stack_reset(Allocator* allocator) {
    StackAllocator* stack = (StackAllocator*)allocator->impl;
    stack->offset = 0;
    stack->stack_top = 0;
}

static size_t stack_get_usage(Allocator* allocator) {
    StackAllocator* stack = (StackAllocator*)allocator->impl;
    return stack->offset;
}

static size_t stack_get_capacity(Allocator* allocator) {
    StackAllocator* stack = (StackAllocator*)allocator->impl;
    return stack->size;
}

static void stack_get_stats(Allocator* allocator, char* buffer, size_t buffer_size) {
    StackAllocator* stack = (StackAllocator*)allocator->impl;
    snprintf(buffer, buffer_size,
             "Stack Allocator '%s': %zu/%zu bytes used (%.1f%%), peak: %zu bytes, markers: %u/%u",
             allocator->name, stack->offset, stack->size,
             (f32)stack->offset / stack->size * 100.0f, stack->peak_usage,
             stack->stack_top, stack->stack_capacity);
}

static void stack_destroy(Allocator* allocator) {
    StackAllocator* stack = (StackAllocator*)allocator->impl;
    if (stack) {
        if (stack->base) free(stack->base);
        if (stack->stack) free(stack->stack);
        free(stack);
    }
    free(allocator);
}

static const AllocatorVTable stack_vtable = {
    .alloc = stack_alloc,
    .realloc = stack_realloc,
    .free = stack_free,
    .reset = stack_reset,
    .get_usage = stack_get_usage,
    .get_capacity = stack_get_capacity,
    .get_stats = stack_get_stats,
    .destroy = stack_destroy
};

// ============================================================================
// POOL ALLOCATOR IMPLEMENTATION
// ============================================================================

static void* pool_alloc(Allocator* allocator, size_t size, size_t alignment) {
    PoolAllocator* pool = (PoolAllocator*)allocator->impl;
    
    if (size > pool->block_size) {
        return NULL; // Too large for this pool
    }
    
    if (pool->free_count == 0) {
        return NULL; // Pool exhausted
    }
    
    pool->free_count--;
    pool->allocated_count++;
    
    if (pool->allocated_count > pool->peak_allocated) {
        pool->peak_allocated = pool->allocated_count;
    }
    
    return pool->free_list[pool->free_count];
}

static void* pool_realloc(Allocator* allocator, void* ptr, size_t new_size) {
    // Pool allocator doesn't support reallocation
    return NULL;
}

static void pool_free(Allocator* allocator, void* ptr) {
    PoolAllocator* pool = (PoolAllocator*)allocator->impl;
    
    if (pool->free_count >= pool->block_count) {
        return; // Pool already full
    }
    
    pool->free_list[pool->free_count] = ptr;
    pool->free_count++;
    pool->allocated_count--;
}

static void pool_reset(Allocator* allocator) {
    PoolAllocator* pool = (PoolAllocator*)allocator->impl;
    
    // Rebuild free list
    for (size_t i = 0; i < pool->block_count; i++) {
        pool->free_list[i] = (u8*)pool->base + (i * pool->block_size);
    }
    pool->free_count = pool->block_count;
    pool->allocated_count = 0;
}

static size_t pool_get_usage(Allocator* allocator) {
    PoolAllocator* pool = (PoolAllocator*)allocator->impl;
    return pool->allocated_count * pool->block_size;
}

static size_t pool_get_capacity(Allocator* allocator) {
    PoolAllocator* pool = (PoolAllocator*)allocator->impl;
    return pool->block_count * pool->block_size;
}

static void pool_get_stats(Allocator* allocator, char* buffer, size_t buffer_size) {
    PoolAllocator* pool = (PoolAllocator*)allocator->impl;
    snprintf(buffer, buffer_size,
             "Pool Allocator '%s': %zu/%zu blocks used (%.1f%%), block size: %zu bytes, peak: %zu blocks",
             allocator->name, pool->allocated_count, pool->block_count,
             (f32)pool->allocated_count / pool->block_count * 100.0f,
             pool->block_size, pool->peak_allocated);
}

static void pool_destroy(Allocator* allocator) {
    PoolAllocator* pool = (PoolAllocator*)allocator->impl;
    if (pool) {
        if (pool->base) free(pool->base);
        if (pool->free_list) free(pool->free_list);
        free(pool);
    }
    free(allocator);
}

static const AllocatorVTable pool_vtable = {
    .alloc = pool_alloc,
    .realloc = pool_realloc,
    .free = pool_free,
    .reset = pool_reset,
    .get_usage = pool_get_usage,
    .get_capacity = pool_get_capacity,
    .get_stats = pool_get_stats,
    .destroy = pool_destroy
};

// ============================================================================
// TRACKED ALLOCATOR IMPLEMENTATION
// ============================================================================

static void* tracked_alloc(Allocator* allocator, size_t size, size_t alignment) {
    TrackedAllocator* tracked = (TrackedAllocator*)allocator->impl;
    
    // Check memory limits
    if (tracked->enforce_limits && 
        tracked->total_allocated + size > tracked->memory_limit) {
        return NULL;
    }
    
    // Allocate from backing allocator
    size_t total_size = size + sizeof(AllocationHeader) + alignment - 1;
    void* raw_ptr = allocator_alloc(tracked->backing_allocator, total_size);
    if (!raw_ptr) {
        return NULL;
    }
    
    // Align the user pointer
    void* user_ptr = align_pointer((u8*)raw_ptr + sizeof(AllocationHeader), alignment);
    AllocationHeader* header = (AllocationHeader*)((u8*)user_ptr - sizeof(AllocationHeader));
    
    // Fill header
    header->size = size;
    header->file = NULL;
    header->line = 0;
    header->tag = allocator->tag;
    header->allocation_id = ++tracked->allocation_counter;
    header->timestamp = 0; // Would need timing system
    header->next = tracked->allocations;
    header->prev = NULL;
    header->stack_depth = 0;
    
    // Add to allocation list
    if (tracked->allocations) {
        tracked->allocations->prev = header;
    }
    tracked->allocations = header;
    
    // Update statistics
    tracked->total_allocated += size;
    tracked->allocation_count++;
    tracked->tag_usage[allocator->tag] += size;
    
    if (tracked->total_allocated > tracked->peak_allocated) {
        tracked->peak_allocated = tracked->total_allocated;
    }
    
    return user_ptr;
}

static void* tracked_realloc(Allocator* allocator, void* ptr, size_t new_size) {
    if (!ptr) {
        return tracked_alloc(allocator, new_size, 16);
    }
    
    if (new_size == 0) {
        tracked_free(allocator, ptr);
        return NULL;
    }
    
    TrackedAllocator* tracked = (TrackedAllocator*)allocator->impl;
    AllocationHeader* header = (AllocationHeader*)((u8*)ptr - sizeof(AllocationHeader));
    size_t old_size = header->size;
    
    // Allocate new block
    void* new_ptr = tracked_alloc(allocator, new_size, 16);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy data
    size_t copy_size = old_size < new_size ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    
    // Free old block
    tracked_free(allocator, ptr);
    
    return new_ptr;
}

static void tracked_free(Allocator* allocator, void* ptr) {
    if (!ptr) return;
    
    TrackedAllocator* tracked = (TrackedAllocator*)allocator->impl;
    AllocationHeader* header = (AllocationHeader*)((u8*)ptr - sizeof(AllocationHeader));
    
    // Remove from allocation list
    if (header->prev) {
        header->prev->next = header->next;
    } else {
        tracked->allocations = header->next;
    }
    if (header->next) {
        header->next->prev = header->prev;
    }
    
    // Update statistics
    tracked->total_allocated -= header->size;
    tracked->allocation_count--;
    tracked->tag_usage[header->tag] -= header->size;
    
    // Free from backing allocator
    allocator_free(tracked->backing_allocator, header);
}

static void tracked_reset(Allocator* allocator) {
    TrackedAllocator* tracked = (TrackedAllocator*)allocator->impl;
    
    // Free all allocations
    AllocationHeader* header = tracked->allocations;
    while (header) {
        AllocationHeader* next = header->next;
        allocator_free(tracked->backing_allocator, header);
        header = next;
    }
    
    // Reset statistics
    tracked->allocations = NULL;
    tracked->total_allocated = 0;
    tracked->allocation_count = 0;
    memset(tracked->tag_usage, 0, sizeof(tracked->tag_usage));
}

static size_t tracked_get_usage(Allocator* allocator) {
    TrackedAllocator* tracked = (TrackedAllocator*)allocator->impl;
    return tracked->total_allocated;
}

static size_t tracked_get_capacity(Allocator* allocator) {
    TrackedAllocator* tracked = (TrackedAllocator*)allocator->impl;
    return allocator_get_capacity(tracked->backing_allocator);
}

static void tracked_get_stats(Allocator* allocator, char* buffer, size_t buffer_size) {
    TrackedAllocator* tracked = (TrackedAllocator*)allocator->impl;
    snprintf(buffer, buffer_size,
             "Tracked Allocator '%s': %zu bytes allocated, %zu peak, %llu allocations, limit: %zu",
             allocator->name, tracked->total_allocated, tracked->peak_allocated,
             tracked->allocation_count, tracked->memory_limit);
}

static void tracked_destroy(Allocator* allocator) {
    TrackedAllocator* tracked = (TrackedAllocator*)allocator->impl;
    
    // Check for leaks
    if (tracked->allocation_count > 0) {
        printf("WARNING: Memory leak detected in allocator '%s': %llu allocations not freed\n",
               allocator->name, tracked->allocation_count);
        
        AllocationHeader* header = tracked->allocations;
        while (header) {
            printf("  Leaked allocation: %zu bytes (ID: %llu)\n", 
                   header->size, header->allocation_id);
            header = header->next;
        }
    }
    
    // Free backing allocator
    if (tracked->backing_allocator && tracked->backing_allocator != allocator) {
        allocator_destroy(tracked->backing_allocator);
    }
    
    free(tracked);
    free(allocator);
}

static const AllocatorVTable tracked_vtable = {
    .alloc = tracked_alloc,
    .realloc = tracked_realloc,
    .free = tracked_free,
    .reset = tracked_reset,
    .get_usage = tracked_get_usage,
    .get_capacity = tracked_get_capacity,
    .get_stats = tracked_get_stats,
    .destroy = tracked_destroy
};

// ============================================================================
// PUBLIC CREATION FUNCTIONS
// ============================================================================

Allocator* allocator_create_linear(size_t size, const char* name, MemoryTag tag) {
    Allocator* allocator = malloc(sizeof(Allocator));
    LinearAllocator* linear = malloc(sizeof(LinearAllocator));
    
    if (!allocator || !linear) {
        free(allocator);
        free(linear);
        return NULL;
    }
    
    // Create single buffer by default
    linear->size = size;
    linear->offset = 0;
    linear->peak_usage = 0;
    linear->buffer_count = 1;
    linear->current_buffer = 0;
    linear->buffers = malloc(sizeof(void*));
    linear->buffers[0] = malloc(size);
    
    if (!linear->buffers[0]) {
        free(linear->buffers);
        free(linear);
        free(allocator);
        return NULL;
    }
    
    allocator->vtable = &linear_vtable;
    allocator->name = name ? name : "Linear";
    allocator->tag = tag;
    allocator->strategy = ALLOCATOR_STRATEGY_LINEAR;
    allocator->fallback = NULL;
    allocator->impl = linear;
    allocator->owns_fallback = false;
    
    return allocator;
}

Allocator* allocator_create_stack(size_t size, const char* name, MemoryTag tag) {
    Allocator* allocator = malloc(sizeof(Allocator));
    StackAllocator* stack = malloc(sizeof(StackAllocator));
    
    if (!allocator || !stack) {
        free(allocator);
        free(stack);
        return NULL;
    }
    
    stack->base = malloc(size);
    if (!stack->base) {
        free(stack);
        free(allocator);
        return NULL;
    }
    
    stack->size = size;
    stack->offset = 0;
    stack->peak_usage = 0;
    stack->stack_capacity = 64; // Default marker capacity
    stack->stack_top = 0;
    stack->stack = malloc(sizeof(StackMarker) * stack->stack_capacity);
    
    allocator->vtable = &stack_vtable;
    allocator->name = name ? name : "Stack";
    allocator->tag = tag;
    allocator->strategy = ALLOCATOR_STRATEGY_STACK;
    allocator->fallback = NULL;
    allocator->impl = stack;
    allocator->owns_fallback = false;
    
    return allocator;
}

Allocator* allocator_create_pool(size_t block_size, size_t block_count, const char* name, MemoryTag tag) {
    Allocator* allocator = malloc(sizeof(Allocator));
    PoolAllocator* pool = malloc(sizeof(PoolAllocator));
    
    if (!allocator || !pool) {
        free(allocator);
        free(pool);
        return NULL;
    }
    
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->allocated_count = 0;
    pool->peak_allocated = 0;
    
    pool->base = malloc(block_size * block_count);
    pool->free_list = malloc(sizeof(void*) * block_count);
    
    if (!pool->base || !pool->free_list) {
        free(pool->base);
        free(pool->free_list);
        free(pool);
        free(allocator);
        return NULL;
    }
    
    // Initialize free list
    for (size_t i = 0; i < block_count; i++) {
        pool->free_list[i] = (u8*)pool->base + (i * block_size);
    }
    pool->free_count = block_count;
    
    allocator->vtable = &pool_vtable;
    allocator->name = name ? name : "Pool";
    allocator->tag = tag;
    allocator->strategy = ALLOCATOR_STRATEGY_POOL;
    allocator->fallback = NULL;
    allocator->impl = pool;
    allocator->owns_fallback = false;
    
    return allocator;
}

Allocator* allocator_create_tracked(Allocator* backing, const char* name, MemoryTag tag) {
    Allocator* allocator = malloc(sizeof(Allocator));
    TrackedAllocator* tracked = malloc(sizeof(TrackedAllocator));
    
    if (!allocator || !tracked) {
        free(allocator);
        free(tracked);
        return NULL;
    }
    
    tracked->backing_allocator = backing ? backing : allocator_create_linear(64*1024*1024, "TrackedBacking", tag);
    tracked->allocations = NULL;
    tracked->total_allocated = 0;
    tracked->peak_allocated = 0;
    tracked->allocation_count = 0;
    tracked->allocation_counter = 0;
    tracked->stack_trace_enabled = false;
    tracked->advanced_leak_detection = false;
    tracked->memory_limit = SIZE_MAX;
    tracked->enforce_limits = false;
    memset(tracked->tag_usage, 0, sizeof(tracked->tag_usage));
    memset(tracked->tag_limits, 0, sizeof(tracked->tag_limits));
    
    allocator->vtable = &tracked_vtable;
    allocator->name = name ? name : "Tracked";
    allocator->tag = tag;
    allocator->strategy = ALLOCATOR_STRATEGY_TRACKED;
    allocator->fallback = NULL;
    allocator->impl = tracked;
    allocator->owns_fallback = (backing == NULL);
    
    return allocator;
}

// ============================================================================
// CORE ALLOCATION INTERFACE
// ============================================================================

void* allocator_alloc(Allocator* allocator, size_t size) {
    return allocator_alloc_aligned(allocator, size, 16);
}

void* allocator_alloc_aligned(Allocator* allocator, size_t size, size_t alignment) {
    if (!allocator || !allocator->vtable) {
        return NULL;
    }
    
    void* ptr = allocator->vtable->alloc(allocator, size, alignment);
    
    // Try fallback if allocation failed
    if (!ptr && allocator->fallback) {
        ptr = allocator->vtable->alloc(allocator->fallback, size, alignment);
    }
    
    return ptr;
}

void* allocator_realloc(Allocator* allocator, void* ptr, size_t new_size) {
    if (!allocator || !allocator->vtable) {
        return NULL;
    }
    
    return allocator->vtable->realloc(allocator, ptr, new_size);
}

void allocator_free(Allocator* allocator, void* ptr) {
    if (!allocator || !allocator->vtable || !ptr) {
        return;
    }
    
    allocator->vtable->free(allocator, ptr);
}

void allocator_reset(Allocator* allocator) {
    if (!allocator || !allocator->vtable) {
        return;
    }
    
    allocator->vtable->reset(allocator);
}

void allocator_destroy(Allocator* allocator) {
    if (!allocator || !allocator->vtable) {
        return;
    }
    
    allocator->vtable->destroy(allocator);
}

// ============================================================================
// STATISTICS AND MONITORING
// ============================================================================

void allocator_get_stats(Allocator* allocator, AllocatorStats* stats) {
    if (!allocator || !stats) return;
    
    memset(stats, 0, sizeof(AllocatorStats));
    stats->total_allocated = allocator_get_usage(allocator);
    stats->capacity = allocator_get_capacity(allocator);
    
    if (allocator->vtable && allocator->vtable->get_stats) {
        char buffer[512];
        allocator->vtable->get_stats(allocator, buffer, sizeof(buffer));
        // Parse additional stats from buffer if needed
    }
}

void allocator_print_stats(Allocator* allocator) {
    if (!allocator) return;
    
    AllocatorStats stats;
    allocator_get_stats(allocator, &stats);
    
    printf("Allocator '%s': %zu/%zu bytes used (%.1f%%)\n",
           allocator->name, stats.total_allocated, stats.capacity,
           stats.capacity > 0 ? (f32)stats.total_allocated / stats.capacity * 100.0f : 0.0f);
}

size_t allocator_get_usage(Allocator* allocator) {
    if (!allocator || !allocator->vtable || !allocator->vtable->get_usage) {
        return 0;
    }
    
    return allocator->vtable->get_usage(allocator);
}

size_t allocator_get_capacity(Allocator* allocator) {
    if (!allocator || !allocator->vtable || !allocator->vtable->get_capacity) {
        return 0;
    }
    
    return allocator->vtable->get_capacity(allocator);
}

// ============================================================================
// GLOBAL ALLOCATOR SYSTEM
// ============================================================================

bool allocator_system_init(void) {
    if (g_allocator_system.initialized) {
        return true;
    }
    
    // Create default allocators
    g_default_allocator = allocator_create_tracked(NULL, "Default", MEMORY_TAG_DEFAULT);
    g_temp_allocator = allocator_create_linear(32*1024*1024, "Temp", MEMORY_TAG_TEMP);
    g_persistent_allocator = allocator_create_tracked(NULL, "Persistent", MEMORY_TAG_PERSISTENT);
    g_asset_allocator = allocator_create_tracked(NULL, "Asset", MEMORY_TAG_ASSET);
    
    if (!g_default_allocator || !g_temp_allocator || !g_persistent_allocator || !g_asset_allocator) {
        printf("ERROR: Failed to create global allocators\n");
        return false;
    }
    
    // Set global pointers
    g_allocator_system.default_allocator = g_default_allocator;
    g_allocator_system.temp_allocator = g_temp_allocator;
    g_allocator_system.persistent_allocator = g_persistent_allocator;
    g_allocator_system.asset_allocator = g_asset_allocator;
    
    g_allocator_system.initialized = true;
    
    printf("✓ Unified allocator system initialized\n");
    return true;
}

void allocator_system_shutdown(void) {
    if (!g_allocator_system.initialized) {
        return;
    }
    
    printf("Shutting down unified allocator system...\n");
    
    if (g_default_allocator) {
        allocator_destroy(g_default_allocator);
        g_default_allocator = NULL;
    }
    
    if (g_temp_allocator) {
        allocator_destroy(g_temp_allocator);
        g_temp_allocator = NULL;
    }
    
    if (g_persistent_allocator) {
        allocator_destroy(g_persistent_allocator);
        g_persistent_allocator = NULL;
    }
    
    if (g_asset_allocator) {
        allocator_destroy(g_asset_allocator);
        g_asset_allocator = NULL;
    }
    
    memset(&g_allocator_system, 0, sizeof(g_allocator_system));
    
    printf("✓ Unified allocator system shutdown complete\n");
}

void allocator_set_default(Allocator* allocator) {
    g_default_allocator = allocator;
    g_allocator_system.default_allocator = allocator;
}

void allocator_set_temp(Allocator* allocator) {
    g_temp_allocator = allocator;
    g_allocator_system.temp_allocator = allocator;
}

void allocator_set_persistent(Allocator* allocator) {
    g_persistent_allocator = allocator;
    g_allocator_system.persistent_allocator = allocator;
}

void allocator_set_asset(Allocator* allocator) {
    g_asset_allocator = allocator;
    g_allocator_system.asset_allocator = allocator;
}

Allocator* allocator_get_thread_local(void) {
    return g_thread_local_allocator;
}

void allocator_set_thread_local(Allocator* allocator) {
    g_thread_local_allocator = allocator;
}

// ============================================================================
// SIMD MEMORY OPERATIONS
// ============================================================================

bool simd_is_supported(void) {
#ifdef __AVX2__
    return true;
#elif defined(__SSE2__)
    return true;
#else
    return false;
#endif
}

void* simd_memcpy(void* dst, const void* src, size_t size) {
#ifdef __AVX2__
    // AVX2 implementation would go here
    return memcpy(dst, src, size);
#elif defined(__SSE2__)
    // SSE2 implementation would go here
    return memcpy(dst, src, size);
#else
    return memcpy(dst, src, size);
#endif
}

void* simd_memset(void* dst, u8 value, size_t size) {
#ifdef __AVX2__
    // AVX2 implementation would go here
    return memset(dst, value, size);
#elif defined(__SSE2__)
    // SSE2 implementation would go here
    return memset(dst, value, size);
#else
    return memset(dst, value, size);
#endif
}

int simd_memcmp(const void* ptr1, const void* ptr2, size_t size) {
    return memcmp(ptr1, ptr2, size);
}

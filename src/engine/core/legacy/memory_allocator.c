// src/engine/core/memory_allocator.c
//
// Purpose: Unified memory allocator for the engine core
// Provides allocation strategies, fragmentation prevention, and tracking

#include <core/memory_allocator.h>
#include <core/logging_system.h>
#include <stdlib.h>
#include <string.h>

// Memory pool for small allocations
#define MEMORY_POOL_SIZE (64 * 1024 * 1024) // 64MB
#define SMALL_ALLOC_MAX 1024
#define POOL_ALIGNMENT 16

typedef struct MemoryPool {
    u8* memory;
    size_t size;
    size_t used;
    size_t peak_used;
    struct MemoryPool* next;
} MemoryPool;

typedef struct AllocationHeader {
    size_t size;
    const char* file;
    int line;
    struct AllocationHeader* next;
    struct AllocationHeader* prev;
} AllocationHeader;

static struct {
    MemoryPool* pools;
    AllocationHeader* allocations;
    size_t total_allocated;
    size_t peak_allocated;
    u64 allocation_count;
    bool initialized;
} g_allocator_state = {0};

// Forward declarations
static MemoryPool* create_memory_pool(size_t size);
static void destroy_memory_pool(MemoryPool* pool);
static void* allocate_from_pool(size_t size);
static void track_allocation(void* ptr, size_t size, const char* file, int line);
static void untrack_allocation(void* ptr);

bool memory_allocator_init(void) {
    if (g_allocator_state.initialized) {
        printf("WARN: Memory allocator already initialized\n");
        return true;
    }

    printf("Initializing unified memory allocator...\n");
    
    // Create initial memory pool
    g_allocator_state.pools = create_memory_pool(MEMORY_POOL_SIZE);
    if (!g_allocator_state.pools) {
        printf("ERROR: Failed to create initial memory pool\n");
        return false;
    }

    g_allocator_state.total_allocated = 0;
    g_allocator_state.peak_allocated = 0;
    g_allocator_state.allocation_count = 0;
    g_allocator_state.initialized = true;

    printf("✓ Memory allocator initialized with %zu MB pool\n", MEMORY_POOL_SIZE / (1024 * 1024));
    return true;
}

void memory_allocator_shutdown(void) {
    if (!g_allocator_state.initialized) {
        return;
    }

    printf("Shutting down memory allocator...\n");
    
    // Check for memory leaks
    if (g_allocator_state.allocation_count > 0) {
        printf("ERROR: Memory leak detected: %llu allocations not freed\n", g_allocator_state.allocation_count);
        
        // Print leaked allocations
        AllocationHeader* header = g_allocator_state.allocations;
        while (header) {
            printf("ERROR: Leaked allocation: %zu bytes at %p (%s:%d)\n", 
                     header->size, (void*)header + sizeof(AllocationHeader),
                     header->file ? header->file : "unknown", header->line);
            header = header->next;
        }
    }

    // Destroy all memory pools
    MemoryPool* pool = g_allocator_state.pools;
    while (pool) {
        MemoryPool* next = pool->next;
        destroy_memory_pool(pool);
        pool = next;
    }

    printf("Memory allocator shutdown complete. Peak usage: %zu MB\n", 
             g_allocator_state.peak_allocated / (1024 * 1024));
    
    memset(&g_allocator_state, 0, sizeof(g_allocator_state));
}

void* memory_allocate(size_t size, const char* file, int line) {
    if (!g_allocator_state.initialized) {
        printf("ERROR: Memory allocator not initialized\n");
        return malloc(size);
    }

    if (size == 0) {
        return NULL;
    }

    void* ptr;
    
    // Use pool for small allocations
    if (size <= SMALL_ALLOC_MAX) {
        ptr = allocate_from_pool(size);
    } else {
        // Use malloc for large allocations
        ptr = malloc(size + sizeof(AllocationHeader));
        if (ptr) {
            ptr = (u8*)ptr + sizeof(AllocationHeader);
        }
    }

    if (ptr) {
        track_allocation(ptr, size, file, line);
        g_allocator_state.total_allocated += size;
        g_allocator_state.allocation_count++;
        
        if (g_allocator_state.total_allocated > g_allocator_state.peak_allocated) {
            g_allocator_state.peak_allocated = g_allocator_state.total_allocated;
        }
    } else {
        printf("ERROR: Failed to allocate %zu bytes\n", size);
    }

    return ptr;
}

void* memory_reallocate(void* ptr, size_t new_size, const char* file, int line) {
    if (!ptr) {
        return memory_allocate(new_size, file, line);
    }

    if (new_size == 0) {
        memory_deallocate(ptr);
        return NULL;
    }

    // Find allocation header
    AllocationHeader* header = (AllocationHeader*)((u8*)ptr - sizeof(AllocationHeader));
    size_t old_size = header->size;

    void* new_ptr = memory_allocate(new_size, file, line);
    if (!new_ptr) {
        return NULL;
    }

    // Copy old data
    size_t copy_size = old_size < new_size ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);

    memory_deallocate(ptr);
    return new_ptr;
}

void memory_deallocate(void* ptr) {
    if (!ptr) {
        return;
    }

    if (!g_allocator_state.initialized) {
        free(ptr);
        return;
    }

    // Find allocation header
    AllocationHeader* header = (AllocationHeader*)((u8*)ptr - sizeof(AllocationHeader));
    
    // Untrack allocation
    untrack_allocation(ptr);
    
    g_allocator_state.total_allocated -= header->size;
    g_allocator_state.allocation_count--;

    // Check if this was a pool allocation
    if (header->size <= SMALL_ALLOC_MAX) {
        // Pool allocations are freed when pool is destroyed
        return;
    }

    // Free large allocation
    free(header);
}

void memory_allocator_get_stats(size_t* total_allocated, size_t* peak_allocated, u64* allocation_count) {
    if (total_allocated) *total_allocated = g_allocator_state.total_allocated;
    if (peak_allocated) *peak_allocated = g_allocator_state.peak_allocated;
    if (allocation_count) *allocation_count = g_allocator_state.allocation_count;
}

// Static helper functions
static MemoryPool* create_memory_pool(size_t size) {
    MemoryPool* pool = (MemoryPool*)malloc(sizeof(MemoryPool));
    if (!pool) {
        return NULL;
    }

    pool->memory = (u8*)malloc(size);
    if (!pool->memory) {
        free(pool);
        return NULL;
    }

    pool->size = size;
    pool->used = 0;
    pool->peak_used = 0;
    pool->next = NULL;

    return pool;
}

static void destroy_memory_pool(MemoryPool* pool) {
    if (pool) {
        if (pool->memory) {
            free(pool->memory);
        }
        free(pool);
    }
}

static void* allocate_from_pool(size_t size) {
    // Align size to POOL_ALIGNMENT
    size = (size + POOL_ALIGNMENT - 1) & ~(POOL_ALIGNMENT - 1);

    // Find a pool with enough space
    MemoryPool* pool = g_allocator_state.pools;
    while (pool) {
        if (pool->used + size <= pool->size) {
            void* ptr = pool->memory + pool->used;
            pool->used += size;
            
            if (pool->used > pool->peak_used) {
                pool->peak_used = pool->used;
            }
            
            return ptr;
        }
        pool = pool->next;
    }

    // Create new pool if needed
    MemoryPool* new_pool = create_memory_pool(MEMORY_POOL_SIZE);
    if (!new_pool) {
        return NULL;
    }

    // Add to front of pool list
    new_pool->next = g_allocator_state.pools;
    g_allocator_state.pools = new_pool;

    void* ptr = new_pool->memory + size;
    new_pool->used = size;
    new_pool->peak_used = size;

    return ptr;
}

static void track_allocation(void* ptr, size_t size, const char* file, int line) {
    AllocationHeader* header = (AllocationHeader*)((u8*)ptr - sizeof(AllocationHeader));
    header->size = size;
    header->file = file;
    header->line = line;
    
    // Add to allocation list
    header->next = g_allocator_state.allocations;
    header->prev = NULL;
    
    if (g_allocator_state.allocations) {
        g_allocator_state.allocations->prev = header;
    }
    
    g_allocator_state.allocations = header;
}

static void untrack_allocation(void* ptr) {
    AllocationHeader* header = (AllocationHeader*)((u8*)ptr - sizeof(AllocationHeader));
    
    // Remove from allocation list
    if (header->prev) {
        header->prev->next = header->next;
    } else {
        g_allocator_state.allocations = header->next;
    }
    
    if (header->next) {
        header->next->prev = header->prev;
    }
}

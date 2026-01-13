#include "memory/unified_memory_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

// ============================================================================
// UNIFIED MEMORY ALLOCATOR IMPLEMENTATION
// ============================================================================

// Global allocator instance
static UnifiedMemoryAllocator g_allocator = {0};

// Canary values for corruption detection
#define CANARY_VALUE 0xDEADBEEFCAFEBABEULL
#define GUARD_PAGE_SIZE 4096

// Internal helper functions
static void* allocate_with_guard_pages(size_t size);
static void free_with_guard_pages(void* ptr, size_t size);
static void setup_canary(void* ptr, size_t size);
static bool check_canary(void* ptr, size_t size);
static void capture_stack_trace(void** buffer, int* frames);
static void add_allocation(AllocationMetadata* metadata);
static void remove_allocation(AllocationMetadata* metadata);
static AllocationMetadata* find_allocation(void* ptr);
static void update_stats(size_t size, bool allocated);
static void* align_memory(void* ptr, size_t alignment);
static size_t calculate_aligned_size(size_t size, size_t alignment);

// ============================================================================
// CORE ALLOCATOR FUNCTIONS
// ============================================================================

bool unified_memory_init(const MemoryPolicy* policy) {
    if (g_allocator.initialized) {
        return true; // Already initialized
    }

    // Initialize mutexes
    if (pthread_mutex_init(&g_allocator.global_mutex, NULL) != 0) return false;
    if (pthread_mutex_init(&g_allocator.allocations_mutex, NULL) != 0) return false;
    if (pthread_mutex_init(&g_allocator.pools_mutex, NULL) != 0) return false;
    if (pthread_mutex_init(&g_allocator.stacks_mutex, NULL) != 0) return false;
    if (pthread_mutex_init(&g_allocator.arenas_mutex, NULL) != 0) return false;
    if (pthread_mutex_init(&g_allocator.stats_mutex, NULL) != 0) return false;

    // Set policy
    if (policy) {
        g_allocator.policy = *policy;
    } else {
        // Default policy
        g_allocator.policy = (MemoryPolicy){
            .global_limit = SIZE_MAX,
            .per_pool_limit = 64 * 1024 * 1024, // 64MB
            .per_stack_limit = 16 * 1024 * 1024, // 16MB
            .per_arena_limit = 128 * 1024 * 1024, // 128MB
            .max_allocations = 1000000,
            .enable_guard_pages = false,
            .enable_canaries = true,
            .enable_stack_traces = false,
            .stack_trace_depth = 8,
            .enable_leak_detection = true,
            .enable_fragmentation_check = true
        };
    }

    // Initialize statistics
    memset(&g_allocator.stats, 0, sizeof(MemoryStats));
    g_allocator.next_allocation_id = 1;

    g_allocator.initialized = true;
    return true;
}

void unified_memory_shutdown(void) {
    if (!g_allocator.initialized) return;

    // Check for leaks if enabled
    if (g_allocator.policy.enable_leak_detection) {
        unified_memory_check_leaks();
    }

    // Destroy all pools
    pthread_mutex_lock(&g_allocator.pools_mutex);
    MemoryPool* pool = g_allocator.pools;
    while (pool) {
        MemoryPool* next = pool->next;
        unified_memory_pool_destroy(pool);
        pool = next;
    }
    pthread_mutex_unlock(&g_allocator.pools_mutex);

    // Destroy all stacks
    pthread_mutex_lock(&g_allocator.stacks_mutex);
    StackAllocator* stack = g_allocator.stacks;
    while (stack) {
        StackAllocator* next = stack->next;
        unified_memory_stack_destroy(stack);
        stack = next;
    }
    pthread_mutex_unlock(&g_allocator.stacks_mutex);

    // Destroy all arenas
    pthread_mutex_lock(&g_allocator.arenas_mutex);
    ArenaAllocator* arena = g_allocator.arenas;
    while (arena) {
        ArenaAllocator* next = arena->next;
        unified_memory_arena_destroy(arena);
        arena = next;
    }
    pthread_mutex_unlock(&g_allocator.arenas_mutex);

    // Clean up remaining allocations
    pthread_mutex_lock(&g_allocator.allocations_mutex);
    AllocationMetadata* alloc = g_allocator.allocations;
    while (alloc) {
        AllocationMetadata* next = alloc->next;
        free(alloc);
        alloc = next;
    }
    pthread_mutex_unlock(&g_allocator.allocations_mutex);

    // Destroy mutexes
    pthread_mutex_destroy(&g_allocator.global_mutex);
    pthread_mutex_destroy(&g_allocator.allocations_mutex);
    pthread_mutex_destroy(&g_allocator.pools_mutex);
    pthread_mutex_destroy(&g_allocator.stacks_mutex);
    pthread_mutex_destroy(&g_allocator.arenas_mutex);
    pthread_mutex_destroy(&g_allocator.stats_mutex);

    g_allocator.initialized = false;
}

void* unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags, 
                        const char* file, int line, const char* function) {
    if (!g_allocator.initialized) return malloc(size); // Fallback to malloc

    if (size == 0) return NULL;

    pthread_mutex_lock(&g_allocator.global_mutex);

    // Check global limit
    if (g_allocator.policy.global_limit != SIZE_MAX && 
        g_allocator.stats.total_allocated + size > g_allocator.policy.global_limit) {
        g_allocator.stats.allocation_failures++;
        pthread_mutex_unlock(&g_allocator.global_mutex);
        return NULL;
    }

    // Check max allocations
    if (g_allocator.policy.max_allocations > 0 && 
        g_allocator.stats.current_allocations >= g_allocator.policy.max_allocations) {
        g_allocator.stats.allocation_failures++;
        pthread_mutex_unlock(&g_allocator.global_mutex);
        return NULL;
    }

    void* ptr = NULL;
    size_t actual_size = size;

    // Apply strategy
    switch (strategy) {
        case MEMORY_STRATEGY_POOL:
            // For now, fall back to default - pools would be pre-created
            break;
            
        case MEMORY_STRATEGY_STACK:
            // For now, fall back to default - stacks would be pre-created
            break;
            
        case MEMORY_STRATEGY_ARENA:
            // For now, fall back to default - arenas would be pre-created
            break;
            
        case MEMORY_STRATEGY_TRACKED:
            flags |= MEMORY_FLAG_TRACK;
            break;
            
        default:
            break;
    }

    // Allocate memory with guard pages if enabled
    if (flags & MEMORY_FLAG_GUARD && g_allocator.policy.enable_guard_pages) {
        ptr = allocate_with_guard_pages(size);
        actual_size += 2 * GUARD_PAGE_SIZE;
    } else {
        // Add space for metadata and canaries if tracking
        if (flags & MEMORY_FLAG_TRACK) {
            actual_size += sizeof(AllocationMetadata);
            if (g_allocator.policy.enable_canaries) {
                actual_size += 2 * sizeof(u64); // Front and back canary
            }
        }
        
        // Align if requested
        if (flags & MEMORY_FLAG_ALIGN) {
            actual_size = calculate_aligned_size(actual_size, 64);
        }
        
        ptr = malloc(actual_size);
    }

    if (!ptr) {
        g_allocator.stats.allocation_failures++;
        pthread_mutex_unlock(&g_allocator.global_mutex);
        return NULL;
    }

    // Zero memory if requested
    if (flags & MEMORY_FLAG_ZERO) {
        memset(ptr, 0, actual_size);
    }

    // Set up tracking if requested
    if (flags & MEMORY_FLAG_TRACK) {
        AllocationMetadata* metadata = (AllocationMetadata*)ptr;
        void* user_ptr = (u8*)ptr + sizeof(AllocationMetadata);
        
        // Set up canaries if enabled
        if (g_allocator.policy.enable_canaries) {
            u64* front_canary = (u64*)((u8*)ptr + sizeof(AllocationMetadata));
            u64* back_canary = (u64*)((u8*)ptr + actual_size - sizeof(u64));
            *front_canary = CANARY_VALUE;
            *back_canary = CANARY_VALUE;
            user_ptr = (u8*)user_ptr + sizeof(u64);
        }

        // Fill metadata
        metadata->ptr = user_ptr;
        metadata->size = size;
        metadata->actual_size = actual_size;
        metadata->file = file;
        metadata->line = line;
        metadata->function = function;
        metadata->strategy = strategy;
        metadata->flags = flags;
        metadata->timestamp = time(NULL);
        metadata->allocation_id = g_allocator.next_allocation_id++;
        metadata->next = NULL;
        metadata->prev = NULL;

        add_allocation(metadata);
        ptr = user_ptr;
    }

    // Update statistics
    update_stats(actual_size, true);

    pthread_mutex_unlock(&g_allocator.global_mutex);
    return ptr;
}

void unified_memory_free(void* ptr, const char* file, int line, const char* function) {
    if (!ptr) return;

    if (!g_allocator.initialized) {
        free(ptr); // Fallback to free
        return;
    }

    pthread_mutex_lock(&g_allocator.global_mutex);

    // Find allocation metadata
    AllocationMetadata* metadata = find_allocation(ptr);
    if (!metadata) {
        // Not a tracked allocation, free directly
        free(ptr);
        pthread_mutex_unlock(&g_allocator.global_mutex);
        return;
    }

    // Check for double free
    if (metadata->ptr == NULL) {
        g_allocator.stats.double_frees_detected++;
        pthread_mutex_unlock(&g_allocator.global_mutex);
        return;
    }

    // Check canaries if enabled
    if (g_allocator.policy.enable_canaries) {
        u64* front_canary = (u64*)((u8*)metadata + sizeof(AllocationMetadata));
        u64* back_canary = (u64*)((u8*)metadata + metadata->actual_size - sizeof(u64));
        
        if (!check_canary(front_canary, sizeof(u64)) || !check_canary(back_canary, sizeof(u64))) {
            g_allocator.stats.guard_page_hits++;
        }
    }

    // Update statistics
    update_stats(metadata->actual_size, false);

    // Remove from tracking
    remove_allocation(metadata);

    // Free memory
    if (metadata->flags & MEMORY_FLAG_GUARD && g_allocator.policy.enable_guard_pages) {
        free_with_guard_pages(metadata, metadata->actual_size);
    } else {
        free(metadata);
    }

    pthread_mutex_unlock(&g_allocator.global_mutex);
}

// ============================================================================
// MEMORY POOL FUNCTIONS
// ============================================================================

MemoryPool* unified_memory_pool_create(const MemoryPoolConfig* config) {
    if (!config || !g_allocator.initialized) return NULL;

    MemoryPool* pool = malloc(sizeof(MemoryPool));
    if (!pool) return NULL;

    memset(pool, 0, sizeof(MemoryPool));
    pool->config = *config;

    // Allocate buffer
    pool->buffer_size = config->block_size * config->block_count;
    pool->buffer = malloc(pool->buffer_size);
    if (!pool->buffer) {
        free(pool);
        return NULL;
    }

    // Initialize free list
    pool->free_list = malloc(config->block_count * sizeof(u32));
    if (!pool->free_list) {
        free(pool->buffer);
        free(pool);
        return NULL;
    }

    for (u32 i = 0; i < config->block_count; i++) {
        pool->free_list[i] = i;
    }
    pool->free_head = 0;
    pool->free_blocks = config->block_count;
    pool->total_blocks = config->block_count;

    pthread_mutex_init(&pool->mutex, NULL);
    pool->initialized = true;

    // Add to global pool list
    pthread_mutex_lock(&g_allocator.pools_mutex);
    pool->next = g_allocator.pools;
    g_allocator.pools = pool;
    pthread_mutex_unlock(&g_allocator.pools_mutex);

    return pool;
}

void unified_memory_pool_destroy(MemoryPool* pool) {
    if (!pool) return;

    pthread_mutex_lock(&pool->mutex);
    
    if (pool->buffer) free(pool->buffer);
    if (pool->free_list) free(pool->free_list);
    
    pool->initialized = false;
    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);
    
    free(pool);
}

void* unified_memory_pool_alloc(MemoryPool* pool, size_t size, MemoryFlags flags) {
    if (!pool || !pool->initialized || size > pool->block_size) return NULL;

    pthread_mutex_lock(&pool->mutex);

    if (pool->free_blocks == 0) {
        if (!pool->config.auto_expand) {
            pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }
        // Expand pool logic would go here
        pthread_mutex_unlock(&pool->mutex);
        return NULL;
    }

    u32 block_index = pool->free_list[pool->free_head];
    pool->free_head = (pool->free_head + 1) % pool->total_blocks;
    pool->free_blocks--;

    void* ptr = (u8*)pool->buffer + (block_index * pool->block_size);

    if (flags & MEMORY_FLAG_ZERO) {
        memset(ptr, 0, pool->block_size);
    }

    pthread_mutex_unlock(&pool->mutex);
    return ptr;
}

void unified_memory_pool_free(MemoryPool* pool, void* ptr) {
    if (!pool || !pool->initialized || !ptr) return;

    pthread_mutex_lock(&pool->mutex);

    // Calculate block index
    size_t offset = (u8*)ptr - (u8*)pool->buffer;
    if (offset % pool->block_size != 0) {
        // Invalid pointer
        pthread_mutex_unlock(&pool->mutex);
        return;
    }

    u32 block_index = offset / pool->block_size;
    if (block_index >= pool->total_blocks) {
        // Invalid block index
        pthread_mutex_unlock(&pool->mutex);
        return;
    }

    // Add back to free list
    u32 insert_pos = (pool->free_head + pool->free_blocks) % pool->total_blocks;
    pool->free_list[insert_pos] = block_index;
    pool->free_blocks++;

    pthread_mutex_unlock(&pool->mutex);
}

// ============================================================================
// STACK ALLOCATOR FUNCTIONS
// ============================================================================

StackAllocator* unified_memory_stack_create(size_t size) {
    if (!g_allocator.initialized) return NULL;

    StackAllocator* stack = malloc(sizeof(StackAllocator));
    if (!stack) return NULL;

    memset(stack, 0, sizeof(StackAllocator));
    stack->buffer_size = size;
    stack->buffer = malloc(size);
    if (!stack->buffer) {
        free(stack);
        return NULL;
    }

    pthread_mutex_init(&stack->mutex, NULL);
    stack->initialized = true;

    // Add to global stack list
    pthread_mutex_lock(&g_allocator.stacks_mutex);
    stack->next = g_allocator.stacks;
    g_allocator.stacks = stack;
    pthread_mutex_unlock(&g_allocator.stacks_mutex);

    return stack;
}

void unified_memory_stack_destroy(StackAllocator* stack) {
    if (!stack) return;

    pthread_mutex_lock(&stack->mutex);
    
    if (stack->buffer) free(stack->buffer);
    stack->initialized = false;
    
    pthread_mutex_unlock(&stack->mutex);
    pthread_mutex_destroy(&stack->mutex);
    
    free(stack);
}

void* unified_memory_stack_alloc(StackAllocator* stack, size_t size, MemoryFlags flags) {
    if (!stack || !stack->initialized) return NULL;

    pthread_mutex_lock(&stack->mutex);

    // Align size
    size = (size + 15) & ~15; // 16-byte alignment

    if (stack->current_offset + size > stack->buffer_size) {
        pthread_mutex_unlock(&stack->mutex);
        return NULL;
    }

    void* ptr = (u8*)stack->buffer + stack->current_offset;
    stack->current_offset += size;

    if (stack->current_offset > stack->peak_offset) {
        stack->peak_offset = stack->current_offset;
    }

    if (flags & MEMORY_FLAG_ZERO) {
        memset(ptr, 0, size);
    }

    pthread_mutex_unlock(&stack->mutex);
    return ptr;
}

void unified_memory_stack_reset(StackAllocator* stack) {
    if (!stack || !stack->initialized) return;

    pthread_mutex_lock(&stack->mutex);
    stack->current_offset = 0;
    stack->marker_count = 0;
    pthread_mutex_unlock(&stack->mutex);
}

// ============================================================================
// ARENA ALLOCATOR FUNCTIONS
// ============================================================================

ArenaAllocator* unified_memory_arena_create(size_t initial_block_size) {
    if (!g_allocator.initialized) return NULL;

    ArenaAllocator* arena = malloc(sizeof(ArenaAllocator));
    if (!arena) return NULL;

    memset(arena, 0, sizeof(ArenaAllocator));

    // Initial block
    if (initial_block_size == 0) initial_block_size = 4096;

    arena->block_count = 1;
    arena->blocks = malloc(sizeof(void*));
    arena->block_sizes = malloc(sizeof(size_t));

    if (!arena->blocks || !arena->block_sizes) {
        if (arena->blocks) free(arena->blocks);
        if (arena->block_sizes) free(arena->block_sizes);
        free(arena);
        return NULL;
    }

    arena->blocks[0] = malloc(initial_block_size);
    arena->block_sizes[0] = initial_block_size;

    if (!arena->blocks[0]) {
        free(arena->blocks);
        free(arena->block_sizes);
        free(arena);
        return NULL;
    }

    arena->current_block = 0;
    arena->current_offset = 0;

    pthread_mutex_init(&arena->mutex, NULL);
    arena->initialized = true;

    // Add to global arena list
    pthread_mutex_lock(&g_allocator.arenas_mutex);
    arena->next = g_allocator.arenas;
    g_allocator.arenas = arena;
    pthread_mutex_unlock(&g_allocator.arenas_mutex);

    return arena;
}

void unified_memory_arena_destroy(ArenaAllocator* arena) {
    if (!arena) return;

    pthread_mutex_lock(&arena->mutex);

    if (arena->blocks) {
        for (u32 i = 0; i < arena->block_count; i++) {
            if (arena->blocks[i]) free(arena->blocks[i]);
        }
        free(arena->blocks);
    }
    if (arena->block_sizes) free(arena->block_sizes);

    arena->initialized = false;

    pthread_mutex_unlock(&arena->mutex);
    pthread_mutex_destroy(&arena->mutex);

    free(arena);
}

void* unified_memory_arena_alloc(ArenaAllocator* arena, size_t size, MemoryFlags flags) {
    if (!arena || !arena->initialized) return NULL;

    pthread_mutex_lock(&arena->mutex);

    // Align size
    size = (size + 15) & ~15;

    // Check if fits in current block
    if (arena->current_offset + size > arena->block_sizes[arena->current_block]) {
        // Simple expansion
        size_t new_block_size = arena->block_sizes[arena->current_block] * 2;
        if (new_block_size < size) new_block_size = size * 2;

        void** new_blocks = realloc(arena->blocks, (arena->block_count + 1) * sizeof(void*));
        size_t* new_sizes = realloc(arena->block_sizes, (arena->block_count + 1) * sizeof(size_t));

        if (!new_blocks || !new_sizes) {
            pthread_mutex_unlock(&arena->mutex);
            return NULL;
        }

        arena->blocks = new_blocks;
        arena->block_sizes = new_sizes;

        arena->blocks[arena->block_count] = malloc(new_block_size);
        arena->block_sizes[arena->block_count] = new_block_size;

        if (!arena->blocks[arena->block_count]) {
            pthread_mutex_unlock(&arena->mutex);
            return NULL;
        }

        arena->current_block++;
        arena->block_count++;
        arena->current_offset = 0;
    }

    void* ptr = (u8*)arena->blocks[arena->current_block] + arena->current_offset;
    arena->current_offset += size;
    arena->total_allocated += size;

    if (flags & MEMORY_FLAG_ZERO) {
        memset(ptr, 0, size);
    }

    pthread_mutex_unlock(&arena->mutex);
    return ptr;
}

void unified_memory_arena_reset(ArenaAllocator* arena) {
    if (!arena || !arena->initialized) return;

    pthread_mutex_lock(&arena->mutex);
    arena->current_block = 0;
    arena->current_offset = 0;
    arena->total_allocated = 0;
    pthread_mutex_unlock(&arena->mutex);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

MemoryStats unified_memory_get_stats(void) {
    pthread_mutex_lock(&g_allocator.stats_mutex);
    MemoryStats stats = g_allocator.stats;
    pthread_mutex_unlock(&g_allocator.stats_mutex);
    return stats;
}

void unified_memory_print_stats(void) {
    MemoryStats stats = unified_memory_get_stats();
    
    printf("=== Unified Memory Allocator Statistics ===\n");
    printf("Total Allocations: %llu\n", (unsigned long long)stats.total_allocations);
    printf("Total Deallocations: %llu\n", (unsigned long long)stats.total_deallocations);
    printf("Current Allocations: %llu\n", (unsigned long long)stats.current_allocations);
    printf("Total Allocated: %zu bytes\n", stats.total_allocated);
    printf("Peak Allocated: %zu bytes\n", stats.peak_allocated);
    printf("Fragmentation Ratio: %.2f%%\n", stats.fragmentation_ratio * 100.0);
    printf("Allocation Failures: %llu\n", (unsigned long long)stats.allocation_failures);
    printf("Guard Page Hits: %llu\n", (unsigned long long)stats.guard_page_hits);
    printf("Double Frees Detected: %llu\n", (unsigned long long)stats.double_frees_detected);
    printf("Use After Free Detected: %llu\n", (unsigned long long)stats.use_after_free_detected);
    printf("==========================================\n");
}

void unified_memory_check_leaks(void) {
    pthread_mutex_lock(&g_allocator.allocations_mutex);
    
    if (g_allocator.allocations) {
        printf("=== MEMORY LEAKS DETECTED ===\n");
        AllocationMetadata* alloc = g_allocator.allocations;
        u32 leak_count = 0;
        
        while (alloc) {
            printf("Leak #%u: %zu bytes at %p (%s:%d in %s)\n",
                   leak_count++, alloc->size, alloc->ptr,
                   alloc->file ? alloc->file : "unknown",
                   alloc->line,
                   alloc->function ? alloc->function : "unknown");
            alloc = alloc->next;
        }
        
        printf("Total leaks: %u\n", leak_count);
        printf("============================\n");
    }
    
    pthread_mutex_unlock(&g_allocator.allocations_mutex);
}

// ============================================================================
// INTERNAL HELPER FUNCTIONS
// ============================================================================

static void add_allocation(AllocationMetadata* metadata) {
    pthread_mutex_lock(&g_allocator.allocations_mutex);
    metadata->next = g_allocator.allocations;
    if (g_allocator.allocations) {
        g_allocator.allocations->prev = metadata;
    }
    g_allocator.allocations = metadata;
    pthread_mutex_unlock(&g_allocator.allocations_mutex);
}

static void remove_allocation(AllocationMetadata* metadata) {
    pthread_mutex_lock(&g_allocator.allocations_mutex);
    
    if (metadata->prev) {
        metadata->prev->next = metadata->next;
    } else {
        g_allocator.allocations = metadata->next;
    }
    
    if (metadata->next) {
        metadata->next->prev = metadata->prev;
    }
    
    pthread_mutex_unlock(&g_allocator.allocations_mutex);
}

static AllocationMetadata* find_allocation(void* ptr) {
    pthread_mutex_lock(&g_allocator.allocations_mutex);
    
    AllocationMetadata* alloc = g_allocator.allocations;
    while (alloc) {
        if (alloc->ptr == ptr) {
            pthread_mutex_unlock(&g_allocator.allocations_mutex);
            return alloc;
        }
        alloc = alloc->next;
    }
    
    pthread_mutex_unlock(&g_allocator.allocations_mutex);
    return NULL;
}

static void update_stats(size_t size, bool allocated) {
    pthread_mutex_lock(&g_allocator.stats_mutex);
    
    if (allocated) {
        g_allocator.stats.total_allocations++;
        g_allocator.stats.current_allocations++;
        g_allocator.stats.total_allocated += size;
        
        if (g_allocator.stats.total_allocated > g_allocator.stats.peak_allocated) {
            g_allocator.stats.peak_allocated = g_allocator.stats.total_allocated;
        }
    } else {
        g_allocator.stats.total_deallocations++;
        g_allocator.stats.current_allocations--;
        g_allocator.stats.total_allocated -= size;
    }
    
    pthread_mutex_unlock(&g_allocator.stats_mutex);
}

static size_t calculate_aligned_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static void* allocate_with_guard_pages(size_t size) {
    // Simplified guard page allocation
    size_t total_size = size + 2 * GUARD_PAGE_SIZE;
    void* ptr = malloc(total_size);
    if (ptr) {
        // Mark guard pages as no-access (simplified)
        mprotect(ptr, GUARD_PAGE_SIZE, PROT_NONE);
        mprotect((u8*)ptr + GUARD_PAGE_SIZE + size, GUARD_PAGE_SIZE, PROT_NONE);
    }
    return (u8*)ptr + GUARD_PAGE_SIZE;
}

static void free_with_guard_pages(void* ptr, size_t size) {
    void* actual_ptr = (u8*)ptr - GUARD_PAGE_SIZE;
    // size_t total_size = size + 2 * GUARD_PAGE_SIZE;
    
    // Restore protection before freeing
    mprotect(actual_ptr, GUARD_PAGE_SIZE, PROT_READ | PROT_WRITE);
    mprotect((u8*)actual_ptr + GUARD_PAGE_SIZE + size, GUARD_PAGE_SIZE, PROT_READ | PROT_WRITE);
    
    free(actual_ptr);
}

static void setup_canary(void* ptr, size_t size) {
    u64* canary = (u64*)ptr;
    *canary = CANARY_VALUE;
}

static bool check_canary(void* ptr, size_t size) {
    u64* canary = (u64*)ptr;
    return *canary == CANARY_VALUE;
}

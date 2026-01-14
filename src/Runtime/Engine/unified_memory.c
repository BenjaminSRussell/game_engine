// unified_memory.c - Unified Memory Management Implementation
#include "unified_memory.h"
#include "unified_logging.h"
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <inttypes.h>

#ifdef _WIN32
#include <windows.h>
#define GET_THREAD_ID() GetCurrentThreadId()
#define GET_TIMESTAMP() GetTickCount64()
#define MEMORY_ALIGN(ptr, alignment) _aligned_malloc((size), (alignment))
#define MEMORY_FREE_ALIGNED(ptr) _aligned_free(ptr)
#else
#include <unistd.h>
#include <sys/mman.h>
#define GET_THREAD_ID() ((uint32_t)pthread_self())
#define GET_TIMESTAMP() ((uint64_t)time(NULL) * 1000)
#define MEMORY_ALIGN(ptr, alignment) aligned_alloc((alignment), (size))
#define MEMORY_FREE_ALIGNED(ptr) free(ptr)
#endif

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static struct {
    bool initialized;
    unified_memory_config_t config;
    unified_memory_stats_t stats;
    
    // Allocation tracking
    unified_memory_allocation_t* allocations;
    pthread_mutex_t allocations_mutex;
    uint32_t next_allocation_id;
    
    // Memory pools
    void* memory_pools;
    pthread_mutex_t pools_mutex;
    
    // Arena allocator
    void* arena_allocator;
    pthread_mutex_t arena_mutex;
    
    // Stack allocator
    void* stack_allocator;
    pthread_mutex_t stack_mutex;
    
    // Profiling
    memory_profile_t* profiles;
    pthread_mutex_t profiles_mutex;
    uint32_t profile_count;
    
    // Memory boundaries
    memory_boundary_t* active_boundary;
    pthread_mutex_t boundary_mutex;
    
    // Global mutex for thread safety
    pthread_mutex_t global_mutex;
} g_unified_memory = {0};

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static uint64_t get_timestamp_ms(void) {
    return GET_TIMESTAMP();
}

static void update_stats_allocation(size_t size, unified_memory_strategy_t strategy, unified_memory_flags_t flags) {
    pthread_mutex_lock(&g_unified_memory.global_mutex);
    
    g_unified_memory.stats.total_allocations++;
    g_unified_memory.stats.current_allocations++;
    g_unified_memory.stats.total_allocated += size;
    g_unified_memory.stats.current_allocated += size;
    
    if (g_unified_memory.stats.current_allocated > g_unified_memory.stats.peak_allocated) {
        g_unified_memory.stats.peak_allocated = g_unified_memory.stats.current_allocated;
    }
    
    if (size > g_unified_memory.stats.largest_allocation) {
        g_unified_memory.stats.largest_allocation = size;
    }
    
    if (g_unified_memory.stats.smallest_allocation == 0 || size < g_unified_memory.stats.smallest_allocation) {
        g_unified_memory.stats.smallest_allocation = size;
    }
    
    g_unified_memory.stats.average_allocation_size = 
        (double)g_unified_memory.stats.total_allocated / g_unified_memory.stats.total_allocations;
    
    // Update strategy-specific counters
    switch (strategy) {
        case UNIFIED_MEMORY_STRATEGY_TRACKED:
            g_unified_memory.stats.tracked_allocations++;
            break;
        case UNIFIED_MEMORY_STRATEGY_POOLED:
            g_unified_memory.stats.pooled_allocations++;
            break;
        case UNIFIED_MEMORY_STRATEGY_ARENA:
            g_unified_memory.stats.arena_allocations++;
            break;
        case UNIFIED_MEMORY_STRATEGY_STACK:
            g_unified_memory.stats.stack_allocations++;
            break;
        case UNIFIED_MEMORY_STRATEGY_GPU:
            g_unified_memory.stats.gpu_allocations++;
            break;
        default:
            break;
    }
    
    // Update flag-specific counters
    if (flags & UNIFIED_MEMORY_FLAG_ZERO) {
        g_unified_memory.stats.zeroed_allocations++;
    }
    if (flags & UNIFIED_MEMORY_FLAG_ALIGN) {
        g_unified_memory.stats.aligned_allocations++;
    }
    
    pthread_mutex_unlock(&g_unified_memory.global_mutex);
}

static void update_stats_deallocation(size_t size) {
    pthread_mutex_lock(&g_unified_memory.global_mutex);
    
    g_unified_memory.stats.total_deallocations++;
    g_unified_memory.stats.current_allocations--;
    g_unified_memory.stats.total_freed += size;
    g_unified_memory.stats.current_allocated -= size;
    
    // Calculate fragmentation ratio
    if (g_unified_memory.stats.total_allocated > 0) {
        g_unified_memory.stats.fragmentation_ratio = 
            (float)(g_unified_memory.stats.total_allocated - g_unified_memory.stats.current_allocated) / 
            g_unified_memory.stats.total_allocated;
    }
    
    pthread_mutex_unlock(&g_unified_memory.global_mutex);
}

static unified_memory_allocation_t* create_allocation_metadata(void* ptr, size_t size,
                                                           unified_memory_strategy_t strategy,
                                                           unified_memory_flags_t flags,
                                                           const char* file, int line,
                                                           const char* function, const char* type) {
    unified_memory_allocation_t* alloc = malloc(sizeof(unified_memory_allocation_t));
    if (!alloc) return NULL;
    
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->actual_size = size; // TODO: Calculate actual size including headers
    alloc->file = file;
    alloc->line = line;
    alloc->function = function;
    alloc->type = type;
    alloc->strategy = strategy;
    alloc->flags = flags;
    alloc->timestamp = get_timestamp_ms();
    alloc->allocation_id = g_unified_memory.next_allocation_id++;
    alloc->next = NULL;
    alloc->prev = NULL;
    
    return alloc;
}

static void add_allocation_to_tracking(unified_memory_allocation_t* allocation) {
    if (!allocation) return;
    
    pthread_mutex_lock(&g_unified_memory.allocations_mutex);
    
    // Add to front of linked list
    allocation->next = g_unified_memory.allocations;
    if (g_unified_memory.allocations) {
        g_unified_memory.allocations->prev = allocation;
    }
    g_unified_memory.allocations = allocation;
    
    pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
}

static unified_memory_allocation_t* find_allocation_metadata(void* ptr) {
    pthread_mutex_lock(&g_unified_memory.allocations_mutex);
    
    unified_memory_allocation_t* current = g_unified_memory.allocations;
    while (current) {
        if (current->ptr == ptr) {
            pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
            return current;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
    return NULL;
}

static void remove_allocation_from_tracking(unified_memory_allocation_t* allocation) {
    if (!allocation) return;
    
    pthread_mutex_lock(&g_unified_memory.allocations_mutex);
    
    if (allocation->prev) {
        allocation->prev->next = allocation->next;
    } else {
        g_unified_memory.allocations = allocation->next;
    }
    
    if (allocation->next) {
        allocation->next->prev = allocation->prev;
    }
    
    pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
}

/* ============================================================================
 * CORE ALLOCATION FUNCTIONS
 * ============================================================================ */

void* unified_memory_alloc(size_t size, unified_memory_strategy_t strategy,
                        unified_memory_flags_t flags, const char* file, int line,
                        const char* function, const char* type) {
    if (!g_unified_memory.initialized) {
        unified_memory_init();
    }
    
    if (size == 0) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "Zero size allocation requested");
        return NULL;
    }
    
    // Check global memory limit
    if (g_unified_memory.config.global_memory_limit > 0) {
        if (g_unified_memory.stats.current_allocated + size > g_unified_memory.config.global_memory_limit) {
            REPORT_ERROR(ERROR_OUT_OF_MEMORY, "Global memory limit exceeded");
            g_unified_memory.stats.allocation_failures++;
            return NULL;
        }
    }
    
    // Check per-allocation limit
    if (g_unified_memory.config.per_allocation_limit > 0 && size > g_unified_memory.config.per_allocation_limit) {
        REPORT_ERROR(ERROR_OUT_OF_MEMORY, "Per-allocation size limit exceeded");
        g_unified_memory.stats.allocation_failures++;
        return NULL;
    }
    
    void* ptr = NULL;
    size_t actual_size = size;
    
    // Apply alignment if requested
    if (flags & UNIFIED_MEMORY_FLAG_ALIGN) {
        actual_size = (size + 63) & ~63; // Align to 64-byte boundary
    }
    
    // Allocate based on strategy
    switch (strategy) {
        case UNIFIED_MEMORY_STRATEGY_DEFAULT:
            if (flags & UNIFIED_MEMORY_FLAG_ALIGN) {
                ptr = MEMORY_ALIGN(actual_size, 64);
            } else {
                ptr = malloc(actual_size);
            }
            break;
            
        case UNIFIED_MEMORY_STRATEGY_TRACKED:
            if (flags & UNIFIED_MEMORY_FLAG_ALIGN) {
                ptr = MEMORY_ALIGN(actual_size, 64);
            } else {
                ptr = malloc(actual_size);
            }
            break;
            
        case UNIFIED_MEMORY_STRATEGY_POOLED:
            // TODO: Implement memory pool allocation
            ptr = malloc(actual_size);
            break;
            
        case UNIFIED_MEMORY_STRATEGY_ARENA:
            // TODO: Implement arena allocation
            ptr = malloc(actual_size);
            break;
            
        case UNIFIED_MEMORY_STRATEGY_STACK:
            // TODO: Implement stack allocation
            ptr = malloc(actual_size);
            break;
            
        case UNIFIED_MEMORY_STRATEGY_GPU:
            // TODO: Implement GPU allocation
            ptr = malloc(actual_size);
            break;
            
        default:
            ptr = malloc(actual_size);
            break;
    }
    
    if (!ptr) {
        REPORT_ERROR(ERROR_OUT_OF_MEMORY, "Memory allocation failed");
        g_unified_memory.stats.allocation_failures++;
        return NULL;
    }
    
    // Zero-initialize if requested
    if (flags & UNIFIED_MEMORY_FLAG_ZERO) {
        memset(ptr, 0, size);
    }
    
    // Add guard pages if requested
    if (flags & UNIFIED_MEMORY_FLAG_GUARD) {
        unified_memory_set_guard_pages(ptr, size);
    }
    
    // Track allocation if enabled
    if (g_unified_memory.config.enable_tracking || (flags & UNIFIED_MEMORY_FLAG_TRACK)) {
        unified_memory_allocation_t* metadata = create_allocation_metadata(
            ptr, size, strategy, flags, file, line, function, type);
        if (metadata) {
            add_allocation_to_tracking(metadata);
        }
    }
    
    update_stats_allocation(size, strategy, flags);
    
    LOG_MEMORY_DEBUG("Allocated %zu bytes at %p (strategy=%d, flags=%d)", size, ptr, strategy, flags);
    
    return ptr;
}

void* unified_memory_realloc(void* ptr, size_t new_size,
                           unified_memory_strategy_t strategy,
                           unified_memory_flags_t flags, const char* file, int line,
                           const char* function, const char* type) {
    if (!ptr) {
        return unified_memory_alloc(new_size, strategy, flags, file, line, function, type);
    }
    
    if (new_size == 0) {
        unified_memory_free(ptr, file, line, function);
        return NULL;
    }
    
    unified_memory_allocation_t* metadata = find_allocation_metadata(ptr);
    size_t old_size = metadata ? metadata->size : 0;
    
    void* new_ptr = unified_memory_alloc(new_size, strategy, flags, file, line, function, type);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy old data
    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    
    // Free old pointer
    unified_memory_free(ptr, file, line, function);
    
    g_unified_memory.stats.reallocations++;
    
    LOG_MEMORY_DEBUG("Reallocated %zu bytes to %zu bytes (old=%p, new=%p)", 
                   old_size, new_size, ptr, new_ptr);
    
    return new_ptr;
}

void unified_memory_free(void* ptr, const char* file, int line, const char* function) {
    if (!ptr) return;
    
    unified_memory_allocation_t* metadata = find_allocation_metadata(ptr);
    size_t size = metadata ? metadata->size : 0;
    
    // Remove from tracking
    if (metadata) {
        remove_allocation_from_tracking(metadata);
        free(metadata);
    }
    
    // Free based on strategy
    // Note: For now, we use standard free for all strategies
    // TODO: Implement strategy-specific deallocation
    free(ptr);
    
    update_stats_deallocation(size);
    
    LOG_MEMORY_DEBUG("Freed %zu bytes at %p", size, ptr);
}

void* unified_memory_calloc(size_t count, size_t size,
                          unified_memory_strategy_t strategy,
                          unified_memory_flags_t flags, const char* file, int line,
                          const char* function, const char* type) {
    size_t total_size = count * size;
    
    // Check for overflow
    if (count > 0 && size > SIZE_MAX / count) {
        REPORT_ERROR(ERROR_INVALID_PARAMETER, "calloc size overflow");
        return NULL;
    }
    
    return unified_memory_alloc(total_size, strategy, flags | UNIFIED_MEMORY_FLAG_ZERO,
                             file, line, function, type);
}

char* unified_memory_strdup(const char* str, unified_memory_strategy_t strategy,
                          unified_memory_flags_t flags, const char* file, int line,
                          const char* function) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* new_str = (char*)unified_memory_alloc(len, strategy, flags, file, line, function, "string");
    if (new_str) {
        memcpy(new_str, str, len);
    }
    
    return new_str;
}

/* ============================================================================
 * MEMORY MANAGEMENT FUNCTIONS
 * ============================================================================ */

void unified_memory_init(void) {
    if (g_unified_memory.initialized) return;
    
    pthread_mutex_init(&g_unified_memory.global_mutex, NULL);
    pthread_mutex_init(&g_unified_memory.allocations_mutex, NULL);
    pthread_mutex_init(&g_unified_memory.pools_mutex, NULL);
    pthread_mutex_init(&g_unified_memory.arena_mutex, NULL);
    pthread_mutex_init(&g_unified_memory.stack_mutex, NULL);
    pthread_mutex_init(&g_unified_memory.profiles_mutex, NULL);
    pthread_mutex_init(&g_unified_memory.boundary_mutex, NULL);
    
    // Initialize default configuration
    g_unified_memory.config.global_memory_limit = 0; // No limit
    g_unified_memory.config.per_allocation_limit = 0; // No limit
    g_unified_memory.config.enable_tracking = true;
    g_unified_memory.config.enable_guard_pages = false;
    g_unified_memory.config.enable_leak_detection = true;
    g_unified_memory.config.enable_corruption_detection = false;
    g_unified_memory.config.enable_profiling = false;
    g_unified_memory.config.allocation_limit = 0;
    g_unified_memory.config.fragmentation_threshold = 0.5f;
    g_unified_memory.config.auto_compact = false;
    
    memset(&g_unified_memory.stats, 0, sizeof(g_unified_memory.stats));
    
    g_unified_memory.initialized = true;
    g_unified_memory.next_allocation_id = 1;
    
    LOG_CORE_INFO("Unified memory system initialized");
}

void unified_memory_shutdown(void) {
    if (!g_unified_memory.initialized) return;
    
    // Report memory leaks
    if (g_unified_memory.config.enable_leak_detection) {
        unified_memory_dump_leaks();
    }
    
    // Free all tracked allocations
    pthread_mutex_lock(&g_unified_memory.allocations_mutex);
    unified_memory_allocation_t* current = g_unified_memory.allocations;
    while (current) {
        unified_memory_allocation_t* next = current->next;
        free(current->ptr);
        free(current);
        current = next;
    }
    g_unified_memory.allocations = NULL;
    pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
    
    // Cleanup profiles
    pthread_mutex_lock(&g_unified_memory.profiles_mutex);
    if (g_unified_memory.profiles) {
        free(g_unified_memory.profiles);
        g_unified_memory.profiles = NULL;
    }
    g_unified_memory.profile_count = 0;
    pthread_mutex_unlock(&g_unified_memory.profiles_mutex);
    
    // Destroy mutexes
    pthread_mutex_destroy(&g_unified_memory.global_mutex);
    pthread_mutex_destroy(&g_unified_memory.allocations_mutex);
    pthread_mutex_destroy(&g_unified_memory.pools_mutex);
    pthread_mutex_destroy(&g_unified_memory.arena_mutex);
    pthread_mutex_destroy(&g_unified_memory.stack_mutex);
    pthread_mutex_destroy(&g_unified_memory.profiles_mutex);
    pthread_mutex_destroy(&g_unified_memory.boundary_mutex);
    
    g_unified_memory.initialized = false;
    
    LOG_CORE_INFO("Unified memory system shutdown");
}

void unified_memory_get_stats(unified_memory_stats_t* stats) {
    if (!stats) return;
    
    pthread_mutex_lock(&g_unified_memory.global_mutex);
    *stats = g_unified_memory.stats;
    pthread_mutex_unlock(&g_unified_memory.global_mutex);
}

void unified_memory_dump_stats(void) {
    unified_memory_stats_t stats;
    unified_memory_get_stats(&stats);
    
    LOG_CORE_INFO("=== Memory Statistics ===");
    LOG_CORE_INFO("Total allocations: %" PRIu64, stats.total_allocations);
    LOG_CORE_INFO("Total deallocations: %" PRIu64, stats.total_deallocations);
    LOG_CORE_INFO("Current allocations: %" PRIu64, stats.current_allocations);
    LOG_CORE_INFO("Total allocated: %zu bytes", stats.total_allocated);
    LOG_CORE_INFO("Total freed: %zu bytes", stats.total_freed);
    LOG_CORE_INFO("Current allocated: %zu bytes", stats.current_allocated);
    LOG_CORE_INFO("Peak allocated: %zu bytes", stats.peak_allocated);
    LOG_CORE_INFO("Allocation failures: %" PRIu64, stats.allocation_failures);
    LOG_CORE_INFO("Reallocations: %" PRIu64, stats.reallocations);
    LOG_CORE_INFO("Fragmentation ratio: %.2f%%", stats.fragmentation_ratio * 100.0f);
    LOG_CORE_INFO("Largest allocation: %zu bytes", stats.largest_allocation);
    LOG_CORE_INFO("Smallest allocation: %zu bytes", stats.smallest_allocation);
    LOG_CORE_INFO("Average allocation: %.2f bytes", stats.average_allocation_size);
    
    LOG_CORE_INFO("Strategy breakdown:");
    LOG_CORE_INFO("  Tracked: %" PRIu64, stats.tracked_allocations);
    LOG_CORE_INFO("  Pooled: %" PRIu64, stats.pooled_allocations);
    LOG_CORE_INFO("  Arena: %" PRIu64, stats.arena_allocations);
    LOG_CORE_INFO("  Stack: %" PRIu64, stats.stack_allocations);
    LOG_CORE_INFO("  GPU: %" PRIu64, stats.gpu_allocations);
    
    LOG_CORE_INFO("Flag breakdown:");
    LOG_CORE_INFO("  Zeroed: %" PRIu64, stats.zeroed_allocations);
    LOG_CORE_INFO("  Aligned: %" PRIu64, stats.aligned_allocations);
}

void unified_memory_dump_leaks(void) {
    pthread_mutex_lock(&g_unified_memory.allocations_mutex);
    
    if (!g_unified_memory.allocations) {
        LOG_CORE_INFO("No memory leaks detected");
        pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
        return;
    }
    
    LOG_CORE_WARNING("=== Memory Leaks Detected ===");
    LOG_CORE_WARNING("Total leaked allocations: %zu", 
                    g_unified_memory.stats.current_allocations);
    
    unified_memory_allocation_t* current = g_unified_memory.allocations;
    size_t total_leaked = 0;
    uint32_t leak_count = 0;
    
    while (current) {
        total_leaked += current->size;
        leak_count++;
        
        LOG_CORE_WARNING("Leak #%u: %zu bytes at %p (%s:%d in %s) - Type: %s",
                        current->allocation_id, current->size, current->ptr,
                        current->file, current->line, current->function, current->type);
        
        current = current->next;
    }
    
    LOG_CORE_WARNING("Total leaked memory: %zu bytes in %u allocations", total_leaked, leak_count);
    
    pthread_mutex_unlock(&g_unified_memory.allocations_mutex);
}

/* ============================================================================
 * MEMORY VALIDATION AND DEBUGGING
 * ============================================================================ */

bool unified_memory_validate_pointer(void* ptr) {
    if (!ptr) return false;
    
    unified_memory_allocation_t* metadata = find_allocation_metadata(ptr);
    return metadata != NULL;
}

bool unified_memory_check_corruption(void* ptr) {
    // TODO: Implement corruption detection using guard patterns
    (void)ptr;
    return false;
}

void unified_memory_set_guard_pages(void* ptr, size_t size) {
    // TODO: Implement guard page setup using mmap/VirtualAlloc
    (void)ptr;
    (void)size;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

bool unified_memory_safe_copy(void* dest, size_t dest_size, const void* src, size_t src_size) {
    if (!dest || !src) return false;
    
    size_t copy_size = (dest_size < src_size) ? dest_size : src_size;
    memcpy(dest, src, copy_size);
    
    return true;
}

bool unified_memory_safe_set(void* dest, size_t dest_size, int value, size_t count) {
    if (!dest) return false;
    
    size_t set_size = (dest_size < count) ? dest_size : count;
    memset(dest, value, set_size);
    
    return true;
}

int unified_memory_safe_compare(const void* mem1, size_t size1, const void* mem2, size_t size2) {
    if (!mem1 || !mem2) return -1;
    
    size_t compare_size = (size1 < size2) ? size1 : size2;
    return memcmp(mem1, mem2, compare_size);
}

/* ============================================================================
 * CONFIGURATION FUNCTIONS
 * ============================================================================ */

void unified_memory_set_config(const unified_memory_config_t* config) {
    if (!config) return;
    
    pthread_mutex_lock(&g_unified_memory.global_mutex);
    g_unified_memory.config = *config;
    pthread_mutex_unlock(&g_unified_memory.global_mutex);
}

void unified_memory_get_config(unified_memory_config_t* config) {
    if (!config) return;
    
    pthread_mutex_lock(&g_unified_memory.global_mutex);
    *config = g_unified_memory.config;
    pthread_mutex_unlock(&g_unified_memory.global_mutex);
}

/* ============================================================================
 * MEMORY PROFILING
 * ============================================================================ */

void memory_profile_start(const char* name) {
    if (!name) return;
    
    pthread_mutex_lock(&g_unified_memory.profiles_mutex);
    
    // Find existing profile or create new one
    memory_profile_t* profile = NULL;
    for (uint32_t i = 0; i < g_unified_memory.profile_count; i++) {
        if (strcmp(g_unified_memory.profiles[i].name, name) == 0) {
            profile = &g_unified_memory.profiles[i];
            break;
        }
    }
    
    if (!profile) {
        // Expand profiles array
        g_unified_memory.profiles = realloc(g_unified_memory.profiles,
                                          sizeof(memory_profile_t) * (g_unified_memory.profile_count + 1));
        profile = &g_unified_memory.profiles[g_unified_memory.profile_count];
        memset(profile, 0, sizeof(memory_profile_t));
        profile->name = strdup(name);
        g_unified_memory.profile_count++;
    }
    
    profile->start_time = get_timestamp_ms();
    profile->active = true;
    
    // Get current stats
    unified_memory_get_stats(&g_unified_memory.stats);
    profile->bytes_allocated = g_unified_memory.stats.total_allocated;
    profile->bytes_freed = g_unified_memory.stats.total_freed;
    profile->allocations_count = g_unified_memory.stats.total_allocations;
    profile->deallocations_count = g_unified_memory.stats.total_deallocations;
    
    pthread_mutex_unlock(&g_unified_memory.profiles_mutex);
}

void memory_profile_end(const char* name) {
    if (!name) return;
    
    pthread_mutex_lock(&g_unified_memory.profiles_mutex);
    
    for (uint32_t i = 0; i < g_unified_memory.profile_count; i++) {
        if (strcmp(g_unified_memory.profiles[i].name, name) == 0) {
            memory_profile_t* profile = &g_unified_memory.profiles[i];
            profile->end_time = get_timestamp_ms();
            profile->active = false;
            
            // Calculate deltas
            unified_memory_get_stats(&g_unified_memory.stats);
            profile->bytes_allocated = g_unified_memory.stats.total_allocated - profile->bytes_allocated;
            profile->bytes_freed = g_unified_memory.stats.total_freed - profile->bytes_freed;
            profile->allocations_count = g_unified_memory.stats.total_allocations - profile->allocations_count;
            profile->deallocations_count = g_unified_memory.stats.total_deallocations - profile->deallocations_count;
            
            break;
        }
    }
    
    pthread_mutex_unlock(&g_unified_memory.profiles_mutex);
}

void memory_profile_dump(const char* name) {
    if (!name) return;
    
    pthread_mutex_lock(&g_unified_memory.profiles_mutex);
    
    for (uint32_t i = 0; i < g_unified_memory.profile_count; i++) {
        if (strcmp(g_unified_memory.profiles[i].name, name) == 0) {
            memory_profile_t* profile = &g_unified_memory.profiles[i];
            
            LOG_CORE_INFO("=== Memory Profile: %s ===", profile->name);
            LOG_CORE_INFO("Duration: %" PRIu64 " ms", profile->end_time - profile->start_time);
            LOG_CORE_INFO("Bytes allocated: %zu", profile->bytes_allocated);
            LOG_CORE_INFO("Bytes freed: %zu", profile->bytes_freed);
            LOG_CORE_INFO("Allocations: %u", profile->allocations_count);
            LOG_CORE_INFO("Deallocations: %u", profile->deallocations_count);
            
            break;
        }
    }
    
    pthread_mutex_unlock(&g_unified_memory.profiles_mutex);
}

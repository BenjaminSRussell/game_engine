// src/engine/rendering/gpu_memory.c
// GPU Memory Management and Defragmentation System

#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <Metal/Metal.h>
#endif

#include "gpu_memory.h"
#include "../backend/metal/mtl_device.h"

// ============================================================================
// Constants and Configuration
// ============================================================================

#define MAX_MEMORY_BLOCKS 4096
#define MAX_MEMORY_POOLS 16
#define MIN_BLOCK_SIZE 64          // 64 bytes minimum allocation
#define MAX_BLOCK_SIZE (256 * 1024 * 1024)  // 256 MB maximum allocation
#define DEFRAGMENTATION_THRESHOLD 0.7f  // Trigger defrag at 70% fragmentation
#define MEMORY_ALIGNMENT 256       // 256-byte alignment for GPU memory

// Memory pool types

// ============================================================================
// Memory Block Structure
// ============================================================================

typedef struct MemoryBlock {
    uint64_t offset;
    uint64_t size;
    bool is_free;
    bool is_dirty;
    
    // Allocation tracking
    uint32_t allocation_id;
    char debug_name[256];
    MemoryPoolType pool_type;
    
    // Defragmentation
    uint64_t new_offset;
    bool needs_move;
    
    // Linked list for free list management
    uint32_t prev_block;
    uint32_t next_block;
} MemoryBlock;

// ============================================================================
// Memory Pool Structure
// ============================================================================

typedef struct GPUMemoryPool {
    MemoryPoolType type;
    char name[64];
    
    // Memory backing
#ifdef __APPLE__
    id<MTLBuffer> metal_buffer;
    void* cpu_pointer;  // For shared memory
#endif
    uint64_t size;
    uint64_t used_size;
    
    // Block management
    MemoryBlock blocks[MAX_MEMORY_BLOCKS];
    uint32_t block_count;
    uint32_t free_list_head;
    
    // Statistics
    uint32_t allocation_count;
    uint32_t free_count;
    uint64_t total_allocated;
    uint64_t peak_usage;
    
    // Defragmentation
    bool needs_defragmentation;
    float fragmentation_ratio;
} GPUMemoryPool;

// ============================================================================
// GPU Memory Manager Structure
// ============================================================================

typedef struct GPUMemoryManager {
    GPUMemoryPool pools[MAX_MEMORY_POOLS];
    uint32_t pool_count;
    
    // Global statistics
    struct {
        uint64_t total_memory;
        uint64_t used_memory;
        uint64_t free_memory;
        uint32_t total_allocations;
        uint32_t total_frees;
        uint64_t peak_usage;
    } stats;
    
    // Defragmentation settings
    struct {
        bool enabled;
        uint32_t defragmentation_interval_ms;
        uint32_t last_defragmentation_time;
        bool force_next_frame;
    } defrag;
    
    // Allocation ID counter
    uint32_t next_allocation_id;
    
    // Validation state
    bool validation_enabled;
    uint64_t validation_errors;
    uint64_t last_validation_time;
    uint64_t total_defragmentations;
    uint64_t failed_defragmentations;
    uint64_t memory_leaks_detected;
    
    bool initialized;
} GPUMemoryManager;

static GPUMemoryManager* g_memory_manager = NULL;

// ============================================================================
// Utility Functions
// ============================================================================

static const char* pool_type_to_string(MemoryPoolType type);
static uint64_t align_size(uint64_t size, uint64_t alignment);
static float calculate_fragmentation_ratio(GPUMemoryPool* pool);
static bool gpu_memory_validate_allocation(GPUMemoryAllocation* allocation);
static void gpu_memory_log_validation_error(const char* error);
static uint64_t gpu_memory_get_timestamp(void);
static bool gpu_memory_validate_pool_integrity(GPUMemoryPool* pool);
static bool gpu_memory_detect_memory_leaks(void);

static uint64_t align_size(uint64_t size, uint64_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static const char* pool_type_to_string(MemoryPoolType type) {
    switch (type) {
        case MEMORY_POOL_TEXTURE: return "texture";
        case MEMORY_POOL_BUFFER: return "buffer";
        case MEMORY_POOL_UNIFORM: return "uniform";
        case MEMORY_POOL_VERTEX: return "vertex";
        case MEMORY_POOL_INDEX: return "index";
        default: return "unknown";
    }
}

static float calculate_fragmentation_ratio(GPUMemoryPool* pool) {
    if (pool->block_count <= 1) return 0.0f;
    
    uint64_t total_free = 0;
    uint32_t free_blocks = 0;
    
    for (uint32_t i = 0; i < pool->block_count; i++) {
        if (pool->blocks[i].is_free) {
            total_free += pool->blocks[i].size;
            free_blocks++;
        }
    }
    
    if (total_free == 0) return 0.0f;
    
    // Fragmentation ratio based on free block count vs total free size
    float avg_free_size = (float)total_free / free_blocks;
    float ideal_free_size = (float)total_free / 1;  // One contiguous block would be ideal
    
    return 1.0f - (avg_free_size / ideal_free_size);
}

// ============================================================================
// Memory Pool Management
// ============================================================================

static bool create_memory_pool(MemoryPoolType type, uint64_t size, const char* name) {
    if (!g_memory_manager || g_memory_manager->pool_count >= MAX_MEMORY_POOLS) {
        return false;
    }
    
    GPUMemoryPool* pool = &g_memory_manager->pools[g_memory_manager->pool_count];
    memset(pool, 0, sizeof(GPUMemoryPool));
    
    pool->type = type;
    pool->size = align_size(size, MEMORY_ALIGNMENT);
    
    if (name) {
        strncpy(pool->name, name, sizeof(pool->name) - 1);
    } else {
        snprintf(pool->name, sizeof(pool->name), "pool_%s", pool_type_to_string(type));
    }
    
#ifdef __APPLE__
    // Create Metal buffer for this pool
    MTLResourceOptions options = MTLResourceStorageModeShared;
    if (type == MEMORY_POOL_TEXTURE) {
        options = MTLResourceStorageModePrivate;
    }
    
    pool->metal_buffer = [g_metal_device newBufferWithLength:pool->size options:options];
    if (!pool->metal_buffer) {
        LOG_ERROR(LOG_CAT_MEMORY, "Failed to create Metal buffer for pool '%s'", pool->name);
        return false;
    }
    
    if (options == MTLResourceStorageModeShared) {
        pool->cpu_pointer = [pool->metal_buffer contents];
    }
#endif
    
    // Create initial free block
    MemoryBlock* block = &pool->blocks[0];
    block->offset = 0;
    block->size = pool->size;
    block->is_free = true;
    block->is_dirty = false;
    block->allocation_id = 0;
    block->pool_type = type;
    block->prev_block = UINT32_MAX;
    block->next_block = UINT32_MAX;
    
    pool->block_count = 1;
    pool->free_list_head = 0;
    
    g_memory_manager->pool_count++;
    g_memory_manager->stats.total_memory += pool->size;
    
    LOG_INFO(LOG_CAT_MEMORY, "Created memory pool '%s': %llu bytes", pool->name, pool->size);
    return true;
}

static GPUMemoryPool* find_pool(MemoryPoolType type) {
    if (!g_memory_manager) return NULL;
    
    for (uint32_t i = 0; i < g_memory_manager->pool_count; i++) {
        if (g_memory_manager->pools[i].type == type) {
            return &g_memory_manager->pools[i];
        }
    }
    return NULL;
}

static uint32_t find_best_fit_block(GPUMemoryPool* pool, uint64_t size) {
    uint32_t best_block = UINT32_MAX;
    uint64_t best_size = UINT64_MAX;
    
    uint32_t current = pool->free_list_head;
    while (current != UINT32_MAX) {
        MemoryBlock* block = &pool->blocks[current];
        
        if (block->is_free && block->size >= size) {
            if (block->size < best_size) {
                best_size = block->size;
                best_block = current;
                
                // Exact match found
                if (block->size == size) {
                    break;
                }
            }
        }
        
        current = block->next_block;
    }
    
    return best_block;
}

static void remove_from_free_list(GPUMemoryPool* pool, uint32_t block_index) {
    MemoryBlock* block = &pool->blocks[block_index];
    
    if (block->prev_block != UINT32_MAX) {
        pool->blocks[block->prev_block].next_block = block->next_block;
    } else {
        pool->free_list_head = block->next_block;
    }
    
    if (block->next_block != UINT32_MAX) {
        pool->blocks[block->next_block].prev_block = block->prev_block;
    }
    
    block->prev_block = UINT32_MAX;
    block->next_block = UINT32_MAX;
}

static void add_to_free_list(GPUMemoryPool* pool, uint32_t block_index) {
    MemoryBlock* block = &pool->blocks[block_index];
    
    // Insert at head of free list
    block->prev_block = UINT32_MAX;
    block->next_block = pool->free_list_head;
    
    if (pool->free_list_head != UINT32_MAX) {
        pool->blocks[pool->free_list_head].prev_block = block_index;
    }
    
    pool->free_list_head = block_index;
}

static void merge_adjacent_free_blocks(GPUMemoryPool* pool) {
    for (uint32_t i = 0; i < pool->block_count; i++) {
        MemoryBlock* block = &pool->blocks[i];
        
        if (!block->is_free) continue;
        
        // Try to merge with next block
        if (i + 1 < pool->block_count && pool->blocks[i + 1].is_free) {
            MemoryBlock* next_block = &pool->blocks[i + 1];
            
            // Remove next block from free list
            remove_from_free_list(pool, i + 1);
            
            // Merge sizes
            block->size += next_block->size;
            
            // Remove next block by shifting remaining blocks
            for (uint32_t j = i + 1; j < pool->block_count - 1; j++) {
                pool->blocks[j] = pool->blocks[j + 1];
            }
            
            pool->block_count--;
            
            // Update free_list_head if it points to a moved block
            if (pool->free_list_head != UINT32_MAX && pool->free_list_head > i + 1) {
                pool->free_list_head--;
            }

            // Update indices in all blocks
            for (uint32_t j = 0; j < pool->block_count; j++) {
                if (pool->blocks[j].is_free) {
                     if (pool->blocks[j].prev_block != UINT32_MAX && pool->blocks[j].prev_block > i + 1) {
                         pool->blocks[j].prev_block--;
                     }
                     if (pool->blocks[j].next_block != UINT32_MAX && pool->blocks[j].next_block > i + 1) {
                         pool->blocks[j].next_block--;
                     }
                }
            }
            
            // Recheck this block
            i--;
        }
    }
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool gpu_memory_init(void) {
    if (g_memory_manager) {
        LOG_WARN(LOG_CAT_MEMORY, "GPU memory manager already initialized");
        return true;
    }
    
    g_memory_manager = calloc(1, sizeof(GPUMemoryManager));
    if (!g_memory_manager) {
        LOG_ERROR(LOG_CAT_MEMORY, "Failed to allocate GPU memory manager");
        return false;
    }
    
    // Initialize validation state
    g_memory_manager->validation_enabled = true;
    g_memory_manager->validation_errors = 0;
    g_memory_manager->last_validation_time = gpu_memory_get_timestamp();
    g_memory_manager->total_defragmentations = 0;
    g_memory_manager->failed_defragmentations = 0;
    g_memory_manager->memory_leaks_detected = 0;
    
    // Create default memory pools
    uint64_t texture_pool_size = 512 * 1024 * 1024;  // 512 MB
    uint64_t buffer_pool_size = 256 * 1024 * 1024;   // 256 MB
    uint64_t vertex_pool_size = 128 * 1024 * 1024;   // 128 MB
    uint64_t uniform_pool_size = 64 * 1024 * 1024;   // 64 MB
    
    if (!create_memory_pool(MEMORY_POOL_TEXTURE, texture_pool_size, "texture_pool") ||
        !create_memory_pool(MEMORY_POOL_BUFFER, buffer_pool_size, "buffer_pool") ||
        !create_memory_pool(MEMORY_POOL_VERTEX, vertex_pool_size, "vertex_pool") ||
        !create_memory_pool(MEMORY_POOL_UNIFORM, uniform_pool_size, "uniform_pool")) {
        LOG_ERROR(LOG_CAT_MEMORY, "Failed to create default memory pools");
        free(g_memory_manager);
        g_memory_manager = NULL;
        return false;
    }
    
    // Initialize defragmentation settings
    g_memory_manager->defrag.enabled = true;
    g_memory_manager->defrag.defragmentation_interval_ms = 1000;  // 1 second
    g_memory_manager->defrag.last_defragmentation_time = 0;
    g_memory_manager->defrag.force_next_frame = false;
    
    g_memory_manager->next_allocation_id = 1;
    g_memory_manager->initialized = true;
    
    LOG_INFO(LOG_CAT_MEMORY, "GPU memory manager initialized with validation enabled");
    return true;
}

void gpu_memory_shutdown(void) {
    if (!g_memory_manager) {
        return;
    }
    
    // Report statistics before destruction
    if (g_memory_manager->validation_enabled) {
        LOG_INFO(LOG_CAT_MEMORY, "GPU Memory Management Statistics:");
        LOG_INFO(LOG_CAT_MEMORY, "  Validation errors: %lu", g_memory_manager->validation_errors);
        LOG_INFO(LOG_CAT_MEMORY, "  Total defragmentations: %lu", g_memory_manager->total_defragmentations);
        LOG_INFO(LOG_CAT_MEMORY, "  Failed defragmentations: %lu", g_memory_manager->failed_defragmentations);
        LOG_INFO(LOG_CAT_MEMORY, "  Memory leaks detected: %lu", g_memory_manager->memory_leaks_detected);
        
        // Check for memory leaks before shutdown
        if (gpu_memory_detect_memory_leaks()) {
            LOG_WARN(LOG_CAT_MEMORY, "Memory leaks detected during shutdown");
        }
    }
    
    // Cleanup all pools
    for (uint32_t i = 0; i < g_memory_manager->pool_count; i++) {
        GPUMemoryPool* pool = &g_memory_manager->pools[i];
        
#ifdef __APPLE__
        if (pool->metal_buffer) {
            [pool->metal_buffer release];
        }
#endif
    }
    
    free(g_memory_manager);
    g_memory_manager = NULL;
    
    LOG_INFO(LOG_CAT_MEMORY, "GPU memory manager shutdown");
}

GPUMemoryAllocation gpu_memory_allocate(MemoryPoolType type, uint64_t size, const char* name) {
    GPUMemoryAllocation allocation = {0};
    
    if (!g_memory_manager) {
        LOG_ERROR(LOG_CAT_MEMORY, "GPU memory manager not initialized");
        return allocation;
    }
    
    GPUMemoryPool* pool = find_pool(type);
    if (!pool) {
        LOG_ERROR(LOG_CAT_MEMORY, "No pool found for type %s", pool_type_to_string(type));
        return allocation;
    }
    
    uint64_t aligned_size = align_size(size, MEMORY_ALIGNMENT);
    
    // Find best fit block
    uint32_t block_index = find_best_fit_block(pool, aligned_size);
    if (block_index == UINT32_MAX) {
        LOG_ERROR(LOG_CAT_MEMORY, "Failed to find free block of size %llu in pool '%s'",
                  aligned_size, pool->name);
        return allocation;
    }
    
    MemoryBlock* block = &pool->blocks[block_index];
    
    // If block is larger than needed, split it
    if (block->size > aligned_size + MIN_BLOCK_SIZE) {
        if (pool->block_count >= MAX_MEMORY_BLOCKS) {
            LOG_ERROR(LOG_CAT_MEMORY, "Too many memory blocks in pool '%s'", pool->name);
            return allocation;
        }
        
        // Create new block for remaining space
        MemoryBlock* new_block = &pool->blocks[pool->block_count];
        memset(new_block, 0, sizeof(MemoryBlock));
        
        new_block->offset = block->offset + aligned_size;
        new_block->size = block->size - aligned_size;
        new_block->is_free = true;
        new_block->is_dirty = false;
        new_block->allocation_id = 0;
        new_block->pool_type = type;
        
        // Update original block
        block->size = aligned_size;
        
        // Add new block to free list
        add_to_free_list(pool, pool->block_count);
        
        pool->block_count++;
    }
    
    // Remove block from free list and mark as allocated
    remove_from_free_list(pool, block_index);
    block->is_free = false;
    block->allocation_id = ++g_memory_manager->next_allocation_id;
    
    if (name) {
        strncpy(block->debug_name, name, sizeof(block->debug_name) - 1);
    } else {
        snprintf(block->debug_name, sizeof(block->debug_name), 
                "alloc_%u_%s", block->allocation_id, pool_type_to_string(type));
    }
    
    // Update statistics
    pool->used_size += aligned_size;
    pool->allocation_count++;
    pool->total_allocated += aligned_size;
    
    if (pool->used_size > pool->peak_usage) {
        pool->peak_usage = pool->used_size;
    }
    
    g_memory_manager->stats.used_memory += aligned_size;
    g_memory_manager->stats.total_allocations++;
    
    if (g_memory_manager->stats.used_memory > g_memory_manager->stats.peak_usage) {
        g_memory_manager->stats.peak_usage = g_memory_manager->stats.used_memory;
    }
    
    // Setup allocation handle
    allocation.pool_type = type;
    allocation.offset = block->offset;
    allocation.size = aligned_size;
    allocation.allocation_id = block->allocation_id;
    
#ifdef __APPLE__
    allocation.metal_buffer = pool->metal_buffer;
    allocation.cpu_pointer = pool->cpu_pointer ? 
                           (uint8_t*)pool->cpu_pointer + block->offset : NULL;
#endif
    
    LOG_DEBUG(LOG_CAT_MEMORY, "Allocated %llu bytes from pool '%s': %s",
              aligned_size, pool->name, block->debug_name);
    
    return allocation;
}

void gpu_memory_free(GPUMemoryAllocation* allocation) {
    if (!g_memory_manager || !allocation || allocation->allocation_id == 0) {
        return;
    }
    
    GPUMemoryPool* pool = find_pool(allocation->pool_type);
    if (!pool) {
        LOG_ERROR(LOG_CAT_MEMORY, "Invalid pool type in allocation");
        return;
    }
    
    // Find the block for this allocation
    uint32_t block_index = UINT32_MAX;
    for (uint32_t i = 0; i < pool->block_count; i++) {
        if (pool->blocks[i].allocation_id == allocation->allocation_id) {
            block_index = i;
            break;
        }
    }
    
    if (block_index == UINT32_MAX) {
        LOG_ERROR(LOG_CAT_MEMORY, "Allocation %u not found in pool '%s'",
                  allocation->allocation_id, pool->name);
        return;
    }
    
    MemoryBlock* block = &pool->blocks[block_index];
    
    // Mark as free
    block->is_free = true;
    block->is_dirty = true;
    block->allocation_id = 0;
    block->debug_name[0] = '\0';
    
    // Add to free list
    add_to_free_list(pool, block_index);
    
    // Update statistics
    pool->used_size -= block->size;
    pool->free_count++;
    pool->total_allocated -= block->size;
    
    g_memory_manager->stats.used_memory -= block->size;
    g_memory_manager->stats.total_frees++;
    
    // Try to merge with adjacent free blocks
    merge_adjacent_free_blocks(pool);
    
    // Update fragmentation ratio
    pool->fragmentation_ratio = calculate_fragmentation_ratio(pool);
    if (pool->fragmentation_ratio > DEFRAGMENTATION_THRESHOLD) {
        pool->needs_defragmentation = true;
    }
    
    LOG_DEBUG(LOG_CAT_MEMORY, "Freed %llu bytes from pool '%s'", block->size, pool->name);
    
    // Clear allocation handle
    memset(allocation, 0, sizeof(GPUMemoryAllocation));
}

void* gpu_memory_get_cpu_pointer(GPUMemoryAllocation* allocation) {
    if (!allocation) return NULL;
    
#ifdef __APPLE__
    return allocation->cpu_pointer;
#else
    return NULL;
#endif
}

// ============================================================================
// Defragmentation
// ============================================================================

static bool plan_defragmentation(GPUMemoryPool* pool) {
    if (!pool->needs_defragmentation) {
        return true;
    }
    
    // Simple defragmentation: move all allocated blocks to the beginning
    uint64_t current_offset = 0;
    
    for (uint32_t i = 0; i < pool->block_count; i++) {
        MemoryBlock* block = &pool->blocks[i];
        
        if (!block->is_free) {
            if (block->offset != current_offset) {
                block->new_offset = current_offset;
                block->needs_move = true;
            }
            current_offset += block->size;
        }
    }
    
    return true;
}

static bool execute_defragmentation(GPUMemoryPool* pool) {
    bool any_moves = false;
    
    for (uint32_t i = 0; i < pool->block_count; i++) {
        MemoryBlock* block = &pool->blocks[i];
        
        if (!block->is_free && block->needs_move) {
#ifdef __APPLE__
            // Copy data to new location
            uint8_t* src = (uint8_t*)pool->cpu_pointer + block->offset;
            uint8_t* dst = (uint8_t*)pool->cpu_pointer + block->new_offset;
            
            memmove(dst, src, block->size);
#endif
            
            block->offset = block->new_offset;
            block->needs_move = false;
            any_moves = true;
        }
    }
    
    if (any_moves) {
        // Rebuild free list after moves
        pool->free_list_head = UINT32_MAX;
        
        for (uint32_t i = 0; i < pool->block_count; i++) {
            if (pool->blocks[i].is_free) {
                add_to_free_list(pool, i);
            }
        }
        
        // Merge adjacent free blocks
        merge_adjacent_free_blocks(pool);
        
        pool->fragmentation_ratio = calculate_fragmentation_ratio(pool);
        pool->needs_defragmentation = false;
        
        LOG_INFO(LOG_CAT_MEMORY, "Defragmented pool '%s': fragmentation %.2f%%",
                 pool->name, pool->fragmentation_ratio * 100.0f);
    }
    
    return true;
}

void gpu_memory_defragment(void) {
    if (!g_memory_manager || !g_memory_manager->defrag.enabled) {
        return;
    }
    
    uint64_t defrag_start = gpu_memory_get_timestamp();
    uint32_t successful_defrags = 0;
    uint32_t failed_defrags = 0;
    
    for (uint32_t i = 0; i < g_memory_manager->pool_count; i++) {
        GPUMemoryPool* pool = &g_memory_manager->pools[i];
        
        if (pool->needs_defragmentation) {
            // Validate pool integrity before defragmentation
            if (g_memory_manager->validation_enabled && !gpu_memory_validate_pool_integrity(pool)) {
                gpu_memory_log_validation_error("Pool integrity validation failed before defragmentation");
                failed_defrags++;
                continue;
            }
            
            if (plan_defragmentation(pool) && execute_defragmentation(pool)) {
                successful_defrags++;
                
                // Validate pool integrity after defragmentation
                if (g_memory_manager->validation_enabled && !gpu_memory_validate_pool_integrity(pool)) {
                    gpu_memory_log_validation_error("Pool integrity validation failed after defragmentation");
                    failed_defrags++;
                }
            } else {
                failed_defrags++;
            }
        }
    }
    
    g_memory_manager->total_defragmentations += successful_defrags;
    g_memory_manager->failed_defragmentations += failed_defrags;
    
    uint64_t defrag_end = gpu_memory_get_timestamp();
    uint64_t defrag_time = defrag_end - defrag_start;
    
    if (successful_defrags > 0 || failed_defrags > 0) {
        LOG_INFO(LOG_CAT_MEMORY, "Defragmentation completed: %u successful, %u failed (%.2f ms)",
                 successful_defrags, failed_defrags, defrag_time / 1000000.0);
    }
}

void gpu_memory_force_defragmentation(void) {
    if (!g_memory_manager) return;
    
    // Mark all pools as needing defragmentation
    for (uint32_t i = 0; i < g_memory_manager->pool_count; i++) {
        g_memory_manager->pools[i].needs_defragmentation = true;
    }
    
    gpu_memory_defragment();
}

// ============================================================================
// Statistics and Debugging
// ============================================================================

void gpu_memory_get_stats(GPUMemoryStats* out) {
    if (!g_memory_manager || !out) return;
    
    memset(out, 0, sizeof(GPUMemoryStats));
    
    out->total_memory = g_memory_manager->stats.total_memory;
    out->used_memory = g_memory_manager->stats.used_memory;
    out->free_memory = g_memory_manager->stats.total_memory - g_memory_manager->stats.used_memory;
    out->total_allocations = g_memory_manager->stats.total_allocations;
    out->total_frees = g_memory_manager->stats.total_frees;
    out->peak_usage = g_memory_manager->stats.peak_usage;
    
    // Calculate fragmentation
    float total_fragmentation = 0.0f;
    uint32_t pools_with_fragmentation = 0;
    
    for (uint32_t i = 0; i < g_memory_manager->pool_count; i++) {
        GPUMemoryPool* pool = &g_memory_manager->pools[i];
        total_fragmentation += pool->fragmentation_ratio;
        if (pool->fragmentation_ratio > 0.0f) {
            pools_with_fragmentation++;
        }
    }
    
    out->fragmentation_ratio = pools_with_fragmentation > 0 ? 
                               total_fragmentation / pools_with_fragmentation : 0.0f;
}

void gpu_memory_log_stats(void) {
    if (!g_memory_manager) return;
    
    GPUMemoryStats stats;
    gpu_memory_get_stats(&stats);
    
    LOG_INFO(LOG_CAT_MEMORY, "=== GPU Memory Statistics ===");
    LOG_INFO(LOG_CAT_MEMORY, "Total memory: %llu MB", stats.total_memory / (1024 * 1024));
    LOG_INFO(LOG_CAT_MEMORY, "Used memory: %llu MB (%.1f%%)",
             stats.used_memory / (1024 * 1024),
             (float)stats.used_memory / stats.total_memory * 100.0f);
    LOG_INFO(LOG_CAT_MEMORY, "Free memory: %llu MB", stats.free_memory / (1024 * 1024));
    LOG_INFO(LOG_CAT_MEMORY, "Total allocations: %u", stats.total_allocations);
    LOG_INFO(LOG_CAT_MEMORY, "Total frees: %u", stats.total_frees);
    LOG_INFO(LOG_CAT_MEMORY, "Peak usage: %llu MB", stats.peak_usage / (1024 * 1024));
    LOG_INFO(LOG_CAT_MEMORY, "Fragmentation: %.1f%%", stats.fragmentation_ratio * 100.0f);
    LOG_INFO(LOG_CAT_MEMORY, "================================");
    
    // Per-pool statistics
    for (uint32_t i = 0; i < g_memory_manager->pool_count; i++) {
        GPUMemoryPool* pool = &g_memory_manager->pools[i];
        LOG_INFO(LOG_CAT_MEMORY, "Pool '%s': %llu/%llu MB used, %u blocks, %.1f%% fragmented",
                 pool->name,
                 pool->used_size / (1024 * 1024),
                 pool->size / (1024 * 1024),
                 pool->block_count,
                 pool->fragmentation_ratio * 100.0f);
    }
}

bool gpu_memory_is_initialized(void) {
    return g_memory_manager && g_memory_manager->initialized;
}

// ============================================================================
// GPU Memory Validation Implementation
// ============================================================================

static bool gpu_memory_validate_allocation(GPUMemoryAllocation* allocation) {
    if (!allocation) return false;
    
    // Check allocation ID
    if (allocation->allocation_id == 0) {
        return false;
    }
    
    // Check pool type
    if (allocation->pool_type >= MEMORY_POOL_COUNT) {
        return false;
    }
    
    // Check size
    if (allocation->size == 0 || allocation->size > MAX_BLOCK_SIZE) {
        return false;
    }
    
    // Check alignment
    if (allocation->offset % MEMORY_ALIGNMENT != 0) {
        return false;
    }
    
    return true;
}

static void gpu_memory_log_validation_error(const char* error) {
    if (!error) return;
    
    g_memory_manager->validation_errors++;
    LOG_ERROR(LOG_CAT_MEMORY, "GPU Memory Validation Error [%lu]: %s",
             g_memory_manager->validation_errors, error);
}

static uint64_t gpu_memory_get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static bool gpu_memory_validate_pool_integrity(GPUMemoryPool* pool) {
    if (!pool) return false;
    
    // Check pool size
    if (pool->size == 0) return false;
    
    // Check used size doesn't exceed pool size
    if (pool->used_size > pool->size) {
        return false;
    }
    
    // Validate block consistency
    uint64_t total_block_size = 0;
    uint32_t allocated_blocks = 0;
    uint32_t free_blocks = 0;
    
    for (uint32_t i = 0; i < pool->block_count; i++) {
        MemoryBlock* block = &pool->blocks[i];
        
        if (block->is_free) {
            free_blocks++;
        } else {
            allocated_blocks++;
        }
        
        total_block_size += block->size;
        
        // Check block offset and size
        if (block->offset >= pool->size || block->size == 0) {
            return false;
        }
        
        // Check for overlapping blocks
        for (uint32_t j = i + 1; j < pool->block_count; j++) {
            MemoryBlock* other = &pool->blocks[j];
            if (block->offset < other->offset + other->size &&
                block->offset + block->size > other->offset) {
                return false; // Overlapping blocks
            }
        }
    }
    
    // Check if total block size matches pool size
    if (total_block_size != pool->size) {
        return false;
    }
    
    // Validate free list consistency
    uint32_t free_list_count = 0;
    uint32_t current = pool->free_list_head;
    
    while (current != UINT32_MAX) {
        if (current >= pool->block_count) {
            return false; // Invalid block index
        }
        
        MemoryBlock* block = &pool->blocks[current];
        if (!block->is_free) {
            return false; // Non-free block in free list
        }
        
        free_list_count++;
        current = block->next_block;
        
        // Prevent infinite loop
        if (free_list_count > pool->block_count) {
            return false;
        }
    }
    
    if (free_list_count != free_blocks) {
        return false; // Free list count mismatch
    }
    
    return true;
}

static bool gpu_memory_detect_memory_leaks(void) {
    if (!g_memory_manager) return false;
    
    bool leaks_detected = false;
    
    for (uint32_t i = 0; i < g_memory_manager->pool_count; i++) {
        GPUMemoryPool* pool = &g_memory_manager->pools[i];
        
        // Check for allocated blocks that should be freed
        for (uint32_t j = 0; j < pool->block_count; j++) {
            MemoryBlock* block = &pool->blocks[j];
            
            if (!block->is_free && block->allocation_id > 0) {
                // This block is allocated - check if it's a potential leak
                // In a real implementation, you'd track allocation lifetimes
                LOG_WARN(LOG_CAT_MEMORY, "Potential memory leak in pool '%s': %s (%llu bytes)",
                         pool->name, block->debug_name, block->size);
                leaks_detected = true;
                g_memory_manager->memory_leaks_detected++;
            }
        }
    }
    
    return leaks_detected;
}

// ============================================================================
// Public Validation API
// ============================================================================

void gpu_memory_enable_validation(bool enabled) {
    if (!g_memory_manager) return;
    
    g_memory_manager->validation_enabled = enabled;
    g_memory_manager->last_validation_time = gpu_memory_get_timestamp();
    LOG_INFO(LOG_CAT_MEMORY, "GPU memory validation %s", enabled ? "enabled" : "disabled");
}

bool gpu_memory_validate_state(void) {
    if (!g_memory_manager || !g_memory_manager->validation_enabled) {
        return true;
    }
    
    bool valid = true;
    
    // Validate all pools
    for (uint32_t i = 0; i < g_memory_manager->pool_count; i++) {
        if (!gpu_memory_validate_pool_integrity(&g_memory_manager->pools[i])) {
            gpu_memory_log_validation_error("Pool integrity validation failed");
            valid = false;
        }
    }
    
    // Check for excessive validation errors
    if (g_memory_manager->validation_errors > 100) {
        gpu_memory_log_validation_error("Excessive validation errors detected");
        valid = false;
    }
    
    // Check defragmentation failure rate
    if (g_memory_manager->total_defragmentations > 0) {
        double failure_rate = (double)g_memory_manager->failed_defragmentations / 
                            g_memory_manager->total_defragmentations;
        if (failure_rate > 0.5) { // More than 50% failure rate
            gpu_memory_log_validation_error("High defragmentation failure rate detected");
            valid = false;
        }
    }
    
    g_memory_manager->last_validation_time = gpu_memory_get_timestamp();
    return valid;
}

uint64_t gpu_memory_get_validation_errors(void) {
    return g_memory_manager ? g_memory_manager->validation_errors : 0;
}

void gpu_memory_get_validation_statistics(uint64_t* validation_errors,
                                         uint64_t* total_defragmentations,
                                         uint64_t* failed_defragmentations,
                                         uint64_t* memory_leaks_detected) {
    if (!g_memory_manager) return;
    
    if (validation_errors) *validation_errors = g_memory_manager->validation_errors;
    if (total_defragmentations) *total_defragmentations = g_memory_manager->total_defragmentations;
    if (failed_defragmentations) *failed_defragmentations = g_memory_manager->failed_defragmentations;
    if (memory_leaks_detected) *memory_leaks_detected = g_memory_manager->memory_leaks_detected;
}

void gpu_memory_reset_validation_statistics(void) {
    if (!g_memory_manager) return;
    
    g_memory_manager->validation_errors = 0;
    g_memory_manager->total_defragmentations = 0;
    g_memory_manager->failed_defragmentations = 0;
    g_memory_manager->memory_leaks_detected = 0;
    g_memory_manager->last_validation_time = gpu_memory_get_timestamp();
    
    LOG_INFO(LOG_CAT_MEMORY, "GPU memory validation statistics reset");
}

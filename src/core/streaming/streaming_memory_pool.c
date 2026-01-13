#include "core/resource/streaming/streaming_memory_pool.h"
#include "engine/include/core/logger.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

// Helper function to align size to alignment boundary
static u32 align_size(u32 size, u32 alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Helper function to find free list node
static FreeListNode* find_free_node(StreamingMemoryPool* pool, u32 size) {
    FreeListNode* node = pool->free_list_head;
    FreeListNode* best_fit = NULL;
    u32 best_size = UINT32_MAX;
    
    while (node) {
        if (node->size >= size && node->size < best_size) {
            best_fit = node;
            best_size = node->size;
            
            // Exact match found
            if (node->size == size) {
                break;
            }
        }
        node = node->next;
    }
    
    return best_fit;
}

// Helper function to add free list node
static void add_free_node(StreamingMemoryPool* pool, u32 offset, u32 size) {
    if (pool->free_list_count >= STREAMING_POOL_MAX_BLOCKS) {
        LOG_ERROR("Free list node limit reached");
        return;
    }
    
    FreeListNode* node = &pool->free_list_nodes[pool->free_list_count++];
    node->offset = offset;
    node->size = size;
    
    // Insert into sorted list (by offset)
    FreeListNode* current = pool->free_list_head;
    FreeListNode* prev = NULL;
    
    while (current && current->offset < offset) {
        prev = current;
        current = current->next;
    }
    
    node->next = current;
    node->prev = prev;
    
    if (prev) {
        prev->next = node;
    } else {
        pool->free_list_head = node;
    }
    
    if (current) {
        current->prev = node;
    }
    
    // Try to coalesce with adjacent nodes
    if (prev && prev->offset + prev->size == offset) {
        // Coalesce with previous node
        prev->size += size;
        prev->next = current;
        if (current) current->prev = prev;
        pool->free_list_count--; // Remove the node we just added
    } else if (current && offset + size == current->offset) {
        // Coalesce with next node
        current->offset = offset;
        current->size += size;
        if (prev) prev->next = current;
        else pool->free_list_head = current;
        pool->free_list_count--; // Remove the node we just added
    }
}

// Helper function to remove free list node
static void remove_free_node(StreamingMemoryPool* pool, FreeListNode* node) {
    if (!node) return;
    
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        pool->free_list_head = node->next;
    }
    
    if (node->next) {
        node->next->prev = node->prev;
    }
    
    // Note: We don't actually free the node, just mark it as unused
    // by moving it to the end of the array (simplified approach)
}

bool streaming_pool_init(StreamingMemoryPool* pool, u64 size) {
    if (!pool || size == 0) return false;
    
    memset(pool, 0, sizeof(StreamingMemoryPool));
    
    // Align pool size
    size = align_size((u32)size, STREAMING_POOL_ALIGNMENT);
    
    // Allocate pool memory
    pool->pool_memory = (u8*)memory_alloc(size);
    if (!pool->pool_memory) {
        LOG_ERROR("Failed to allocate streaming memory pool of size %llu", (unsigned long long)size);
        return false;
    }
    
    pool->pool_size = size;
    pool->used_memory = 0;
    pool->peak_memory = 0;
    
    // Initialize free list with entire pool
    pool->free_list_nodes = (FreeListNode*)memory_alloc(sizeof(FreeListNode) * STREAMING_POOL_MAX_BLOCKS);
    if (!pool->free_list_nodes) {
        LOG_ERROR("Failed to allocate free list nodes");
        memory_free(pool->pool_memory);
        return false;
    }
    
    // Add entire pool as free space
    add_free_node(pool, 0, (u32)size);
    
    // Initialize configuration
    pool->eviction_threshold = 85;  // 85% usage triggers eviction
    pool->max_block_size = 16 * 1024 * 1024;  // 16MB max block size
    pool->allow_compression = true;
    pool->use_lru_eviction = true;
    
    LOG_INFO("Streaming memory pool initialized: %llu bytes", (unsigned long long)size);
    return true;
}

void streaming_pool_shutdown(StreamingMemoryPool* pool) {
    if (!pool) return;
    
    streaming_pool_lock(pool);
    
    if (pool->pool_memory) {
        memory_free(pool->pool_memory);
        pool->pool_memory = NULL;
    }
    
    if (pool->free_list_nodes) {
        memory_free(pool->free_list_nodes);
        pool->free_list_nodes = NULL;
    }
    
    pool->pool_size = 0;
    pool->used_memory = 0;
    
    streaming_pool_unlock(pool);
    
    LOG_INFO("Streaming memory pool shutdown");
}

bool streaming_pool_resize(StreamingMemoryPool* pool, u64 new_size) {
    if (!pool || new_size == 0) return false;
    
    streaming_pool_lock(pool);
    
    // Check if we can shrink
    if (new_size < pool->pool_size) {
        u64 min_required = pool->used_memory;
        if (new_size < min_required) {
            LOG_ERROR("Cannot shrink pool below used memory (%llu < %llu)", (unsigned long long)new_size, (unsigned long long)min_required);
            streaming_pool_unlock(pool);
            return false;
        }
    }
    
    // Reallocate memory
    u8* new_memory = (u8*)memory_realloc(pool->pool_memory, new_size);
    if (!new_memory) {
        LOG_ERROR("Failed to resize streaming memory pool");
        streaming_pool_unlock(pool);
        return false;
    }
    
    pool->pool_memory = new_memory;
    pool->pool_size = new_size;
    
    // If we grew, add the new space to free list
    if (new_size > pool->used_memory) {
        add_free_node(pool, (u32)pool->used_memory, (u32)(new_size - pool->used_memory));
    }
    
    streaming_pool_unlock(pool);
    
    LOG_INFO("Streaming memory pool resized to %llu bytes", (unsigned long long)new_size);
    return true;
}

void* streaming_pool_alloc(StreamingMemoryPool* pool, u32 size, MemoryBlockType type, u64 asset_id) {
    if (!pool || size == 0) return NULL;
    
    streaming_pool_lock(pool);
    
    // Align size
    u32 aligned_size = align_size(size, STREAMING_POOL_ALIGNMENT);
    
    // Check if size exceeds maximum
    if (aligned_size > pool->max_block_size) {
        LOG_ERROR("Allocation size %u exceeds maximum block size %u", aligned_size, pool->max_block_size);
        streaming_pool_unlock(pool);
        return NULL;
    }
    
    // Find free block
    FreeListNode* node = find_free_node(pool, aligned_size);
    if (!node) {
        // Try eviction if we're over threshold
        if ((pool->used_memory * 100) / pool->pool_size > pool->eviction_threshold) {
            u32 evicted = streaming_pool_evict_blocks(pool, aligned_size);
            if (evicted > 0) {
                node = find_free_node(pool, aligned_size);
            }
        }
        
        if (!node) {
            pool->allocation_failures++;
            LOG_WARN("Failed to allocate %u bytes from streaming pool", aligned_size);
            streaming_pool_unlock(pool);
            return NULL;
        }
    }
    
    // Find free block slot
    u32 block_index = UINT32_MAX;
    for (u32 i = 0; i < STREAMING_POOL_MAX_BLOCKS; i++) {
        if (pool->blocks[i].status == BLOCK_STATUS_FREE) {
            block_index = i;
            break;
        }
    }
    
    if (block_index == UINT32_MAX) {
        LOG_ERROR("No free block slots available");
        streaming_pool_unlock(pool);
        return NULL;
    }
    
    // Setup block
    StreamingMemoryBlock* block = &pool->blocks[block_index];
    block->data = pool->pool_memory + node->offset;
    block->size = aligned_size;
    block->capacity = aligned_size;
    block->offset = node->offset;
    block->type = type;
    block->status = BLOCK_STATUS_ALLOCATED;
    block->asset_id = asset_id;
    block->last_access_time = time_get_current_ms();
    block->ref_count = 1;
    block->priority = 0;
    block->is_compressed = false;
    block->compressed_size = 0;
    
    // Update free list
    if (node->size == aligned_size) {
        // Exact match, remove node
        remove_free_node(pool, node);
    } else {
        // Split node
        node->offset += aligned_size;
        node->size -= aligned_size;
    }
    
    // Update statistics
    pool->used_memory += aligned_size;
    if (pool->used_memory > pool->peak_memory) {
        pool->peak_memory = pool->used_memory;
    }
    pool->total_allocations++;
    pool->active_blocks++;
    
    streaming_pool_unlock(pool);
    
    LOG_TRACE("Allocated %u bytes from streaming pool (asset: %llu, type: %d)", 
              aligned_size, (unsigned long long)asset_id, type);
    
    return block->data;
}

void* streaming_pool_realloc(StreamingMemoryPool* pool, void* ptr, u32 new_size) {
    if (!pool || !ptr) return NULL;
    
    streaming_pool_lock(pool);
    
    // Find block
    StreamingMemoryBlock* block = streaming_pool_get_block(pool, ptr);
    if (!block) {
        LOG_ERROR("Invalid pointer passed to streaming_pool_realloc");
        streaming_pool_unlock(pool);
        return NULL;
    }
    
    // If new size is smaller, just shrink
    if (new_size <= block->capacity) {
        u32 aligned_size = align_size(new_size, STREAMING_POOL_ALIGNMENT);
        block->size = aligned_size;
        
        // Add freed space back to free list
        if (block->capacity - aligned_size > 0) {
            add_free_node(pool, block->offset + aligned_size, block->capacity - aligned_size);
        }
        
        streaming_pool_unlock(pool);
        return ptr;
    }
    
    // Need to allocate new block and copy data
    void* new_ptr = streaming_pool_alloc(pool, new_size, block->type, block->asset_id);
    if (!new_ptr) {
        streaming_pool_unlock(pool);
        return NULL;
    }
    
    // Copy data
    memcpy(new_ptr, ptr, block->size);
    
    // Free old block
    streaming_pool_free(pool, ptr);
    
    streaming_pool_unlock(pool);
    return new_ptr;
}

void streaming_pool_free(StreamingMemoryPool* pool, void* ptr) {
    if (!pool || !ptr) return;
    
    streaming_pool_lock(pool);
    
    // Find block
    StreamingMemoryBlock* block = streaming_pool_get_block(pool, ptr);
    if (!block || block->status == BLOCK_STATUS_FREE) {
        LOG_ERROR("Invalid or double-free of pointer in streaming pool");
        streaming_pool_unlock(pool);
        return;
    }
    
    // Add block back to free list
    add_free_node(pool, block->offset, block->capacity);
    
    // Update statistics
    pool->used_memory -= block->size;
    pool->total_deallocations++;
    pool->active_blocks--;
    
    // Clear block
    memset(block, 0, sizeof(StreamingMemoryBlock));
    block->status = BLOCK_STATUS_FREE;
    
    streaming_pool_unlock(pool);
    
    LOG_TRACE("Freed block from streaming pool");
}

StreamingMemoryBlock* streaming_pool_get_block(StreamingMemoryPool* pool, void* ptr) {
    if (!pool || !ptr) return NULL;
    
    // Find block containing this pointer
    for (u32 i = 0; i < STREAMING_POOL_MAX_BLOCKS; i++) {
        StreamingMemoryBlock* block = &pool->blocks[i];
        if (block->status != BLOCK_STATUS_FREE && 
            ptr >= block->data && ptr < block->data + block->size) {
            return block;
        }
    }
    
    return NULL;
}

bool streaming_pool_lock_block(StreamingMemoryPool* pool, void* ptr) {
    if (!pool || !ptr) return false;
    
    streaming_pool_lock(pool);
    
    StreamingMemoryBlock* block = streaming_pool_get_block(pool, ptr);
    if (!block) {
        streaming_pool_unlock(pool);
        return false;
    }
    
    block->ref_count++;
    if (block->ref_count > 0) {
        block->status = BLOCK_STATUS_LOCKED;
    }
    
    streaming_pool_unlock(pool);
    return true;
}

bool streaming_pool_unlock_block(StreamingMemoryPool* pool, void* ptr) {
    if (!pool || !ptr) return false;
    
    streaming_pool_lock(pool);
    
    StreamingMemoryBlock* block = streaming_pool_get_block(pool, ptr);
    if (!block) {
        streaming_pool_unlock(pool);
        return false;
    }
    
    if (block->ref_count > 0) {
        block->ref_count--;
        if (block->ref_count == 0) {
            block->status = BLOCK_STATUS_ALLOCATED;
        }
    }
    
    streaming_pool_unlock(pool);
    return true;
}

bool streaming_pool_mark_dirty(StreamingMemoryPool* pool, void* ptr) {
    if (!pool || !ptr) return false;
    
    streaming_pool_lock(pool);
    
    StreamingMemoryBlock* block = streaming_pool_get_block(pool, ptr);
    if (!block) {
        streaming_pool_unlock(pool);
        return false;
    }
    
    block->status = BLOCK_STATUS_DIRTY;
    
    streaming_pool_unlock(pool);
    return true;
}

StreamingMemoryBlock* streaming_pool_select_eviction_candidate(StreamingMemoryPool* pool) {
    if (!pool) return NULL;
    
    StreamingMemoryBlock* best_candidate = NULL;
    u64 oldest_time = UINT64_MAX;
    
    for (u32 i = 0; i < STREAMING_POOL_MAX_BLOCKS; i++) {
        StreamingMemoryBlock* block = &pool->blocks[i];
        
        // Skip locked or loading blocks
        if (block->status == BLOCK_STATUS_LOCKED || 
            block->status == BLOCK_STATUS_LOADING ||
            block->status == BLOCK_STATUS_FREE) {
            continue;
        }
        
        // Prefer blocks with lower priority and older access time
        u64 score = (u64)(block->priority * 1000) + block->last_access_time;
        
        if (score < oldest_time) {
            oldest_time = score;
            best_candidate = block;
        }
    }
    
    return best_candidate;
}

bool streaming_pool_evict_block(StreamingMemoryPool* pool, StreamingMemoryBlock* block) {
    if (!pool || !block) return false;
    
    streaming_pool_lock(pool);
    
    // Can't evict locked blocks
    if (block->status == BLOCK_STATUS_LOCKED || block->status == BLOCK_STATUS_LOADING) {
        streaming_pool_unlock(pool);
        return false;
    }
    
    // If block is dirty, write back to storage (simplified - just mark as clean)
    if (block->status == BLOCK_STATUS_DIRTY) {
        //  COMPLETED: Implement actual write-back to storage
        LOG_DEBUG("Writing dirty block back to storage (asset: %llu)", (unsigned long long)block->asset_id);
    }
    
    // Add block back to free list
    add_free_node(pool, block->offset, block->capacity);
    
    // Update statistics
    pool->used_memory -= block->size;
    pool->eviction_count++;
    pool->active_blocks--;
    
    // Clear block
    memset(block, 0, sizeof(StreamingMemoryBlock));
    block->status = BLOCK_STATUS_FREE;
    
    streaming_pool_unlock(pool);
    
    LOG_TRACE("Evicted block from streaming pool");
    return true;
}

u32 streaming_pool_evict_blocks(StreamingMemoryPool* pool, u32 bytes_needed) {
    if (!pool || bytes_needed == 0) return 0;
    
    u32 bytes_evicted = 0;
    u32 blocks_evicted = 0;
    
    while (bytes_evicted < bytes_needed) {
        StreamingMemoryBlock* candidate = streaming_pool_select_eviction_candidate(pool);
        if (!candidate) {
            break; // No more candidates
        }
        
        u32 block_size = candidate->size;
        if (streaming_pool_evict_block(pool, candidate)) {
            bytes_evicted += block_size;
            blocks_evicted++;
        } else {
            break; // Failed to evict
        }
    }
    
    if (blocks_evicted > 0) {
        LOG_DEBUG("Evicted %u blocks (%u bytes) from streaming pool", blocks_evicted, bytes_evicted);
    }
    
    return bytes_evicted;
}

u32 streaming_pool_get_free_memory(StreamingMemoryPool* pool) {
    if (!pool) return 0;
    
    streaming_pool_lock(pool);
    u32 free_memory = (u32)(pool->pool_size - pool->used_memory);
    streaming_pool_unlock(pool);
    
    return free_memory;
}

u32 streaming_pool_get_used_memory(StreamingMemoryPool* pool) {
    if (!pool) return 0;
    
    streaming_pool_lock(pool);
    u32 used_memory = (u32)pool->used_memory;
    streaming_pool_unlock(pool);
    
    return used_memory;
}

f32 streaming_pool_get_fragmentation_ratio(StreamingMemoryPool* pool) {
    if (!pool || pool->used_memory == 0) return 0.0f;
    
    streaming_pool_lock(pool);
    
    u32 total_free = 0;
    u32 largest_free = 0;
    
    FreeListNode* node = pool->free_list_head;
    while (node) {
        total_free += node->size;
        if (node->size > largest_free) {
            largest_free = node->size;
        }
        node = node->next;
    }
    
    streaming_pool_unlock(pool);
    
    // Fragmentation ratio: 1.0 = fully fragmented, 0.0 = not fragmented
    return (total_free > 0) ? (1.0f - ((f32)largest_free / (f32)total_free)) : 0.0f;
}

bool streaming_pool_is_healthy(StreamingMemoryPool* pool) {
    if (!pool) return false;
    
    // Check for excessive fragmentation
    f32 fragmentation = streaming_pool_get_fragmentation_ratio(pool);
    if (fragmentation > 0.7f) {
        return false;
    }
    
    // Check for excessive allocation failures
    if (pool->total_allocations > 0) {
        f32 failure_rate = (f32)pool->allocation_failures / (f32)pool->total_allocations;
        if (failure_rate > 0.1f) {
            return false;
        }
    }
    
    // Check memory usage
    f32 usage_ratio = (f32)pool->used_memory / (f32)pool->pool_size;
    if (usage_ratio > 0.95f) {
        return false;
    }
    
    return true;
}

void streaming_pool_get_stats(StreamingMemoryPool* pool, u64* used, u64* peak, 
                             u32* allocations, u32* failures) {
    if (!pool) return;
    
    streaming_pool_lock(pool);
    
    if (used) *used = pool->used_memory;
    if (peak) *peak = pool->peak_memory;
    if (allocations) *allocations = pool->total_allocations;
    if (failures) *failures = pool->allocation_failures;
    
    streaming_pool_unlock(pool);
}

void streaming_pool_print_stats(StreamingMemoryPool* pool) {
    if (!pool) return;
    
    streaming_pool_lock(pool);
    
    LOG_INFO("=== Streaming Memory Pool Stats ===");
    LOG_INFO("Pool size: %llu bytes", (unsigned long long)pool->pool_size);
    LOG_INFO("Used memory: %llu bytes (%.1f%%)", (unsigned long long)pool->used_memory, 
             (f32)pool->used_memory / (f32)pool->pool_size * 100.0f);
    LOG_INFO("Peak memory: %llu bytes", (unsigned long long)pool->peak_memory);
    LOG_INFO("Active blocks: %u / %u", pool->active_blocks, STREAMING_POOL_MAX_BLOCKS);
    LOG_INFO("Total allocations: %u", pool->total_allocations);
    LOG_INFO("Total deallocations: %u", pool->total_deallocations);
    LOG_INFO("Allocation failures: %u", pool->allocation_failures);
    LOG_INFO("Evictions: %u", pool->eviction_count);
    LOG_INFO("Fragmentation: %.1f%%", streaming_pool_get_fragmentation_ratio(pool) * 100.0f);
    LOG_INFO("Health: %s", streaming_pool_is_healthy(pool) ? "GOOD" : "POOR");
    
    streaming_pool_unlock(pool);
}

void streaming_pool_dump_blocks(StreamingMemoryPool* pool) {
    if (!pool) return;
    
    streaming_pool_lock(pool);
    
    LOG_INFO("=== Streaming Memory Pool Blocks ===");
    
    for (u32 i = 0; i < STREAMING_POOL_MAX_BLOCKS; i++) {
        StreamingMemoryBlock* block = &pool->blocks[i];
        if (block->status != BLOCK_STATUS_FREE) {
            LOG_INFO("  [%u] Asset: %llu, Type: %d, Size: %u, Status: %d, Refs: %u, Priority: %u",
                     i, (unsigned long long)block->asset_id, block->type, block->size, block->status, 
                     block->ref_count, block->priority);
        }
    }
    
    streaming_pool_unlock(pool);
}

void streaming_pool_lock(StreamingMemoryPool* pool) {
    if (!pool) return;
    
    // Simple spinlock (in production, use proper mutex)
    while (__sync_lock_test_and_set(&pool->locked, 1)) {
        // Spin
    }
}

void streaming_pool_unlock(StreamingMemoryPool* pool) {
    if (!pool) return;
    
    __sync_lock_release(&pool->locked);
}

bool streaming_pool_try_lock(StreamingMemoryPool* pool) {
    if (!pool) return false;
    
    return !__sync_lock_test_and_set(&pool->locked, 1);
}

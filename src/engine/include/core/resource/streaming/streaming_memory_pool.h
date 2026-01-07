#ifndef STREAMING_MEMORY_POOL_H
#define STREAMING_MEMORY_POOL_H

#include "include/core/types.h"

#define STREAMING_POOL_DEFAULT_SIZE (64 * 1024 * 1024)  // 64MB default pool
#define STREAMING_POOL_MAX_BLOCKS 4096
#define STREAMING_POOL_ALIGNMENT 64                     // 64-byte alignment for SIMD

// Memory block types for streaming assets
typedef enum {
    MEMORY_BLOCK_TEXTURE = 0,
    MEMORY_BLOCK_MESH,
    MEMORY_BLOCK_AUDIO,
    MEMORY_BLOCK_ANIMATION,
    MEMORY_BLOCK_MATERIAL,
    MEMORY_BLOCK_SHADER,
    MEMORY_BLOCK_GENERIC,
    MEMORY_BLOCK_TYPE_COUNT
} MemoryBlockType;

// Memory block status
typedef enum {
    BLOCK_STATUS_FREE = 0,
    BLOCK_STATUS_ALLOCATED,
    BLOCK_STATUS_LOCKED,     // Cannot be evicted (currently in use)
    BLOCK_STATUS_LOADING,    // Asset is being loaded into this block
    BLOCK_STATUS_DIRTY       // Needs to be written back to storage
} MemoryBlockStatus;

// Individual memory block
typedef struct {
    u8* data;
    u32 size;
    u32 capacity;
    u32 offset;              // Offset from pool start
    MemoryBlockType type;
    MemoryBlockStatus status;
    u64 asset_id;            // Asset that owns this block
    u64 last_access_time;    // For LRU eviction
    u32 ref_count;           // Reference count for locking
    u32 priority;            // Streaming priority
    bool is_compressed;      // Data is compressed
    u32 compressed_size;     // Size when compressed
} StreamingMemoryBlock;

// Free list node for efficient allocation
typedef struct FreeListNode {
    u32 offset;
    u32 size;
    struct FreeListNode* next;
    struct FreeListNode* prev;
} FreeListNode;

// Memory pool for streaming assets
typedef struct {
    u8* pool_memory;         // Raw memory pool
    u64 pool_size;           // Total pool size
    u64 used_memory;         // Currently used memory
    u64 peak_memory;         // Peak memory usage
    
    // Block management
    StreamingMemoryBlock blocks[STREAMING_POOL_MAX_BLOCKS];
    u32 block_count;
    u32 active_blocks;
    
    // Free list for efficient allocation
    FreeListNode* free_list_head;
    FreeListNode* free_list_nodes;  // Pre-allocated nodes
    u32 free_list_count;
    
    // Statistics
    u32 total_allocations;
    u32 total_deallocations;
    u32 allocation_failures;
    u32 eviction_count;
    u64 total_allocation_time;
    
    // Configuration
    u32 eviction_threshold;  // Percentage usage that triggers eviction
    u32 max_block_size;      // Maximum block size
    bool allow_compression;  // Allow block compression
    bool use_lru_eviction;   // Use LRU for eviction
    
    // Thread safety (simple spinlock)
    volatile bool locked;
} StreamingMemoryPool;

// Pool lifecycle
bool streaming_pool_init(StreamingMemoryPool* pool, u64 size);
void streaming_pool_shutdown(StreamingMemoryPool* pool);
bool streaming_pool_resize(StreamingMemoryPool* pool, u64 new_size);

// Memory allocation
void* streaming_pool_alloc(StreamingMemoryPool* pool, u32 size, MemoryBlockType type, u64 asset_id);
void* streaming_pool_realloc(StreamingMemoryPool* pool, void* ptr, u32 new_size);
void streaming_pool_free(StreamingMemoryPool* pool, void* ptr);

// Block management
StreamingMemoryBlock* streaming_pool_get_block(StreamingMemoryPool* pool, void* ptr);
bool streaming_pool_lock_block(StreamingMemoryPool* pool, void* ptr);
bool streaming_pool_unlock_block(StreamingMemoryPool* pool, void* ptr);
bool streaming_pool_mark_dirty(StreamingMemoryPool* pool, void* ptr);

// Eviction system
StreamingMemoryBlock* streaming_pool_select_eviction_candidate(StreamingMemoryPool* pool);
bool streaming_pool_evict_block(StreamingMemoryPool* pool, StreamingMemoryBlock* block);
u32 streaming_pool_evict_blocks(StreamingMemoryPool* pool, u32 bytes_needed);

// Compression support
bool streaming_pool_compress_block(StreamingMemoryPool* pool, StreamingMemoryBlock* block);
bool streaming_pool_decompress_block(StreamingMemoryPool* pool, StreamingMemoryBlock* block);

// Utility functions
u32 streaming_pool_get_free_memory(StreamingMemoryPool* pool);
u32 streaming_pool_get_used_memory(StreamingMemoryPool* pool);
f32 streaming_pool_get_fragmentation_ratio(StreamingMemoryPool* pool);
bool streaming_pool_is_healthy(StreamingMemoryPool* pool);

// Statistics and debugging
void streaming_pool_get_stats(StreamingMemoryPool* pool, u64* used, u64* peak, 
                             u32* allocations, u32* failures);
void streaming_pool_print_stats(StreamingMemoryPool* pool);
void streaming_pool_dump_blocks(StreamingMemoryPool* pool);

// Thread safety
void streaming_pool_lock(StreamingMemoryPool* pool);
void streaming_pool_unlock(StreamingMemoryPool* pool);
bool streaming_pool_try_lock(StreamingMemoryPool* pool);

#endif // STREAMING_MEMORY_POOL_H

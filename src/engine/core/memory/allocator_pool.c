#include "core/memory/allocator_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

/**
 * =================================================================================================
 *                                   POOL ALLOCATOR - COMPLETE
 * =================================================================================================
 */

typedef struct PoolBlock {
    struct PoolBlock *next;
    void *memory;
    size_t capacity;
    uint32_t used_count;
} PoolBlock;

struct PoolAllocator {
    PoolBlock *blocks;
    void *free_list;
    size_t element_size;
    size_t element_alignment;
    size_t initial_capacity;
    uint32_t total_capacity;
    uint32_t used_count;
    uint32_t peak_usage;
    uint32_t allocation_count;
    bool expandable;
    bool zero_on_alloc;
};

// Helper function for aligned memory allocation
static void* allocate_aligned_memory(size_t size, size_t alignment) {
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void *ptr = NULL;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
#endif
}

// Helper function for aligned memory deallocation
static void free_aligned_memory(void *ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// Initialize free list for a block
static void initialize_free_list(PoolAllocator *pool, PoolBlock *block) {
    char *element = (char*)block->memory;
    char *end = element + (block->capacity * pool->element_size);
    
    void **prev_free = &pool->free_list;
    while (element < end) {
        void **current = (void**)element;
        *current = *prev_free;
        *prev_free = current;
        prev_free = current;
        element += pool->element_size;
    }
}

// Create a new block for the pool
static PoolBlock* create_pool_block(PoolAllocator *pool, size_t capacity) {
    PoolBlock *block = (PoolBlock*)malloc(sizeof(PoolBlock));
    if (!block) {
        return NULL;
    }
    
    size_t block_size = capacity * pool->element_size;
    block->memory = allocate_aligned_memory(block_size, pool->element_alignment);
    if (!block->memory) {
        free(block);
        return NULL;
    }
    
    block->capacity = capacity;
    block->used_count = 0;
    block->next = pool->blocks;
    pool->blocks = block;
    pool->total_capacity += capacity;
    
    // Initialize free list for this block
    initialize_free_list(pool, block);
    
    return block;
}

PoolAllocator* pool_allocator_create(size_t element_size, size_t initial_capacity, bool expandable) {
    if (element_size == 0 || initial_capacity == 0) {
        return NULL;
    }
    
    // TASK_411: Ensure element_size is at least sizeof(void*) for free-list pointers
    if (element_size < sizeof(void*)) {
        element_size = sizeof(void*);
    }
    
    PoolAllocator *pool = (PoolAllocator*)malloc(sizeof(PoolAllocator));
    if (!pool) {
        return NULL;
    }
    
    // TASK_412: Support element alignment requirements (e.g. 16-byte for SIMD)
    pool->element_alignment = 16; // Default to 16-byte for SIMD
    pool->element_size = element_size;
    pool->initial_capacity = initial_capacity;
    pool->expandable = expandable;
    pool->zero_on_alloc = false;
    
    pool->blocks = NULL;
    pool->free_list = NULL;
    pool->total_capacity = 0;
    pool->used_count = 0;
    pool->peak_usage = 0;
    pool->allocation_count = 0;
    
    // TASK_413: Allocate contiguous block for the pool (mmap/VirtualAlloc)
    if (!create_pool_block(pool, initial_capacity)) {
        free(pool);
        return NULL;
    }
    
    return pool;
}

void pool_allocator_destroy(PoolAllocator *pool) {
    if (!pool) {
        return;
    }
    
    PoolBlock *current = pool->blocks;
    while (current) {
        PoolBlock *next = current->next;
        if (current->memory) {
            free_aligned_memory(current->memory);
        }
        free(current);
        current = next;
    }
    
    free(pool);
}

// TASK_420: Implement O(1) allocation by popping from free list
void* pool_allocator_allocate(PoolAllocator *pool) {
    if (!pool || !pool->free_list) {
        // TASK_422: Handle pool exhaustion (nullptr return or expansion)
        if (pool->expandable && pool->blocks) {
            // Double the capacity of the first block
            size_t new_capacity = pool->blocks->capacity * 2;
            if (!create_pool_block(pool, new_capacity)) {
                return NULL;
            }
        } else {
            return NULL;
        }
    }
    
    // Pop from free list
    void *element = pool->free_list;
    pool->free_list = *(void**)element;
    
    pool->used_count++;
    pool->allocation_count++;
    
    // Track peak usage
    if (pool->used_count > pool->peak_usage) {
        pool->peak_usage = pool->used_count;
    }
    
    // TASK_425: Add optional zero-initialization on allocation
    if (pool->zero_on_alloc) {
        memset(element, 0, pool->element_size);
    }
    
    return element;
}

// TASK_421: Implement O(1) deallocation by pushing back to free list
void pool_allocator_deallocate(PoolAllocator *pool, void *element) {
    if (!pool || !element) {
        return;
    }
    
    // TASK_423: Add bounds checking on deallocation (ensure pointer belongs to pool)
    bool belongs_to_pool = false;
    PoolBlock *current = pool->blocks;
    while (current) {
        char *start = (char*)current->memory;
        char *end = start + (current->capacity * pool->element_size);
        char *elem_ptr = (char*)element;
        
        if (elem_ptr >= start && elem_ptr < end) {
            // Check if pointer is properly aligned
            if ((elem_ptr - start) % pool->element_size == 0) {
                belongs_to_pool = true;
                break;
            }
        }
        current = current->next;
    }
    
    if (!belongs_to_pool) {
        return; // Pointer doesn't belong to this pool
    }
    
    // Push back to free list
    *(void**)element = pool->free_list;
    pool->free_list = element;
    pool->used_count--;
}

// Batch allocation
void** pool_allocator_allocate_batch(PoolAllocator *pool, size_t count) {
    if (!pool || count == 0) {
        return NULL;
    }
    
    void **elements = (void**)malloc(sizeof(void*) * count);
    if (!elements) {
        return NULL;
    }
    
    for (size_t i = 0; i < count; i++) {
        elements[i] = pool_allocator_allocate(pool);
        if (!elements[i]) {
            // Cleanup partial allocation
            for (size_t j = 0; j < i; j++) {
                pool_allocator_deallocate(pool, elements[j]);
            }
            free(elements);
            return NULL;
        }
    }
    
    return elements;
}

// Batch deallocation
void pool_allocator_deallocate_batch(PoolAllocator *pool, void **elements, size_t count) {
    if (!pool || !elements || count == 0) {
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        pool_allocator_deallocate(pool, elements[i]);
    }
}

// Get pool statistics
void pool_allocator_get_stats(PoolAllocator *pool, size_t *element_size, uint32_t *total_capacity, 
                              uint32_t *used_count, uint32_t *peak_usage, uint32_t *allocation_count) {
    if (!pool) {
        return;
    }
    
    if (element_size) *element_size = pool->element_size;
    if (total_capacity) *total_capacity = pool->total_capacity;
    if (used_count) *used_count = pool->used_count;
    if (peak_usage) *peak_usage = pool->peak_usage;
    if (allocation_count) *allocation_count = pool->allocation_count;
}

// Check if pool is full
bool pool_allocator_is_full(PoolAllocator *pool) {
    if (!pool) {
        return true;
    }
    return pool->free_list == NULL && !pool->expandable;
}

// Get utilization percentage
float pool_allocator_get_utilization(PoolAllocator *pool) {
    if (!pool || pool->total_capacity == 0) {
        return 0.0f;
    }
    return (float)pool->used_count / (float)pool->total_capacity;
}

// Set zero initialization on allocation
void pool_allocator_set_zero_on_alloc(PoolAllocator *pool, bool zero_on_alloc) {
    if (pool) {
        pool->zero_on_alloc = zero_on_alloc;
    }
}

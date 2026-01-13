#include "core/memory/allocator_aligned.h"
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
 *                                 ALIGNED MEMORY ALLOCATOR - COMPLETE
 * =================================================================================================
 */

typedef struct AllocationHeader {
    struct AllocationHeader *prev;
    struct AllocationHeader *next;
    void *actual_ptr;  // Pointer to actual allocated memory
    size_t requested_size;
    size_t alignment;
    size_t actual_size;
} AllocationHeader;

struct AlignedAllocator {
    AllocationHeader *free_list;
    AllocationHeader *allocated_list;
    void *backing_allocator;
    size_t default_alignment;
    size_t total_allocated;
    uint32_t allocation_count;
    bool owns_backing_allocator;
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

// Helper function to check if alignment is power of 2
static bool is_power_of_2(size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

// Helper function to get next power of 2
static size_t next_power_of_2(size_t value) {
    if (value == 0) {
        return 1;
    }
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;  // For 64-bit values
    value++;
    return value;
}

// Calculate padding needed for alignment
static size_t calculate_padding(void *ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    size_t mask = alignment - 1;
    size_t padding = (alignment - (addr & mask)) & mask;
    return padding;
}

// Find header from user pointer
static AllocationHeader* get_header_from_pointer(void *user_ptr) {
    if (!user_ptr) {
        return NULL;
    }
    
    // Header is stored just before the user pointer
    uintptr_t header_addr = (uintptr_t)user_ptr - sizeof(AllocationHeader);
    return (AllocationHeader*)header_addr;
}

// Find user pointer from header
static void* get_user_pointer_from_header(AllocationHeader *header) {
    if (!header) {
        return NULL;
    }
    
    uintptr_t header_addr = (uintptr_t)header;
    uintptr_t user_ptr = header_addr + sizeof(AllocationHeader);
    
    // Add padding to achieve alignment
    size_t padding = calculate_padding((void*)user_ptr, header->alignment);
    user_ptr += padding;
    
    return (void*)user_ptr;
}

AlignedAllocator* aligned_allocator_create(void *backing_allocator, size_t default_alignment) {
    // Validate default alignment
    if (default_alignment == 0) {
        default_alignment = 16; // Default to 16-byte for SIMD
    }
    
    if (!is_power_of_2(default_alignment)) {
        default_alignment = next_power_of_2(default_alignment);
    }
    
    AlignedAllocator *allocator = (AlignedAllocator*)malloc(sizeof(AlignedAllocator));
    if (!allocator) {
        return NULL;
    }
    
    allocator->free_list = NULL;
    allocator->allocated_list = NULL;
    allocator->backing_allocator = backing_allocator;
    allocator->default_alignment = default_alignment;
    allocator->total_allocated = 0;
    allocator->allocation_count = 0;
    allocator->owns_backing_allocator = false;
    
    return allocator;
}

void aligned_allocator_destroy(AlignedAllocator *allocator) {
    if (!allocator) {
        return;
    }
    
    // Free all allocated memory
    AllocationHeader *current = allocator->allocated_list;
    while (current) {
        AllocationHeader *next = current->next;
        if (current->actual_ptr) {
            free_aligned_memory(current->actual_ptr);
        }
        current = next;
    }
    
    // Free free list
    current = allocator->free_list;
    while (current) {
        AllocationHeader *next = current->next;
        if (current->actual_ptr) {
            free_aligned_memory(current->actual_ptr);
        }
        current = next;
    }
    
    free(allocator);
}

void* aligned_allocator_allocate(AlignedAllocator *allocator, size_t size, size_t alignment) {
    if (!allocator || size == 0) {
        return NULL;
    }
    
    // Use default alignment if not specified
    if (alignment == 0) {
        alignment = allocator->default_alignment;
    }
    
    // Validate alignment
    if (!is_power_of_2(alignment)) {
        alignment = next_power_of_2(alignment);
    }
    
    // Calculate total size needed
    size_t header_size = sizeof(AllocationHeader);
    size_t total_size = header_size + size + alignment;
    
    // Try to find suitable block in free list
    AllocationHeader *block = allocator->free_list;
    AllocationHeader *prev_block = NULL;
    
    while (block) {
        if (block->alignment >= alignment && block->actual_size >= total_size) {
            // Found suitable block
            if (prev_block) {
                prev_block->next = block->next;
            } else {
                allocator->free_list = block->next;
            }
            
            // Update block info
            block->requested_size = size;
            block->alignment = alignment;
            
            // Add to allocated list
            block->prev = NULL;
            block->next = allocator->allocated_list;
            if (allocator->allocated_list) {
                allocator->allocated_list->prev = block;
            }
            allocator->allocated_list = block;
            
            allocator->allocation_count++;
            return get_user_pointer_from_header(block);
        }
        
        prev_block = block;
        block = block->next;
    }
    
    // No suitable block found, allocate new memory
    void *actual_ptr = allocate_aligned_memory(total_size, 16); // Header doesn't need special alignment
    if (!actual_ptr) {
        return NULL;
    }
    
    // Create header
    AllocationHeader *header = (AllocationHeader*)actual_ptr;
    header->actual_ptr = actual_ptr;
    header->requested_size = size;
    header->alignment = alignment;
    header->actual_size = total_size;
    
    // Add to allocated list
    header->prev = NULL;
    header->next = allocator->allocated_list;
    if (allocator->allocated_list) {
        allocator->allocated_list->prev = header;
    }
    allocator->allocated_list = header;
    
    allocator->total_allocated += total_size;
    allocator->allocation_count++;
    
    return get_user_pointer_from_header(header);
}

void aligned_allocator_deallocate(AlignedAllocator *allocator, void *ptr) {
    if (!allocator || !ptr) {
        return;
    }
    
    AllocationHeader *header = get_header_from_pointer(ptr);
    if (!header) {
        return; // Invalid pointer
    }
    
    // Remove from allocated list
    if (header->prev) {
        header->prev->next = header->next;
    } else {
        allocator->allocated_list = header->next;
    }
    
    if (header->next) {
        header->next->prev = header->prev;
    }
    
    // Add to free list
    header->prev = NULL;
    header->next = allocator->free_list;
    allocator->free_list = header;
    
    allocator->allocation_count--;
}

void* aligned_allocator_reallocate(AlignedAllocator *allocator, void *ptr, size_t size, size_t alignment) {
    if (!allocator) {
        return NULL;
    }
    
    if (!ptr) {
        return aligned_allocator_allocate(allocator, size, alignment);
    }
    
    if (size == 0) {
        aligned_allocator_deallocate(allocator, ptr);
        return NULL;
    }
    
    AllocationHeader *header = get_header_from_pointer(ptr);
    if (!header) {
        return NULL; // Invalid pointer
    }
    
    // If new size fits in current block, just update size
    size_t total_needed = sizeof(AllocationHeader) + size + (alignment ? alignment : allocator->default_alignment);
    if (total_needed <= header->actual_size) {
        header->requested_size = size;
        return ptr;
    }
    
    // Need to allocate new block and copy data
    void *new_ptr = aligned_allocator_allocate(allocator, size, alignment);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy old data
    size_t copy_size = header->requested_size < size ? header->requested_size : size;
    memcpy(new_ptr, ptr, copy_size);
    
    // Free old block
    aligned_allocator_deallocate(allocator, ptr);
    
    return new_ptr;
}

void aligned_allocator_get_stats(AlignedAllocator *allocator, size_t *total_allocated, 
                                uint32_t *allocation_count, size_t *default_alignment) {
    if (!allocator) {
        return;
    }
    
    if (total_allocated) *total_allocated = allocator->total_allocated;
    if (allocation_count) *allocation_count = allocator->allocation_count;
    if (default_alignment) *default_alignment = allocator->default_alignment;
}

bool aligned_allocator_is_valid_alignment(size_t alignment) {
    return is_power_of_2(alignment) && alignment > 0;
}

size_t aligned_allocator_get_next_power_of_2(size_t size) {
    return next_power_of_2(size);
}

size_t aligned_allocator_round_up(size_t size, size_t alignment) {
    if (alignment == 0) {
        return size;
    }
    
    if (!is_power_of_2(alignment)) {
        alignment = next_power_of_2(alignment);
    }
    
    return (size + alignment - 1) & ~(alignment - 1);
}

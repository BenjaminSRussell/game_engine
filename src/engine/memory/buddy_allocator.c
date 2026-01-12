// buddy_allocator.c - Stub implementation
#include "memory/buddy_allocator.h"
#include "core/logger.h"
#include <stdlib.h>

BuddyAllocator* buddy_allocator_create(size_t size, size_t min_block_size) {
    LOG_INFO("Creating buddy allocator (stub)");
    (void)size; (void)min_block_size;
    BuddyAllocator* allocator = (BuddyAllocator*)malloc(sizeof(BuddyAllocator));
    return allocator;
}

void buddy_allocator_destroy(BuddyAllocator* allocator) {
    if (allocator) {
        LOG_INFO("Destroying buddy allocator (stub)");
        free(allocator);
    }
}

void* buddy_allocator_alloc(BuddyAllocator* allocator, size_t size) {
    (void)allocator; (void)size;
    return malloc(size); // Stub - use malloc
}

void buddy_allocator_free(BuddyAllocator* allocator, void* ptr) {
    (void)allocator;
    free(ptr); // Stub - use free
}

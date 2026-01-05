#include <catch2/catch_test_macros.hpp>
#include <core/memory.h>

// Test Buddy Allocator implementation
TEST_CASE("Buddy Allocator", "[memory][buddy]") {
    
    SECTION("Creation with valid parameters") {
        BuddyAllocator* alloc = buddy_allocator_create(64, 1024);
        REQUIRE(alloc != nullptr);
        REQUIRE(alloc->min_block_size == 64);
        REQUIRE(alloc->max_block_size == 1024);
        REQUIRE(alloc->pool_size == 1024);
        
        buddy_allocator_destroy(alloc);
    }
    
    SECTION("Power-of-2 validation") {
        // Invalid: not power of 2
        BuddyAllocator* alloc1 = buddy_allocator_create(63, 1024);
        REQUIRE(alloc1 == nullptr);
        
        // Invalid: min > max
        BuddyAllocator* alloc2 = buddy_allocator_create(1024, 512);
        REQUIRE(alloc2 == nullptr);
    }
    
    SECTION("Basic allocation") {
        BuddyAllocator* alloc = buddy_allocator_create(64, 1024);
        REQUIRE(alloc != nullptr);
        
        void* ptr1 = buddy_allocator_alloc(alloc, 64);
        REQUIRE(ptr1 != nullptr);
        
        void* ptr2 = buddy_allocator_alloc(alloc, 128);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(ptr2 != ptr1);
        
        buddy_allocator_destroy(alloc);
    }
    
    SECTION("Block splitting") {
        BuddyAllocator* alloc = buddy_allocator_create(64, 512);
        
        // Allocate small block (should split larger blocks)
        void* ptr1 = buddy_allocator_alloc(alloc, 64);
        REQUIRE(ptr1 != nullptr);
        
        // Allocate another small block (should use split buddy)
        void* ptr2 = buddy_allocator_alloc(alloc, 64);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(ptr2 != ptr1);
        
        buddy_allocator_destroy(alloc);
    }
    
    SECTION("Allocation and freeing") {
        BuddyAllocator* alloc = buddy_allocator_create(64, 512);
        
        u32 initial_free = alloc->free_blocks;
        
        void* ptr = buddy_allocator_alloc(alloc, 128);
        REQUIRE(ptr != nullptr);
        REQUIRE(alloc->free_blocks < initial_free);
        
        buddy_allocator_free(alloc, ptr);
        // After free and potential merge, should approach initial state
        
        buddy_allocator_destroy(alloc);
    }
    
    SECTION("Buddy merging") {
        BuddyAllocator* alloc = buddy_allocator_create(64, 512);
        
        // Allocate two adjacent small blocks
        void* ptr1 = buddy_allocator_alloc(alloc, 64);
        void* ptr2 = buddy_allocator_alloc(alloc, 64);
        REQUIRE(ptr1 != nullptr);
        REQUIRE(ptr2 != nullptr);
        
        u32 blocks_before = alloc->free_blocks;
        
        // Free in reverse order (may trigger merging)
        buddy_allocator_free(alloc, ptr2);
        buddy_allocator_free(alloc, ptr1);
        
        // Free count should increase due to merging
        REQUIRE(alloc->free_blocks >= blocks_before);
        
        buddy_allocator_destroy(alloc);
    }
    
    SECTION("Out of memory handling") {
        BuddyAllocator* alloc = buddy_allocator_create(64, 256);
        
        void* ptr1 = buddy_allocator_alloc(alloc, 128);
        void* ptr2 = buddy_allocator_alloc(alloc, 128);
        REQUIRE(ptr1 != nullptr);
        REQUIRE(ptr2 != nullptr);
        
        // Pool is full (256 bytes allocated)
        void* ptr3 = buddy_allocator_alloc(alloc, 64);
        REQUIRE(ptr3 == nullptr);
        
        // Free one block
        buddy_allocator_free(alloc, ptr1);
        
        // Should be able to allocate again
        void* ptr4 = buddy_allocator_alloc(alloc, 64);
        REQUIRE(ptr4 != nullptr);
        
        buddy_allocator_destroy(alloc);
    }
    
    SECTION("Size rounding") {
        BuddyAllocator* alloc = buddy_allocator_create(64, 512);
        
        // Request non-power-of-2 size (should round up)
        void* ptr1 = buddy_allocator_alloc(alloc, 100); // Rounds to 128
        REQUIRE(ptr1 != nullptr);
        
        void* ptr2 = buddy_allocator_alloc(alloc, 50); // Rounds to 64
        REQUIRE(ptr2 != nullptr);
        
        buddy_allocator_destroy(alloc);
    }
    
    SECTION("Fragmentation characteristics") {
        BuddyAllocator* alloc = buddy_allocator_create(64, 1024);
        
        // Allocate and free in pattern to create fragmentation
        void* ptrs[8];
        for (int i = 0; i < 8; i++) {
            ptrs[i] = buddy_allocator_alloc(alloc, 64);
            REQUIRE(ptrs[i] != nullptr);
        }
        
        // Free every other block
        for (int i = 0; i < 8; i += 2) {
            buddy_allocator_free(alloc, ptrs[i]);
        }
        
        // Buddy allocator should handle fragmentation via merging
        // Can still allocate
        void* ptr_new = buddy_allocator_alloc(alloc, 64);
        REQUIRE(ptr_new != nullptr);
        
        buddy_allocator_destroy(alloc);
    }
}

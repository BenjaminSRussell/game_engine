#include <catch2/catch_test_macros.hpp>
#include <core/memory.h>
#include <cstring>

// Test Linear Allocator implementation
TEST_CASE("Linear Allocator", "[memory][linear]") {
    
    SECTION("Creation and destruction") {
        LinearAllocator* alloc = linear_allocator_create(1024, 1);
        REQUIRE(alloc != nullptr);
        REQUIRE(alloc->size == 1024);
        REQUIRE(alloc->offset == 0);
        
        linear_allocator_destroy(alloc);
    }
    
    SECTION("Basic allocation") {
        LinearAllocator* alloc = linear_allocator_create(1024, 1);
        REQUIRE(alloc != nullptr);
        
        void* ptr1 = linear_allocator_alloc(alloc, 64, 4);
        REQUIRE(ptr1 != nullptr);
        REQUIRE(alloc->offset == 64);
        
        void* ptr2 = linear_allocator_alloc(alloc, 128, 4);
        REQUIRE(ptr2 != nullptr);
        REQUIRE(alloc->offset == 192);
        
        // Verify pointers are within buffer
        REQUIRE(ptr1 >= alloc->base);
        REQUIRE(ptr2 >= alloc->base);
        REQUIRE((char*)ptr2 - (char*)ptr1 == 64);
        
        linear_allocator_destroy(alloc);
    }
    
    SECTION("Alignment") {
        LinearAllocator* alloc = linear_allocator_create(1024, 1);
        
        // Allocate with 16-byte alignment
        void* ptr1 = linear_allocator_alloc(alloc, 10, 16);
        REQUIRE(ptr1 != nullptr);
        REQUIRE((uintptr_t)ptr1 % 16 == 0); // Check 16-byte aligned
        
        // Allocate with 32-byte alignment
        void* ptr2 = linear_allocator_alloc(alloc, 20, 32);
        REQUIRE(ptr2 != nullptr);
        REQUIRE((uintptr_t)ptr2 % 32 == 0); // Check 32-byte aligned
        
        linear_allocator_destroy(alloc);
    }
    
    SECTION("Reset functionality") {
        LinearAllocator* alloc = linear_allocator_create(1024, 1);
        
        void* ptr1 = linear_allocator_alloc(alloc, 256, 4);
        REQUIRE(alloc->offset == 256);
        
        linear_allocator_reset(alloc);
        REQUIRE(alloc->offset == 0);
        
        // Can reuse memory after reset
        void* ptr2 = linear_allocator_alloc(alloc, 128, 4);
        REQUIRE(ptr2 == alloc->base); // Points to start again
        
        linear_allocator_destroy(alloc);
    }
    
    SECTION("Out of memory handling") {
        LinearAllocator* alloc = linear_allocator_create(128, 1);
        
        void* ptr1 = linear_allocator_alloc(alloc, 64, 4);
        REQUIRE(ptr1 != nullptr);
        
        void* ptr2 = linear_allocator_alloc(alloc, 64, 4);
        REQUIRE(ptr2 != nullptr);
        
        // This should fail (128 bytes used, 128 total)
        void* ptr3 = linear_allocator_alloc(alloc, 64, 4);
        REQUIRE(ptr3 == nullptr);
        
        linear_allocator_destroy(alloc);
    }
    
    SECTION("Double buffering") {
        LinearAllocator* alloc = linear_allocator_create(1024, 2);
        REQUIRE(alloc->buffer_count == 2);
        
        void* ptr1 = linear_allocator_alloc(alloc, 64, 4);
        REQUIRE(alloc->current_buffer == 0);
        
        linear_allocator_swap_buffers(alloc);
        REQUIRE(alloc->current_buffer == 1);
        REQUIRE(alloc->offset == 0); // Reset after swap
        
        void* ptr2 = linear_allocator_alloc(alloc, 64, 4);
        REQUIRE(ptr2 != ptr1); // Different buffer
        
        linear_allocator_destroy(alloc);
    }
    
    SECTION("Peak usage tracking") {
        LinearAllocator* alloc = linear_allocator_create(1024, 1);
        
        linear_allocator_alloc(alloc, 256, 4);
        REQUIRE(alloc->peak_usage == 256);
        
        linear_allocator_alloc(alloc, 512, 4);
        REQUIRE(alloc->peak_usage == 768);
        
        linear_allocator_reset(alloc);
        REQUIRE(alloc->peak_usage == 768); // Peak preserved after reset
        
        linear_allocator_destroy(alloc);
    }
}

// Test SIMD memory operations
TEST_CASE("SIMD Memory Operations", "[memory][simd]") {
    
    SECTION("SIMD support detection") {
        bool supported = simd_is_supported();
        // Just verify it returns without crashing
        REQUIRE((supported == true || supported == false));
    }
    
    SECTION("simd_memcpy correctness") {
        const size_t size = 1024;
        char* src = (char*)malloc(size);
        char* dst = (char*)malloc(size);
        
        // Fill source with pattern
        for (size_t i = 0; i < size; i++) {
            src[i] = (char)(i % 256);
        }
        
        // Copy with SIMD
        simd_memcpy(dst, src, size);
        
        // Verify
        for (size_t i = 0; i < size; i++) {
            REQUIRE(dst[i] == src[i]);
        }
        
        free(src);
        free(dst);
    }
    
    SECTION("simd_memset correctness") {
        const size_t size = 512;
        char* buffer = (char*)malloc(size);
        
        simd_memset(buffer, 0xAB, size);
        
        for (size_t i = 0; i < size; i++) {
            REQUIRE((unsigned char)buffer[i] == 0xAB);
        }
        
        free(buffer);
    }
    
    SECTION("simd_memcmp correctness") {
        const size_t size = 256;
        char* buf1 = (char*)malloc(size);
        char* buf2 = (char*)malloc(size);
        
        memset(buf1, 0x55, size);
        memset(buf2, 0x55, size);
        
        // Should be equal
        REQUIRE(simd_memcmp(buf1, buf2, size) == 0);
        
        // Make them different
        buf2[128] = 0x56;
        REQUIRE(simd_memcmp(buf1, buf2, size) != 0);
        
        free(buf1);
        free(buf2);
    }
}

/*
 * mtl_resource_tests.c
 * Unit tests for Metal resource management
 *
 * Compile with: clang -framework Metal -framework Foundation -o mtl_tests mtl_resource_tests.c
 */

#import <Metal/Metal.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_buffer.h"
#include "backend/metal/mtl_texture.h"
#include "backend/metal/mtl_transfer.h"
#include "backend/metal/mtl_resource_pool.h"
#include "backend/metal/mtl_sampler.h"

/* ============================================================================
 * TEST FRAMEWORK
 * ============================================================================ */

typedef struct test_stats {
    int total_tests;
    int passed_tests;
    int failed_tests;
} test_stats_t;

static test_stats_t g_stats = {0};

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        printf("Running: %s\n", #name); \
        g_stats.total_tests++; \
        test_##name(); \
        g_stats.passed_tests++; \
        printf("  ✓ PASSED\n\n"); \
    } \
    static void test_##name(void)

#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  ✗ FAILED: %s (line %d)\n", message, __LINE__); \
            g_stats.failed_tests++; \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(a, b, message) \
    ASSERT((a) == (b), message)

#define ASSERT_NE(a, b, message) \
    ASSERT((a) != (b), message)

#define ASSERT_NOT_NULL(ptr, message) \
    ASSERT((ptr) != NULL, message)

#define ASSERT_NULL(ptr, message) \
    ASSERT((ptr) == NULL, message)

/* ============================================================================
 * BUFFER TESTS
 * ============================================================================ */

TEST(buffer_creation) {
    metal_device_t* device = metal_device_create(0);
    ASSERT_NOT_NULL(device, "Device creation failed");
    
    metal_buffer_desc_t desc = {
        .size = 1024,
        .storage_mode = METAL_STORAGE_SHARED,
        .usage = METAL_BUFFER_USAGE_VERTEX,
        .initial_data = NULL,
        .label = "Test Buffer"
    };
    
    metal_buffer_t* buffer = metal_buffer_create(device, &desc);
    ASSERT_NOT_NULL(buffer, "Buffer creation failed");
    ASSERT_EQ(buffer->size, 1024, "Buffer size mismatch");
    ASSERT(buffer->is_mapped, "Shared buffer should be mapped");
    
    metal_buffer_destroy(buffer);
    metal_device_destroy(device);
}

TEST(ring_buffer_allocation) {
    metal_device_t* device = metal_device_create(0);
    metal_ring_buffer_t* ring = metal_ring_buffer_create(device, 1024, 3);
    ASSERT_NOT_NULL(ring, "Ring buffer creation failed");
    
    size_t offset;
    void* ptr1 = metal_ring_buffer_alloc(ring, 256, 16, &offset);
    ASSERT_NOT_NULL(ptr1, "First allocation failed");
    
    void* ptr2 = metal_ring_buffer_alloc(ring, 256, 16, NULL);
    ASSERT_NOT_NULL(ptr2, "Second allocation failed");
    
    // Check stats
    metal_ring_buffer_stats_t stats = metal_ring_buffer_get_stats(ring);
    ASSERT_EQ(stats.total_allocations, 2, "Allocation count mismatch");
    ASSERT(stats.bytes_allocated_current_frame >= 512, "Allocated bytes too low");
    
    metal_ring_buffer_destroy(ring);
    metal_device_destroy(device);
}

TEST(buffer_sub_allocator) {
    metal_device_t* device = metal_device_create(0);
    metal_buffer_allocator_t* allocator = metal_buffer_allocator_create(device, 4096, METAL_STORAGE_SHARED);
    ASSERT_NOT_NULL(allocator, "Allocator creation failed");
    
    // Allocate multiple buffers
    metal_buffer_t* buf1 = metal_buffer_suballocate(allocator, 256, 256);
    metal_buffer_t* buf2 = metal_buffer_suballocate(allocator, 512, 256);
    metal_buffer_t* buf3 = metal_buffer_suballocate(allocator, 128, 256);
    
    ASSERT_NOT_NULL(buf1, "First sub-allocation failed");
    ASSERT_NOT_NULL(buf2, "Second sub-allocation failed");
    ASSERT_NOT_NULL(buf3, "Third sub-allocation failed");
    
    // Check stats
    metal_buffer_allocator_stats_t stats = metal_buffer_allocator_get_stats(allocator);
    ASSERT_EQ(stats.allocation_count, 3, "Allocation count mismatch");
    ASSERT(stats.allocated_bytes >= 896, "Allocated bytes too low");
    ASSERT(stats.fragmentation >= 0.0 && stats.fragmentation <= 1.0, "Invalid fragmentation");
    
    // Free and verify
    metal_buffer_subfree(allocator, buf2);
    stats = metal_buffer_allocator_get_stats(allocator);
    ASSERT_EQ(stats.allocation_count, 3, "Allocation count should stay same");
    
    metal_buffer_subfree(allocator, buf1);
    metal_buffer_subfree(allocator, buf3);
    
    metal_buffer_allocator_destroy(allocator);
    metal_device_destroy(device);
}

/* ============================================================================
 * TEXTURE TESTS
 * ============================================================================ */

TEST(texture_creation) {
    metal_device_t* device = metal_device_create(0);
    
    metal_texture_desc_t desc = {
        .type = METAL_TEXTURE_TYPE_2D,
        .format = METAL_PIXEL_FORMAT_RGBA8_UNORM,
        .width = 512,
        .height = 512,
        .depth = 1,
        .mip_levels = 1,
        .usage = METAL_TEXTURE_USAGE_SHADER_READ,
        .sample_count = 1,
        .label = "Test Texture"
    };
    
    metal_texture_t* texture = metal_texture_create(device, &desc);
    ASSERT_NOT_NULL(texture, "Texture creation failed");
    ASSERT_EQ(texture->width, 512, "Width mismatch");
    ASSERT_EQ(texture->height, 512, "Height mismatch");
    
    metal_texture_destroy(texture);
    metal_device_destroy(device);
}

TEST(texture_msaa) {
    metal_device_t* device = metal_device_create(0);
    
    metal_texture_desc_t desc = {
        .type = METAL_TEXTURE_TYPE_2D,
        .format = METAL_PIXEL_FORMAT_RGBA8_UNORM,
        .width = 1920,
        .height = 1080,
        .sample_count = 4,  // 4x MSAA
        .usage = METAL_TEXTURE_USAGE_RENDER_TARGET
    };
    
    metal_texture_t* texture = metal_texture_create(device, &desc);
    ASSERT_NOT_NULL(texture, "MSAA texture creation failed");
    
    metal_texture_destroy(texture);
    metal_device_destroy(device);
}

/* ============================================================================
 * SAMPLER TESTS
 * ============================================================================ */

TEST(sampler_creation) {
    metal_device_t* device = metal_device_create(0);
    
    metal_sampler_desc_t desc = {
        .min_filter = METAL_FILTER_LINEAR,
        .mag_filter = METAL_FILTER_LINEAR,
        .mip_filter = METAL_FILTER_LINEAR,
        .address_mode_u = METAL_ADDRESS_REPEAT,
        .address_mode_v = METAL_ADDRESS_REPEAT,
        .address_mode_w = METAL_ADDRESS_REPEAT,
        .max_anisotropy = 16,
        .normalized_coordinates = true,
        .label = "Test Sampler"
    };
    
    metal_sampler_t* sampler = metal_sampler_create(device, &desc);
    ASSERT_NOT_NULL(sampler, "Sampler creation failed");
    
    metal_sampler_destroy(sampler);
    metal_device_destroy(device);
}

TEST(sampler_caching) {
    metal_device_t* device = metal_device_create(0);
    
    // Create two identical samplers
    metal_sampler_t* s1 = metal_sampler_create_linear_clamp(device);
    metal_sampler_t* s2 = metal_sampler_create_linear_clamp(device);
    
    ASSERT_NOT_NULL(s1, "First sampler creation failed");
    ASSERT_NOT_NULL(s2, "Second sampler creation failed");
    ASSERT_EQ(s1, s2, "Samplers should be cached and identical");
    
    // Check cache stats
    metal_sampler_cache_stats_t stats = metal_sampler_cache_get_stats();
    ASSERT(stats.cache_hits >= 1, "Should have at least one cache hit");
    
    metal_sampler_destroy(s1);
    metal_sampler_destroy(s2);
    
    metal_device_destroy(device);
}

/* ============================================================================
 * RESOURCE POOL TESTS
 * ============================================================================ */

TEST(resource_pool_basic) {
    metal_device_t* device = metal_device_create(0);
    metal_resource_pool_t* pool = metal_resource_pool_create(device);
    ASSERT_NOT_NULL(pool, "Pool creation failed");
    
    // Create and return buffer
    metal_buffer_desc_t buf_desc = {
        .size = 1024,
        .storage_mode = METAL_STORAGE_SHARED,
        .usage = METAL_BUFFER_USAGE_VERTEX
    };
    
    metal_buffer_t* buf1 = metal_pool_acquire_buffer(pool, &buf_desc);
    ASSERT_NOT_NULL(buf1, "Buffer acquisition failed");
    
    metal_pool_return_buffer(pool, buf1);
    
    // Acquire again - should reuse
    metal_buffer_t* buf2 = metal_pool_acquire_buffer(pool, &buf_desc);
    ASSERT_EQ(buf1, buf2, "Should reuse pooled buffer");
    
    metal_pool_return_buffer(pool, buf2);
    metal_resource_pool_destroy(pool);
    metal_device_destroy(device);
}

TEST(resource_pool_cleanup) {
    metal_device_t* device = metal_device_create(0);
    metal_resource_pool_t* pool = metal_resource_pool_create(device);
    
    // Create multiple buffers
    metal_buffer_desc_t desc = { .size = 512, .storage_mode = METAL_STORAGE_SHARED };
    
    for (int i = 0; i < 5; i++) {
        metal_buffer_t* buf = metal_pool_acquire_buffer(pool, &desc);
        metal_pool_return_buffer(pool, buf);
    }
    
    metal_pool_stats_t stats = metal_pool_get_stats(pool);
    ASSERT(stats.pooled_buffers > 0, "Should have pooled buffers");
    
    // Cleanup with age threshold 0 (evict everything)
    metal_pool_cleanup(pool, 0);
    
    stats = metal_pool_get_stats(pool);
    ASSERT_EQ(stats.pooled_buffers, 0, "All buffers should be evicted");
    
    metal_resource_pool_destroy(pool);
    metal_device_destroy(device);
}

/* ============================================================================
 * TRANSFER TESTS
 * ============================================================================ */

TEST(transfer_manager_basic) {
    metal_device_t* device = metal_device_create(0);
    metal_transfer_manager_t* transfer = metal_transfer_manager_create(device);
    ASSERT_NOT_NULL(transfer, "Transfer manager creation failed");
    
    metal_transfer_stats_t stats = metal_transfer_manager_get_stats(transfer);
    ASSERT_EQ(stats.total_transfers, 0, "Should start with zero transfers");
    
    metal_transfer_manager_destroy(transfer);
    metal_device_destroy(device);
}

/* ============================================================================
 * PERFORMANCE BENCHMARKS
 * ============================================================================ */

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void benchmark_buffer_allocation(void) {
    printf("\n=== Buffer Allocation Benchmark ===\n");
    metal_device_t* device = metal_device_create(0);
    
    const int iterations = 1000;
    
    // Benchmark: Direct allocation
    double start = get_time_ms();
    metal_buffer_desc_t desc = { .size = 256, .storage_mode = METAL_STORAGE_SHARED };
    for (int i = 0; i < iterations; i++) {
        metal_buffer_t* buf = metal_buffer_create(device, &desc);
        metal_buffer_destroy(buf);
    }
    double direct_time = get_time_ms() - start;
    
    // Benchmark: Sub-allocator
    start = get_time_ms();
    metal_buffer_allocator_t* allocator = metal_buffer_allocator_create(device, 1024 * 1024, METAL_STORAGE_SHARED);
    for (int i = 0; i < iterations; i++) {
        metal_buffer_t* buf = metal_buffer_suballocate(allocator, 256, 256);
        metal_buffer_subfree(allocator, buf);
    }
    double suballoc_time = get_time_ms() - start;
    metal_buffer_allocator_destroy(allocator);
    
    printf("Direct allocation: %.2f ms (%d iterations)\n", direct_time, iterations);
    printf("Sub-allocator:     %.2f ms (%d iterations)\n", suballoc_time, iterations);
    printf("Speedup: %.2fx\n\n", direct_time / suballoc_time);
    
    metal_device_destroy(device);
}

void benchmark_sampler_caching(void) {
    printf("=== Sampler Caching Benchmark ===\n");
    metal_device_t* device = metal_device_create(0);
    
    metal_sampler_cache_clear();
    
    const int iterations = 1000;
    double start = get_time_ms();
    
    for (int i = 0; i < iterations; i++) {
        metal_sampler_t* s = metal_sampler_create_linear_clamp(device);
        metal_sampler_destroy(s);
    }
    
    double time = get_time_ms() - start;
    metal_sampler_cache_stats_t stats = metal_sampler_cache_get_stats();
    
    printf("Created %d samplers in %.2f ms\n", iterations, time);
    printf("Cache hits: %u (%.1f%%)\n", stats.cache_hits, 
           100.0 * stats.cache_hits / (stats.cache_hits + stats.cache_misses));
    printf("Cache misses: %u\n\n", stats.cache_misses);
    
    metal_device_destroy(device);
}

/* ============================================================================
 * TEST RUNNER
 * ============================================================================ */

int main(int argc, char** argv) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║      Metal Resource Management - Test Suite             ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    // Run unit tests
    printf("Running Unit Tests...\n\n");
    
    run_test_buffer_creation();
    run_test_ring_buffer_allocation();
    run_test_buffer_sub_allocator();
    run_test_texture_creation();
    run_test_texture_msaa();
    run_test_sampler_creation();
    run_test_sampler_caching();
    run_test_resource_pool_basic();
    run_test_resource_pool_cleanup();
    run_test_transfer_manager_basic();
    
    // Performance benchmarks
    printf("\nRunning Performance Benchmarks...\n");
    benchmark_buffer_allocation();
    benchmark_sampler_caching();
    
    // Summary
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                          ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("Total:  %d tests\n", g_stats.total_tests);
    printf("Passed: %d tests ✓\n", g_stats.passed_tests);
    printf("Failed: %d tests ✗\n", g_stats.failed_tests);
    printf("\n");
    
    if (g_stats.failed_tests == 0) {
        printf("🎉 ALL TESTS PASSED!\n\n");
        return 0;
    } else {
        printf("❌ SOME TESTS FAILED\n\n");
        return 1;
    }
}

/* End of mtl_resource_tests.c */

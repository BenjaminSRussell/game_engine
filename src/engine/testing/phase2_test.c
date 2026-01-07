/*
 * phase2_test.c
 * Phase 2 Geometry & Directory Reorganization Tests
 *
 * Tests mesh sorting, culling, and BVH systems
 */

#include "../rendering/3d_rendering/geometry/mesh/mesh_sorting.h"
#include "../rendering/3d_rendering/geometry/mesh/static_mesh_draw.h"
#include "../rendering/3d_rendering/geometry/bvh/bvh_scene_cull.h"
#include <stdio.h>
#include <stdbool.h>

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        return false; \
    }

#define TEST_PASS(msg) printf("PASS: %s\n", msg)

// Test 1: Mesh Sorting Initialization
bool test_mesh_sorting_init() {
    printf("\n=== Test 1: Mesh Sorting Initialization ===\n");
    
    int result = mesh_sorting_init();
    TEST_ASSERT(result == 0, "Mesh sorting initialization failed");
    TEST_PASS("Mesh sorting initialized");
    
    mesh_sorting_shutdown();
    TEST_PASS("Mesh sorting shutdown");
    
    return true;
}

// Test 2: Static Mesh Draw Initialization
bool test_static_mesh_draw_init() {
    printf("\n=== Test 2: Static Mesh Draw Initialization ===\n");
    
    int result = static_mesh_draw_init();
    TEST_ASSERT(result == 0, "Static mesh draw init failed");
    TEST_PASS("Static mesh draw initialized");
    
    static_mesh_draw_shutdown();
    TEST_PASS("Static mesh draw shutdown");
    
    return true;
}

// Test 3: BVH Scene Culling Initialization
bool test_bvh_scene_cull_init() {
    printf("\n=== Test 3: BVH Scene Culling Initialization ===\n");
    
    int result = bvh_scene_cull_init();
    TEST_ASSERT(result == 0, "BVH scene cull init failed");
    TEST_PASS("BVH scene cull initialized");
    
    // Test operations
    uint32_t count = bvh_scene_cull_get_count();
    printf("  BVH scene count: %u\n", count);
    TEST_PASS("BVH scene count queried");
    
    size_t memory = bvh_scene_cull_get_memory_usage();
    printf("  Memory usage: %zu bytes\n", memory);
    TEST_PASS("Memory usage queried");
    
    bvh_scene_cull_shutdown();
    TEST_PASS("BVH scene cull shutdown");
    
    return true;
}

// Test 4: Mesh Sorting Statistics
bool test_mesh_sorting_stats() {
    printf("\n=== Test 4: Mesh Sorting Statistics ===\n");
    
    mesh_sorting_init();
    
    uint32_t count = mesh_sorting_get_count();
    printf("  Mesh sorting count: %u\n", count);
    TEST_PASS("Mesh sorting count queried");
    
    size_t memory = mesh_sorting_get_memory_usage();
    printf("  Memory usage: %zu bytes\n", memory);
    TEST_PASS("Memory usage queried");
    
    mesh_sorting_shutdown();
    
    return true;
}

// Test 5: Static Mesh Draw Statistics
bool test_static_mesh_draw_stats() {
    printf("\n=== Test 5: Static Mesh Draw Statistics ===\n");
    
    static_mesh_draw_init();
    
    uint32_t count = static_mesh_draw_get_count();
    printf("  Static mesh draw count: %u\n", count);
    TEST_PASS("Static mesh draw count queried");
    
    size_t memory = static_mesh_draw_get_memory_usage();
    printf("  Memory usage: %zu bytes\n", memory);
    TEST_PASS("Memory usage queried");
    
    static_mesh_draw_shutdown();
    
    return true;
}

// Main test runner
int main(int argc, const char * argv[]) {
    printf("========================================\n");
    printf("Phase 2: Geometry & Directory Tests\n");
    printf("========================================\n");
    
    bool all_passed = true;
    
    all_passed &= test_mesh_sorting_init();
    all_passed &= test_static_mesh_draw_init();
    all_passed &= test_bvh_scene_cull_init();
    all_passed &= test_mesh_sorting_stats();
    all_passed &= test_static_mesh_draw_stats();
    
    printf("\n========================================\n");
    if (all_passed) {
        printf("All Phase 2 tests PASSED!\n");
        printf("========================================\n");
        return 0;
    } else {
        printf("Some Phase 2 tests FAILED!\n");
        printf("========================================\n");
        return 1;
    }
}

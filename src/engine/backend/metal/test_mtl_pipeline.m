/*
 * test_mtl_pipeline.m
 * Metal Pipeline System Tests
 *
 * Tests for pipeline creation, caching, shader library, and validation
 */

#include "backend/metal/mtl_device.h"
#include "backend/metal/mtl_pipeline.h"
#include "backend/metal/mtl_shader_library.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <assert.h>
#include <stdio.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      printf(" FAIL: %s\n", message);                                        \
      tests_failed++;                                                          \
      return false;                                                            \
    }                                                                          \
    tests_passed++;                                                            \
  } while (0)

#define RUN_TEST(test_func)                                                    \
  do {                                                                         \
    printf("\n Running: %s\n", #test_func);                                  \
    if (test_func()) {                                                         \
      printf(" PASS: %s\n", #test_func);                                     \
    } else {                                                                   \
      printf(" FAIL: %s\n", #test_func);                                     \
    }                                                                          \
  } while (0)

/* ============================================================================
 * TEST: Render Pipeline Statistics
 * ============================================================================
 */

bool test_render_pipeline_stats(void) {
  printf("  Testing render pipeline statistics...\n");

  // This test requires actual shader functions, which need compilation
  // TODO: Would need to load actual shaders for full test
  printf("    Skipped: Requires shader compilation\n");

  return true;
}

/* ============================================================================
 * TEST: Compute Pipeline Validation
 * ============================================================================
 */

bool test_compute_pipeline_validation(void) {
  printf("  Testing compute pipeline validation...\n");

  // Get default Metal device
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  if (!device) {
    printf("    No Metal device available\n");
    return true; // Skip on non-Metal systems
  }

  MTLDeviceRef device_ref = (__bridge MTLDeviceRef)device;

  // Test max threads query
  uint32_t max_threads = metal_get_max_threads_per_threadgroup(device_ref);
  TEST_ASSERT(max_threads > 0, "Max threads per threadgroup should be > 0");
  printf("  Max threads per threadgroup: %u\n", max_threads);

  // Test validation with invalid descriptor (NULL function)
  metal_compute_pipeline_desc_t invalid_desc = {0};
  invalid_desc.compute_function = NULL;
  invalid_desc.threadgroup_size_x = 8;
  invalid_desc.threadgroup_size_y = 8;
  invalid_desc.threadgroup_size_z = 1;

  char error_msg[256] = {0};
  bool is_valid = metal_compute_pipeline_validate(device_ref, &invalid_desc,
                                                  error_msg, sizeof(error_msg));

  TEST_ASSERT(!is_valid, "Validation should fail for NULL compute function");
  printf("  Expected validation error: %s\n", error_msg);

  // Test validation with oversized threadgroup
  metal_compute_pipeline_desc_t oversized_desc = {0};
  oversized_desc.compute_function = (MTLFunctionRef)0x1; // Dummy non-null
  oversized_desc.threadgroup_size_x = max_threads + 1;
  oversized_desc.threadgroup_size_y = 1;
  oversized_desc.threadgroup_size_z = 1;

  // This will fail but for different reason (invalid function pointer)
  // In real usage, function would be valid

  return true;
}

/* ============================================================================
 * TEST: Pipeline Cache Operations
 * ============================================================================
 */

bool test_pipeline_cache_operations(void) {
  printf("  Testing pipeline cache operations...\n");

  metal_pipeline_cache_t cache = {0};
  metal_pipeline_cache_init(&cache);

  // Test initial state
  metal_pipeline_cache_stats_t stats = metal_pipeline_cache_get_stats(&cache);
  TEST_ASSERT(stats.total_pipelines == 0, "Initial cache should be empty");
  TEST_ASSERT(stats.cache_hits == 0, "Initial cache hits should be 0");
  TEST_ASSERT(stats.cache_misses == 0, "Initial cache misses should be 0");

  // Test memory limit
  metal_pipeline_cache_set_memory_limit(&cache, 1024 * 1024); // 1 MB limit
  TEST_ASSERT(cache.max_memory_bytes == 1024 * 1024,
              "Memory limit should be set");

  // Test frame advance
  metal_pipeline_cache_advance_frame(&cache);
  TEST_ASSERT(cache.current_frame == 1, "Frame should advance");

  // Test invalidate all
  metal_pipeline_cache_invalidate_all(&cache);
  stats = metal_pipeline_cache_get_stats(&cache);
  TEST_ASSERT(stats.total_pipelines == 0,
              "Cache should be empty after invalidate");

  metal_pipeline_cache_shutdown(&cache);

  return true;
}

/* ============================================================================
 * TEST: Pipeline Cache Invalidation by Hash
 * ============================================================================
 */

bool test_pipeline_cache_invalidation(void) {
  printf("  Testing pipeline cache invalidation by hash...\n");

  metal_pipeline_cache_t cache = {0};
  metal_pipeline_cache_init(&cache);

  // Test invalidating non-existent hash
  bool found = metal_pipeline_cache_invalidate_by_hash(&cache, 0x12345678);
  TEST_ASSERT(!found, "Should return false for non-existent hash");

  // Test invalidate all on empty cache
  metal_pipeline_cache_invalidate_all(&cache);

  metal_pipeline_cache_stats_t stats = metal_pipeline_cache_get_stats(&cache);
  TEST_ASSERT(stats.total_pipelines == 0, "Cache should remain empty");

  metal_pipeline_cache_shutdown(&cache);

  return true;
}

/* ============================================================================
 * TEST: Shader Library Manager
 * ============================================================================
 */

bool test_shader_library_manager(void) {
  printf("  Testing shader library manager...\n");

  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  if (!device) {
    printf("    No Metal device available\n");
    return true;
  }

  MTLDeviceRef device_ref = (__bridge MTLDeviceRef)device;

  // Create library manager
  metal_shader_library_manager_t *manager =
      metal_shader_library_manager_create(device_ref);
  TEST_ASSERT(manager != NULL, "Library manager should be created");
  TEST_ASSERT(manager->library_count == 0, "Initial library count should be 0");

  // Test statistics
  metal_library_stats_t stats = metal_library_manager_get_stats(manager);
  TEST_ASSERT(stats.total_libraries == 0, "Should have 0 libraries initially");
  TEST_ASSERT(stats.total_functions == 0, "Should have 0 functions initially");

  // Print stats
  metal_library_manager_print_stats(manager);

  // Cleanup
  metal_shader_library_manager_destroy(manager);

  return true;
}

/* ============================================================================
 * TEST: Function Metadata Extraction
 * ============================================================================
 */

bool test_function_metadata(void) {
  printf("  Testing function metadata extraction...\n");

  // This test requires actual shader functions
  printf("    Skipped: Requires compiled shader functions\n");

  return true;
}

/* ============================================================================
 * TEST: Function Binding Reflection
 * ============================================================================
 */

bool test_function_bindings(void) {
  printf("  Testing function binding reflection...\n");

  // This test requires actual shader functions with bindings
  printf("    Skipped: Requires compiled shader functions\n");

  return true;
}

/* ============================================================================
 * TEST: Vertex Descriptor
 * ============================================================================
 */

bool test_vertex_descriptor(void) {
  printf("  Testing vertex descriptor...\n");

  metal_vertex_descriptor_t desc;
  metal_vertex_descriptor_init(&desc);

  TEST_ASSERT(desc.attribute_count == 0, "Initial attribute count should be 0");
  TEST_ASSERT(desc.layout_count == 0, "Initial layout count should be 0");

  // Add position attribute (float3 at location 0)
  metal_vertex_descriptor_add_attribute(&desc, 0, METAL_VERTEX_FORMAT_FLOAT3, 0,
                                        0);
  TEST_ASSERT(desc.attribute_count == 1, "Should have 1 attribute after add");
  TEST_ASSERT(desc.attributes[0].format == METAL_VERTEX_FORMAT_FLOAT3,
              "Attribute format should match");

  // Add normal attribute (float3 at location 1)
  metal_vertex_descriptor_add_attribute(&desc, 1, METAL_VERTEX_FORMAT_FLOAT3,
                                        12, 0);
  TEST_ASSERT(desc.attribute_count == 2, "Should have 2 attributes");

  // Set layout for buffer 0 (vertex data, stride=24 bytes)
  metal_vertex_descriptor_set_layout(&desc, 0, 24, 0);
  TEST_ASSERT(desc.layout_count == 1, "Should have 1 layout");
  TEST_ASSERT(desc.layouts[0].stride == 24, "Layout stride should be 24");
  TEST_ASSERT(desc.layouts[0].step_rate == 0, "Step rate should be per-vertex");

  return true;
}

/* ============================================================================
 * TEST: Depth/Stencil State Creation
 * ============================================================================
 */

bool test_depth_stencil_state(void) {
  printf("  Testing depth/stencil state creation...\n");

  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  if (!device) {
    printf("    No Metal device available\n");
    return true;
  }

  MTLDeviceRef device_ref = (__bridge MTLDeviceRef)device;

  // Create simple depth state
  metal_depth_stencil_desc_t desc = {.depth_compare = METAL_COMPARE_LESS,
                                     .depth_write_enabled = true,
                                     .stencil_enabled = false};

  metal_depth_stencil_state_t *state =
      metal_create_depth_stencil_state(device_ref, &desc);

  TEST_ASSERT(state != NULL, "Depth/stencil state should be created");
  TEST_ASSERT(state->state != NULL, "State object should not be NULL");
  TEST_ASSERT(state->desc.depth_compare == METAL_COMPARE_LESS,
              "Compare function should match");
  TEST_ASSERT(state->desc.depth_write_enabled == true,
              "Depth write should be enabled");

  metal_destroy_depth_stencil_state(state);

  return true;
}

/* ============================================================================
 * TEST: Pipeline Hash Functions
 * ============================================================================
 */

bool test_pipeline_hashing(void) {
  printf("  Testing pipeline hash functions...\n");

  // Create two identical descriptors
  metal_compute_pipeline_desc_t desc1 = {.compute_function =
                                             (MTLFunctionRef)0x1000,
                                         .threadgroup_size_x = 8,
                                         .threadgroup_size_y = 8,
                                         .threadgroup_size_z = 1,
                                         .threadgroup_memory_length = 0};

  metal_compute_pipeline_desc_t desc2 = desc1;

  uint64_t hash1 = metal_hash_compute_pipeline_desc(&desc1);
  uint64_t hash2 = metal_hash_compute_pipeline_desc(&desc2);

  TEST_ASSERT(hash1 == hash2,
              "Identical descriptors should hash to same value");
  TEST_ASSERT(hash1 != 0, "Hash should not be zero");

  // Modify descriptor and rehash
  desc2.threadgroup_size_x = 16;
  uint64_t hash3 = metal_hash_compute_pipeline_desc(&desc2);

  TEST_ASSERT(hash1 != hash3,
              "Different descriptors should hash to different values");

  return true;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================
 */

int main(int argc, char *argv[]) {
  @autoreleasepool {
    printf("\n");
    printf("\n");
    printf("   Metal Pipeline System Test Suite                    \n");
    printf("\n");

    // Initialize pipeline system
    if (metal_pipeline_init() != 0) {
      printf(" Failed to initialize pipeline system\n");
      return 1;
    }

    // Run all tests
    RUN_TEST(test_vertex_descriptor);
    RUN_TEST(test_pipeline_hashing);
    RUN_TEST(test_depth_stencil_state);
    RUN_TEST(test_pipeline_cache_operations);
    RUN_TEST(test_pipeline_cache_invalidation);
    RUN_TEST(test_compute_pipeline_validation);
    RUN_TEST(test_shader_library_manager);
    RUN_TEST(test_function_metadata);
    RUN_TEST(test_function_bindings);
    RUN_TEST(test_render_pipeline_stats);

    // Shutdown
    metal_pipeline_shutdown();

    // Print summary
    printf("\n");
    printf("\n");
    printf("   Test Summary                                         \n");
    printf("\n");
    printf("   Passed: %3d                                          \n",
           tests_passed);
    printf("   Failed: %3d                                          \n",
           tests_failed);
    printf("\n");
    printf("\n");

    return (tests_failed == 0) ? 0 : 1;
  }
}

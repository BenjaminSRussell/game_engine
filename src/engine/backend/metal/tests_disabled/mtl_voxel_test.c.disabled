#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "mtl_buffer.h"
#include "mtl_device.h"

#include "rendering/voxel_renderer.c"

/* ============================================================================
 * TEST FRAMEWORK
 * ============================================================================
 */

typedef struct test_stats {
  int total_tests;
  int passed_tests;
  int failed_tests;
} test_stats_t;

static test_stats_t g_stats = {0};

#define TEST(name)                                                             \
  static void test_##name(void);                                               \
  static void run_test_##name(void) {                                          \
    printf("Running: %s\n", #name);                                            \
    g_stats.total_tests++;                                                     \
    test_##name();                                                             \
    g_stats.passed_tests++;                                                    \
    printf("   PASSED\n\n");                                                  \
  }                                                                            \
  static void test_##name(void)

#define ASSERT(condition, message)                                             \
  do {                                                                         \
    if (!(condition)) {                                                        \
      printf("   FAILED: %s (line %d)\n", message, __LINE__);                 \
      g_stats.failed_tests++;                                                  \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define ASSERT_NOT_NULL(ptr, message) ASSERT((ptr) != NULL, message)

/* ============================================================================
 * VOXEL RENDERER TESTS
 * ============================================================================
 */

TEST(renderer_creation) {
  metal_device_t *device = metal_device_create_system_default();
  ASSERT_NOT_NULL(device, "Device creation failed");

  VoxelRenderer *renderer = voxel_renderer_create(device);
  ASSERT_NOT_NULL(renderer, "Voxel renderer creation failed");

  voxel_renderer_destroy(renderer);
}

TEST(mesh_generation) {
  metal_device_t *device = metal_device_create_system_default();
  VoxelRenderer *renderer = voxel_renderer_create(device);

  // Create a small 2x2x2 cube of stone
  uint8_t blocks[8] = {1, 1, 1, 1, 1, 1, 1, 1};
  VoxelMesh *mesh = voxel_mesh_generate(renderer, blocks, 2, 2, 2);

  ASSERT_NOT_NULL(mesh, "Mesh generation failed");
  ASSERT(mesh->vertex_count > 0, "Vertex count should be > 0");
  ASSERT(mesh->index_count > 0, "Index count should be > 0");
  ASSERT_NOT_NULL(mesh->vertex_buffer, "Vertex buffer missing");
  ASSERT_NOT_NULL(mesh->index_buffer, "Index buffer missing");
  ASSERT(mesh->uploaded, "Mesh should be marked as uploaded");

  voxel_renderer_destroy(renderer);
}

int main(int argc, char **argv) {
  printf("\n=== Voxel Renderer Metal Integration Tests ===\n\n");

  run_test_renderer_creation();
  run_test_mesh_generation();

  printf("Total:  %d tests\n", g_stats.total_tests);
  printf("Passed: %d tests \n", g_stats.passed_tests);
  printf("Failed: %d tests \n", g_stats.failed_tests);

  if (g_stats.failed_tests == 0) {
    printf("\n ALL TESTS PASSED!\n\n");
    return 0;
  } else {
    printf("\n SOME TESTS FAILED\n\n");
    return 1;
  }
}

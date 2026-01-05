// test/physics/test_ocean.c - Ocean simulator test
#include "../../test_framework.h"
#include <math.h>
#include <physics/ocean_simulator.h>

static OceanSimulator *ocean = NULL;

static TestResult setup(void) {
  OceanConfig config = ocean_get_default_config();
  config.resolution = 64; // Small for testing
  config.patch_size = 100.0f;
  config.wind_speed = 10.0f;

  ocean = ocean_create(&config);
  TEST_ASSERT_NOT_NULL(ocean, "Ocean creation failed");

  return TEST_PASS;
}

static TestResult teardown(void) {
  ocean_free(ocean);
  ocean = NULL;
  return TEST_PASS;
}

static TestResult test_ocean_creation(void) {
  TEST_ASSERT_NOT_NULL(ocean, "Ocean should be created");
  TEST_ASSERT_EQ(ocean->config.resolution, 64, "Resolution mismatch");
  TEST_ASSERT_FLOAT_EQ(ocean->config.patch_size, 100.0f, 0.001f,
                       "Patch size mismatch");
  return TEST_PASS;
}

static TestResult test_ocean_update(void) {
  ocean_update(ocean, 0.016f); // One frame at 60 FPS

  const float *heights = ocean_get_heights(ocean);
  TEST_ASSERT_NOT_NULL(heights, "Heights should be generated");

  // Check that we have some wave activity
  bool has_waves = false;
  for (uint32_t i = 0; i < 64 * 64; i++) {
    if (fabsf(heights[i]) > 0.001f) {
      has_waves = true;
      break;
    }
  }

  TEST_ASSERT_TRUE(has_waves, "Ocean should have waves");
  return TEST_PASS;
}

static TestResult test_ocean_normals(void) {
  ocean_update(ocean, 0.016f);

  const Vec3 *normals = ocean_get_normals(ocean);
  TEST_ASSERT_NOT_NULL(normals, "Normals should be generated");

  // Check that normals are normalized
  Vec3 n = normals[0];
  float len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
  TEST_ASSERT_FLOAT_EQ(len, 1.0f, 0.01f, "Normals should be normalized");

  return TEST_PASS;
}

static TestResult test_ocean_height_query(void) {
  ocean_update(ocean, 0.016f);

  float h = ocean_get_height_at(ocean, 50.0f, 50.0f);
  // Height should be reasonable (not NaN or infinity)
  TEST_ASSERT_TRUE(!isnan(h) && !isinf(h), "Height should be valid");

  return TEST_PASS;
}

static TestResult test_ocean_statistics(void) {
  ocean_update(ocean, 0.016f);

  float max_height, avg_height;
  ocean_get_statistics(ocean, &max_height, &avg_height);

  TEST_ASSERT_TRUE(max_height >= 0.0f, "Max height should be non-negative");
  TEST_ASSERT_TRUE(avg_height >= 0.0f, "Avg height should be non-negative");
  TEST_ASSERT_TRUE(max_height >= avg_height, "Max should be >= avg");

  return TEST_PASS;
}

void register_ocean_tests(void) {
  TEST_REGISTER_WITH_SETUP("Physics", "ocean_creation", test_ocean_creation,
                           setup, teardown);
  TEST_REGISTER_WITH_SETUP("Physics", "ocean_update", test_ocean_update, setup,
                           teardown);
  TEST_REGISTER_WITH_SETUP("Physics", "ocean_normals", test_ocean_normals,
                           setup, teardown);
  TEST_REGISTER_WITH_SETUP("Physics", "ocean_height_query",
                           test_ocean_height_query, setup, teardown);
  TEST_REGISTER_WITH_SETUP("Physics", "ocean_statistics", test_ocean_statistics,
                           setup, teardown);
}

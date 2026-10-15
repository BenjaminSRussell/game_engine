/* Proves the layout element cache keeps its const contract.
 * Issue #432. layout_invalidation_is_cached is a read-only query and takes a
 * const system. layout_invalidation_get_cached_layout records hit and miss
 * counts, so it takes a mutable system. The registry entry compiles
 * layout_invalidation.c directly with the discarded-qualifiers diagnostic
 * promoted to an error, so a const-discarding assignment fails the build.
 * add_engine_test links Engine, which does not build on origin/main.
 */

#include "ui/layout/layout_invalidation.h"

#include <stdio.h>

static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

static void test_is_cached_accepts_const_system(void) {
  LayoutInvalidationSystem *system = layout_invalidation_create();
  const LayoutInvalidationSystem *view = system;

  EXPECT(!layout_invalidation_is_cached(view, 7), "nothing should be cached yet");

  layout_invalidation_cache_layout(system, 7, 1.0f, 2.0f, 3.0f, 4.0f);
  EXPECT(layout_invalidation_is_cached(view, 7),
         "cached element should be reported through a const view");
  EXPECT(!layout_invalidation_is_cached(view, 8),
         "uncached element should not be reported");

  layout_invalidation_destroy(system);
}

static void test_cached_layout_round_trip_and_stats(void) {
  LayoutInvalidationSystem *system = layout_invalidation_create();
  float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
  uint32_t entries = 0, hits = 0, misses = 0;
  float hit_rate = 0.0f;

  layout_invalidation_register_element(system, 7);
  layout_invalidation_cache_layout(system, 7, 10.0f, 20.0f, 30.0f, 40.0f);

  EXPECT(layout_invalidation_get_cached_layout(system, 7, &x, &y, &width, &height),
         "valid cached layout should be returned");
  EXPECT(x == 10.0f && y == 20.0f && width == 30.0f && height == 40.0f,
         "returned layout should match the cached values");
  EXPECT(!layout_invalidation_get_cached_layout(system, 99, &x, &y, &width, &height),
         "uncached element should miss");

  layout_invalidation_get_cache_stats(system, CACHE_LEVEL_ELEMENT, &entries,
                                      &hits, &misses, &hit_rate);
  EXPECT(hits == 1, "one hit should be recorded");
  EXPECT(misses == 1, "one miss should be recorded");
  EXPECT(hit_rate == 0.5f, "hit rate should be one hit out of two lookups");

  layout_invalidation_destroy(system);
}

static void test_invalidated_entry_is_a_miss(void) {
  LayoutInvalidationSystem *system = layout_invalidation_create();
  float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;

  layout_invalidation_register_element(system, 7);
  layout_invalidation_cache_layout(system, 7, 1.0f, 2.0f, 3.0f, 4.0f);
  layout_invalidation_invalidate(system, 7, INVALIDATE_LAYOUT, "test");

  EXPECT(!layout_invalidation_is_cached(system, 7),
         "invalidated entry should not be reported as cached");
  EXPECT(!layout_invalidation_get_cached_layout(system, 7, &x, &y, &width, &height),
         "invalidated entry should not be returned");

  layout_invalidation_destroy(system);
}

static void test_generation_bump_makes_entry_stale(void) {
  LayoutInvalidationSystem *system = layout_invalidation_create();
  float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;

  layout_invalidation_register_element(system, 7);
  layout_invalidation_cache_layout(system, 7, 1.0f, 2.0f, 3.0f, 4.0f);
  layout_invalidation_increment_generation(system, 7);

  EXPECT(!layout_invalidation_get_cached_layout(system, 7, &x, &y, &width, &height),
         "entry cached at an older element generation should not be returned");

  layout_invalidation_destroy(system);
}

int main(void) {
  test_is_cached_accepts_const_system();
  test_cached_layout_round_trip_and_stats();
  test_invalidated_entry_is_a_miss();
  test_generation_bump_makes_entry_stale();

  if (g_failed) {
    fprintf(stderr, "%d layout invalidation test(s) failed\n", g_failed);
    return 1;
  }
  printf("layout invalidation tests passed\n");
  return 0;
}

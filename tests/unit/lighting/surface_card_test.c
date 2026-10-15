/* Unit tests for the Lumen surface card pool (surface_card.c).
 * Issue #361.
 *
 * The unit is included directly so the tests can read the face normals and
 * mesh ids that generate_for_mesh stores; the public API exposes neither.
 * Not add_engine_test: that macro links Engine, which does not build on
 * origin/main (missing GLFW, then gpu_profiler.c:80).
 */

#include "lighting/lumen/surface_cache/surface_card.c"

#include <stdio.h>

static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);           \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

/* Every test starts from an empty, initialized pool. */
static int reset_pool(void) {
  lumen_surface_card_shutdown();
  return lumen_surface_card_init();
}

static bool vec3_equals(vec3_t a, vec3_t b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

static void test_lifecycle(void) {
  lumen_surface_card_shutdown();
  lumen_surface_card_desc_t desc = {0};
  lumen_surface_card_handle_t handle = {0};

  EXPECT(lumen_surface_card_create(&handle, &desc) == -1,
         "create before init should fail");
  EXPECT(!lumen_surface_card_is_valid(handle),
         "no card should be valid before init");

  EXPECT(reset_pool() == 0, "init should succeed");
  EXPECT(lumen_surface_card_get_count() == 0, "new pool should be empty");
  EXPECT(lumen_surface_card_create(&handle, &desc) == 0,
         "create after init should succeed");

  EXPECT(lumen_surface_card_init() == 0, "second init should be a no-op");
  EXPECT(lumen_surface_card_is_valid(handle),
         "second init must keep existing cards");
  EXPECT(lumen_surface_card_get_count() == 1,
         "second init must keep the count");

  lumen_surface_card_shutdown();
  EXPECT(lumen_surface_card_get_count() == 0,
         "shutdown should reset the count");
  EXPECT(!lumen_surface_card_is_valid(handle),
         "shutdown should invalidate cards");
  EXPECT(lumen_surface_card_create(&handle, &desc) == -1,
         "create after shutdown should fail");

  EXPECT(reset_pool() == 0, "init after shutdown should succeed");
  EXPECT(lumen_surface_card_get_count() == 0,
         "pool should be empty after re-init");
}

static void test_create_copies_descriptor(void) {
  EXPECT(reset_pool() == 0, "init should succeed");

  lumen_surface_card_desc_t desc = {0};
  desc.position = (vec3_t){{1, 2, 3, 0}};
  desc.normal = (vec3_t){{0, 1, 0, 0}};
  desc.extent = (vec3_t){{4, 5, 6, 0}};
  desc.mesh_id = 7;

  lumen_surface_card_handle_t handle = {0};
  EXPECT(lumen_surface_card_create(&handle, &desc) == 0,
         "create should succeed");
  EXPECT(handle.id == 0, "first card should take slot 0");
  EXPECT(lumen_surface_card_is_valid(handle), "new card should be valid");
  EXPECT(lumen_surface_card_get_count() == 1, "count should be 1");

  lumen_surface_card_info_t info = {0};
  EXPECT(lumen_surface_card_get_info(handle, &info) == 0,
         "get_info on a live card should succeed");
  EXPECT(info.id == 0, "info id should match the handle");
  EXPECT(info.active && info.initialized,
         "info should report active and initialized");
  EXPECT(vec3_equals(info.position, desc.position),
         "info position should match the descriptor");

  const lumen_surface_card_t *card = &g_surface_cards[handle.id];
  EXPECT(vec3_equals(card->normal, desc.normal), "normal should be copied");
  EXPECT(vec3_equals(card->extent, desc.extent), "extent should be copied");
  EXPECT(card->mesh_id == 7, "mesh_id should be copied");
}

static void test_create_rejects_bad_arguments(void) {
  EXPECT(reset_pool() == 0, "init should succeed");

  lumen_surface_card_desc_t desc = {0};
  lumen_surface_card_handle_t handle = {99};
  EXPECT(lumen_surface_card_create(NULL, &desc) == -1,
         "NULL handle should fail");
  EXPECT(lumen_surface_card_create(&handle, NULL) == -1,
         "NULL descriptor should fail");
  EXPECT(handle.id == 99, "failed create must not touch the handle");
  EXPECT(lumen_surface_card_get_count() == 0,
         "failed create must not allocate a card");
}

static void test_pool_capacity_and_slot_reuse(void) {
  EXPECT(reset_pool() == 0, "init should succeed");

  lumen_surface_card_desc_t desc = {0};
  for (uint32_t i = 0; i < MAX_SURFACE_CARDS; i++) {
    lumen_surface_card_handle_t handle = {0};
    EXPECT(lumen_surface_card_create(&handle, &desc) == 0,
           "pool should accept MAX_SURFACE_CARDS cards");
    EXPECT(handle.id == i, "cards should fill the pool in slot order");
  }
  EXPECT(lumen_surface_card_get_count() == MAX_SURFACE_CARDS,
         "count should equal the pool size");
  EXPECT(lumen_surface_card_get_memory_usage() ==
             MAX_SURFACE_CARDS * sizeof(lumen_surface_card_t),
         "memory usage should cover the whole pool");

  lumen_surface_card_handle_t handle = {1234};
  EXPECT(lumen_surface_card_create(&handle, &desc) == -2,
         "create on a full pool should fail");
  EXPECT(handle.id == 1234, "failed create must not touch the handle");
  EXPECT(lumen_surface_card_get_count() == MAX_SURFACE_CARDS,
         "failed create must not change the count");

  lumen_surface_card_handle_t freed = {500};
  lumen_surface_card_destroy(freed);
  EXPECT(!lumen_surface_card_is_valid(freed),
         "destroyed card should be invalid");
  EXPECT(lumen_surface_card_get_count() == MAX_SURFACE_CARDS - 1,
         "destroy should free one slot");

  EXPECT(lumen_surface_card_create(&handle, &desc) == 0,
         "freed slot should be available again");
  EXPECT(handle.id == 500, "new card should reuse the freed slot");
  EXPECT(lumen_surface_card_get_count() == MAX_SURFACE_CARDS,
         "count should return to the pool size");
}

static void test_destroy_is_idempotent_and_bounds_checked(void) {
  EXPECT(reset_pool() == 0, "init should succeed");

  lumen_surface_card_desc_t desc = {0};
  lumen_surface_card_handle_t handle = {0};
  EXPECT(lumen_surface_card_create(&handle, &desc) == 0,
         "create should succeed");

  lumen_surface_card_destroy(handle);
  lumen_surface_card_destroy(handle);
  EXPECT(lumen_surface_card_get_count() == 0,
         "double destroy must not underflow the count");
  EXPECT(!lumen_surface_card_is_valid(handle),
         "destroyed card should be invalid");

  lumen_surface_card_handle_t unused = {3};
  lumen_surface_card_handle_t out_of_range = {MAX_SURFACE_CARDS};
  lumen_surface_card_destroy(unused);
  lumen_surface_card_destroy(out_of_range);
  EXPECT(lumen_surface_card_get_count() == 0,
         "destroying unused handles must not change the count");

  lumen_surface_card_shutdown();
  lumen_surface_card_destroy(handle);
  EXPECT(lumen_surface_card_get_count() == 0,
         "destroy after shutdown should be a no-op");
}

static void test_invalid_handles_are_rejected(void) {
  EXPECT(reset_pool() == 0, "init should succeed");

  lumen_surface_card_desc_t desc = {0};
  lumen_surface_card_handle_t handle = {0};
  EXPECT(lumen_surface_card_create(&handle, &desc) == 0,
         "create should succeed");

  lumen_surface_card_handle_t unused = {1};
  lumen_surface_card_handle_t out_of_range = {MAX_SURFACE_CARDS};
  EXPECT(!lumen_surface_card_is_valid(unused), "unused slot should be invalid");
  EXPECT(!lumen_surface_card_is_valid(out_of_range),
         "out-of-range handle should be invalid");

  lumen_surface_card_info_t info = {0};
  info.id = 77;
  EXPECT(lumen_surface_card_get_info(unused, &info) == -1,
         "get_info on an unused slot should fail");
  EXPECT(lumen_surface_card_get_info(handle, NULL) == -1,
         "get_info with NULL out_info should fail");
  EXPECT(info.id == 77, "failed get_info must not write out_info");

  lumen_surface_card_destroy(handle);
  EXPECT(lumen_surface_card_get_info(handle, &info) == -1,
         "get_info on a destroyed card should fail");
}

static void test_generate_for_mesh_makes_six_face_cards(void) {
  EXPECT(reset_pool() == 0, "init should succeed");

  static const float face_normals[6][3] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                                           {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
  lumen_surface_card_handle_t handles[6] = {0};
  uint32_t count = 0;
  EXPECT(lumen_surface_card_generate_for_mesh(42, handles, &count) == 0,
         "generate should succeed on an empty pool");
  EXPECT(count == 6, "generate should report six cards");
  EXPECT(lumen_surface_card_get_count() == 6, "pool should hold six cards");

  for (uint32_t i = 0; i < 6; i++) {
    EXPECT(lumen_surface_card_is_valid(handles[i]),
           "every generated handle should be valid");
    const lumen_surface_card_t *card = &g_surface_cards[handles[i].id];
    EXPECT(card->mesh_id == 42, "generated card should carry the mesh id");
    vec3_t expected = {{face_normals[i][0], face_normals[i][1],
                        face_normals[i][2], 0}};
    EXPECT(vec3_equals(card->normal, expected),
           "face normals should be +X, -X, +Y, -Y, +Z, -Z in order");
  }
}

static void test_generate_rejects_bad_arguments(void) {
  lumen_surface_card_handle_t handles[6] = {0};
  uint32_t count = 7;

  lumen_surface_card_shutdown();
  EXPECT(lumen_surface_card_generate_for_mesh(1, handles, &count) == -1,
         "generate before init should fail");
  EXPECT(count == 7, "generate before init must not touch the count");

  EXPECT(reset_pool() == 0, "init should succeed");
  EXPECT(lumen_surface_card_generate_for_mesh(1, NULL, &count) == -1,
         "NULL handles should fail");
  EXPECT(lumen_surface_card_generate_for_mesh(1, handles, NULL) == -1,
         "NULL count should fail");
  EXPECT(lumen_surface_card_get_count() == 0,
         "rejected generate must not allocate cards");
}

static void test_generate_is_all_or_nothing_when_pool_is_short(void) {
  EXPECT(reset_pool() == 0, "init should succeed");

  lumen_surface_card_desc_t desc = {0};
  lumen_surface_card_handle_t filler = {0};
  for (uint32_t i = 0; i < MAX_SURFACE_CARDS - 3; i++)
    EXPECT(lumen_surface_card_create(&filler, &desc) == 0,
           "filler create should succeed");

  /* Six cards are needed but only three slots are free. */
  lumen_surface_card_handle_t handles[6] = {0};
  uint32_t count = 99;
  EXPECT(lumen_surface_card_generate_for_mesh(9, handles, &count) == -2,
         "generate with three free slots should report the pool as full");
  EXPECT(count == 0, "failed generate should report zero cards");
  EXPECT(lumen_surface_card_get_count() == MAX_SURFACE_CARDS - 3,
         "failed generate must not leave partial cards behind");

  for (uint32_t i = 0; i < 3; i++)
    EXPECT(lumen_surface_card_create(&filler, &desc) == 0,
           "the three slots should still be free after a failed generate");
  EXPECT(lumen_surface_card_create(&filler, &desc) == -2,
         "pool should now be full");
}

static void test_generate_fits_when_exactly_six_slots_are_free(void) {
  EXPECT(reset_pool() == 0, "init should succeed");

  lumen_surface_card_desc_t desc = {0};
  lumen_surface_card_handle_t filler = {0};
  for (uint32_t i = 0; i < MAX_SURFACE_CARDS - 6; i++)
    EXPECT(lumen_surface_card_create(&filler, &desc) == 0,
           "filler create should succeed");

  lumen_surface_card_handle_t handles[6] = {0};
  uint32_t count = 0;
  EXPECT(lumen_surface_card_generate_for_mesh(9, handles, &count) == 0,
         "six free slots should be enough");
  EXPECT(count == 6, "exact fit should report six cards");
  EXPECT(lumen_surface_card_get_count() == MAX_SURFACE_CARDS,
         "exact fit should fill the pool");
}

int main(void) {
  test_lifecycle();
  test_create_copies_descriptor();
  test_create_rejects_bad_arguments();
  test_pool_capacity_and_slot_reuse();
  test_destroy_is_idempotent_and_bounds_checked();
  test_invalid_handles_are_rejected();
  test_generate_for_mesh_makes_six_face_cards();
  test_generate_rejects_bad_arguments();
  test_generate_is_all_or_nothing_when_pool_is_short();
  test_generate_fits_when_exactly_six_slots_are_free();

  lumen_surface_card_shutdown();
  if (g_failed) {
    fprintf(stderr, "%d surface card test(s) failed\n", g_failed);
    return 1;
  }
  printf("surface card tests passed\n");
  return 0;
}

/* Unit test for src/engine/ecs/components/asset_instance_component.c (issue #325).
 *
 * The component is compiled directly and ecs_register_component is stubbed, so
 * the test does not link the ECS world implementation (the Engine target does
 * not build on main; see tests/registry/README.md). The stub records each
 * registration request so the checks can see what the component asks the ECS
 * to register and how often.
 *
 * The component caches its id in a file-static variable, so the checks run in
 * a fixed order inside one process. The first check asserts that nothing has
 * registered the component yet, so a reordering fails loudly instead of
 * passing vacuously.
 */

#include "ecs/components/asset_instance_component.h"
#include "ecs/ecs.h"

#include <stdalign.h>
#include <stdio.h>
#include <string.h>

#define STUB_COMPONENT_ID ((ECSComponentID)7)

static int g_register_calls = 0;
static World *g_last_world = NULL;
static ComponentInfo g_last_info;
static World g_test_world;
static int g_failed = 0;

/* Stub: records the request and hands back a fixed id. */
ECSComponentID ecs_register_component(World *world, const ComponentInfo *info) {
  g_register_calls++;
  g_last_world = world;
  g_last_info = *info;
  return STUB_COMPONENT_ID;
}

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failed++;                                                              \
    }                                                                          \
  } while (0)

/* First call in the process: get_id must register exactly once and return the
 * id the ECS handed back. */
static void test_first_get_id_registers_component(void) {
  CHECK(g_register_calls == 0);

  ECSComponentID id = asset_instance_component_get_id(&g_test_world);

  CHECK(g_register_calls == 1);
  CHECK(id == STUB_COMPONENT_ID);
  CHECK(g_last_world == &g_test_world);
}

/* The registration request describes AssetInstanceComponent accurately. */
static void test_registration_info(void) {
  CHECK(g_register_calls == 1);
  CHECK(g_last_info.name != NULL &&
        strcmp(g_last_info.name, "AssetInstanceComponent") == 0);
  CHECK(g_last_info.size == sizeof(AssetInstanceComponent));
  CHECK(g_last_info.alignment == alignof(AssetInstanceComponent));
  CHECK(!g_last_info.is_tag);
  CHECK(g_last_info.default_constructor == NULL);
  CHECK(g_last_info.destructor == NULL);
  CHECK(g_last_info.serialize == NULL);
  CHECK(g_last_info.deserialize == NULL);
}

/* Later get_id calls return the cached id without registering again. */
static void test_get_id_returns_cached_id(void) {
  ECSComponentID id = asset_instance_component_get_id(&g_test_world);

  CHECK(id == STUB_COMPONENT_ID);
  CHECK(g_register_calls == 1);
}

/* An explicit register after registration must not register a second time. */
static void test_register_is_idempotent(void) {
  asset_instance_component_register(&g_test_world);

  CHECK(g_register_calls == 1);
  CHECK(asset_instance_component_get_id(&g_test_world) == STUB_COMPONENT_ID);
  CHECK(g_register_calls == 1);
}

int main(void) {
  test_first_get_id_registers_component();
  test_registration_info();
  test_get_id_returns_cached_id();
  test_register_is_idempotent();

  if (g_failed != 0) {
    fprintf(stderr, "asset_instance_component: %d check(s) failed\n", g_failed);
    return 1;
  }
  printf("asset_instance_component: all checks passed\n");
  return 0;
}

/*
 * ecs_system_impl_test.c
 * Entity lifetime, hierarchy, components, system order, world lookup, events.
 * Compiles src/engine/ecs/ecs_system_impl.c directly and does not link Engine.
 */

#include "ecs/ecs_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_order[8];
static int g_order_n;
static float g_last_dt;
static int g_event_hits;
static void *g_event_data;

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static void expect_true(const char *msg, int cond) {
  if (!cond)
    fail(msg);
}

static void sys_lo(float dt) {
  g_last_dt = dt;
  g_order[g_order_n++] = 1;
}

static void sys_hi(float dt) {
  (void)dt;
  g_order[g_order_n++] = 2;
}

static void on_hit(void *data) {
  g_event_hits++;
  g_event_data = data;
}

static void test_entity_lifetime(void) {
  uint32_t id;
  uint32_t gen;
  char long_name[80];
  const char *stored;

  id = entity_create();
  expect_true("create id", id == 1);
  expect_true("create valid", entity_is_valid(id));
  expect_true("create enabled", entity_is_enabled(id));
  expect_true("create generation", entity_get_generation(id) == 1);
  expect_true("create name", entity_get_name(id)[0] == '\0');
  expect_true("missing invalid", !entity_is_valid(0));
  expect_true("missing generation", entity_get_generation(0) == 0);

  entity_set_name(id, NULL);
  expect_true("null name ignored", entity_get_name(id)[0] == '\0');
  entity_set_name(id, "hero");
  expect_true("name", strcmp(entity_get_name(id), "hero") == 0);

  memset(long_name, 'b', sizeof(long_name) - 1);
  long_name[sizeof(long_name) - 1] = '\0';
  entity_set_name(id, long_name);
  stored = entity_get_name(id);
  expect_true("name length", strlen(stored) == 63);
  expect_true("name terminated", stored[63] == '\0');

  entity_set_enabled(id, false);
  expect_true("disabled still valid", entity_is_valid(id));
  expect_true("disabled flag", !entity_is_enabled(id));
  expect_true("disabled keeps name", strlen(entity_get_name(id)) == 63);
  entity_set_enabled(id, true);
  expect_true("reenabled", entity_is_enabled(id) && entity_is_valid(id));

  entity_set_name(id, "hero");
  gen = entity_get_generation(id);
  entity_destroy(id);
  expect_true("destroyed invalid", !entity_is_valid(id));
  expect_true("destroyed disabled", !entity_is_enabled(id));
  expect_true("generation bump", entity_get_generation(id) == gen + 1);
  expect_true("destroyed name", entity_get_name(id)[0] == '\0');
  entity_set_enabled(id, true);
  expect_true("destroyed stays dead", !entity_is_valid(id));
}

static void test_hierarchy_and_clone(void) {
  uint32_t parent;
  uint32_t other;
  uint32_t child;
  uint32_t clone;
  uint32_t count;
  uint32_t *kids;
  uint32_t health;
  int *value;

  parent = entity_create();
  other = entity_create();
  child = entity_create();
  entity_set_name(child, "child");
  entity_add_child(parent, child);
  expect_true("parent link", entity_get_parent(child) == parent);
  kids = entity_get_children(parent, &count);
  expect_true("one child", count == 1 && kids && kids[0] == child);

  entity_add_child(parent, child);
  kids = entity_get_children(parent, &count);
  expect_true("no duplicate child", count == 1);

  entity_set_parent(child, other);
  expect_true("reparent", entity_get_parent(child) == other);
  entity_get_children(parent, &count);
  expect_true("old parent cleared", count == 0);
  kids = entity_get_children(other, &count);
  expect_true("new parent list", count == 1 && kids && kids[0] == child);

  entity_remove_child(other, child);
  expect_true("unparent", entity_get_parent(child) == 0);
  entity_get_children(other, &count);
  expect_true("removed from list", count == 0);

  entity_add_child(parent, child);
  entity_destroy(child);
  entity_get_children(parent, &count);
  expect_true("destroy unlinks child", count == 0);

  child = entity_create();
  entity_add_child(parent, child);
  entity_destroy(parent);
  expect_true("orphan stays valid", entity_is_valid(child));
  expect_true("orphan parent cleared", entity_get_parent(child) == 0);

  entity_set_name(child, "clone-me");
  entity_set_enabled(child, false);
  health = component_register("health", sizeof(int));
  expect_true("health id", health != 0);
  value = component_add(child, health);
  expect_true("health storage", value != NULL);
  *value = 9;
  clone = entity_clone(child);
  expect_true("clone distinct", clone != 0 && clone != child && entity_is_valid(clone));
  expect_true("clone name", strcmp(entity_get_name(clone), "clone-me") == 0);
  expect_true("clone disabled", !entity_is_enabled(clone));
  expect_true("clone no parent", entity_get_parent(clone) == 0);
  value = component_get(clone, health);
  expect_true("clone component", value && *value == 9);
  entity_set_enabled(child, true);
}

static void test_components(void) {
  uint32_t entity;
  uint32_t other;
  uint32_t health;
  uint32_t ammo;
  int *hp;
  int *copied;

  entity = entity_create();
  other = entity_create();
  health = component_get_id("health");
  expect_true("health lookup", health != 0);
  expect_true("health info", strcmp(component_get_info(health), "health") == 0);
  expect_true("reregister", component_register("health", sizeof(int)) == health);
  expect_true("reject empty", component_register("", sizeof(int)) == 0);
  expect_true("reject zero size", component_register("empty", 0) == 0);

  ammo = component_register("ammo", sizeof(int));
  expect_true("distinct component", ammo != 0 && ammo != health);
  expect_true("ammo lookup", component_get_id("ammo") == ammo);

  expect_true("add invalid entity", component_add(0, health) == NULL);
  expect_true("add unknown component", component_add(entity, 0) == NULL);
  hp = component_add(entity, health);
  expect_true("add", hp != NULL);
  *hp = 42;
  expect_true("has", component_has(entity, health));
  expect_true("get", component_get(entity, health) == hp && *hp == 42);

  component_set_enabled(entity, health, false);
  expect_true("disabled still attached", component_has(entity, health));
  expect_true("get hides disabled", component_get(entity, health) == NULL);
  expect_true("try_get disabled", component_try_get(entity, health) == hp);
  component_set_enabled(entity, health, true);
  expect_true("component reenabled", *(int *)component_get(entity, health) == 42);

  component_copy(entity, other, health);
  copied = component_get(other, health);
  expect_true("copy", copied && copied != hp && *copied == 42);
  *hp = 7;
  expect_true("copy is independent", *copied == 42);

  component_remove(entity, health);
  expect_true("removed", !component_has(entity, health));
  expect_true("removed get", component_get(entity, health) == NULL);
  expect_true("other kept", *(int *)component_get(other, health) == 42);
}

static void test_world_systems_events(void) {
  void *world;
  uint32_t alpha;
  uint32_t beta;
  uint32_t before;
  int payload;

  world = world_create();
  expect_true("world", world != NULL);
  expect_true("null world count", world_get_entity_count(NULL) == 0);
  expect_true("null world find", world_find_entity_by_name(NULL, "alpha") == 0);

  alpha = entity_create();
  entity_set_name(alpha, "alpha");
  beta = entity_create();
  entity_set_name(beta, "beta");
  before = world_get_entity_count(world);
  entity_destroy(beta);
  expect_true("count ignores destroyed", world_get_entity_count(world) == before - 1);
  expect_true("find alpha", world_find_entity_by_name(world, "alpha") == alpha);
  expect_true("destroyed not found", world_find_entity_by_name(world, "beta") == 0);
  expect_true("missing name", world_find_entity_by_name(world, "nope") == 0);

  expect_true("missing system disabled", !system_get_enabled("missing"));
  system_register("lo", sys_lo);
  system_register("hi", sys_hi);
  system_set_priority("lo", 1);
  system_set_priority("hi", 5);
  expect_true("system enabled", system_get_enabled("lo") && system_get_enabled("hi"));

  g_order_n = 0;
  world_update(world, 0.25f);
  expect_true("priority order", g_order_n == 2 && g_order[0] == 2 && g_order[1] == 1);
  expect_true("update dt", g_last_dt == 0.25f);

  system_add_dependency("hi", "lo");
  g_order_n = 0;
  world_update(world, 0.5f);
  expect_true("dependency order", g_order_n == 2 && g_order[0] == 1 && g_order[1] == 2);

  system_set_enabled("hi", false);
  expect_true("system disabled", !system_get_enabled("hi"));
  g_order_n = 0;
  g_last_dt = 0.0f;
  system_execute("hi", 0.5f);
  expect_true("disabled execute skipped", g_order_n == 0);
  system_execute("lo", 0.5f);
  expect_true("execute", g_order_n == 1 && g_order[0] == 1 && g_last_dt == 0.5f);
  system_execute("missing", 0.5f);
  expect_true("missing execute", g_order_n == 1);

  event_register("hit");
  event_subscribe("hit", on_hit);
  payload = 7;
  g_event_hits = 0;
  g_event_data = NULL;
  event_emit("hit", &payload);
  expect_true("emit", g_event_hits == 1 && g_event_data == &payload);

  g_event_hits = 0;
  event_emit_deferred("hit", &payload);
  expect_true("deferred waits", g_event_hits == 0);
  event_queue_clear();
  event_queue_process();
  expect_true("cleared queue", g_event_hits == 0);

  event_emit_deferred("hit", &payload);
  event_queue_process();
  expect_true("deferred flush", g_event_hits == 1 && g_event_data == &payload);

  event_unsubscribe("hit", on_hit);
  g_event_hits = 0;
  event_emit("hit", &payload);
  expect_true("unsubscribed", g_event_hits == 0);

  world_clear(world);
  expect_true("cleared", world_get_entity_count(world) == 0);
  expect_true("cleared name gone", world_find_entity_by_name(world, "alpha") == 0);
  world_destroy(world);
}

int main(void) {
  test_entity_lifetime();
  test_hierarchy_and_clone();
  test_components();
  test_world_systems_events();
  printf("ok\n");
  return 0;
}

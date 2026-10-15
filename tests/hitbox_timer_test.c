// tests/hitbox_timer_test.c
//
// Issue #119: a temporary hitbox is destroyed when its lifetime TimerComponent
// expires, and not before. hitbox.c is compiled directly. The ECS, logger and
// damage calls it makes are stubbed below, because the Engine library does not
// link on this host (see tests/registry/test_hitbox_timer.cmake).

#include <include/core/logger.h>
#include <include/gameplay/combat/damage.h>
#include <include/gameplay/combat/hitbox.h>
#include <stdbool.h>
#include <stdio.h>

// ----------------------------------------------------------------------------
// Stubs for the external calls hitbox.c makes
// ----------------------------------------------------------------------------

#define STUB_MAX_ENTITIES 256
static bool g_entity_alive[STUB_MAX_ENTITIES];
static u32 g_next_entity_id = 1;

Entity ecs_create_entity(World *world) {
  (void)world;
  if (g_next_entity_id >= STUB_MAX_ENTITIES)
    return INVALID_ENTITY;
  Entity entity = {g_next_entity_id, 1};
  g_entity_alive[g_next_entity_id++] = true;
  return entity;
}

void ecs_destroy_entity(World *world, Entity entity) {
  (void)world;
  if (entity.id > 0 && entity.id < STUB_MAX_ENTITIES)
    g_entity_alive[entity.id] = false;
}

bool ecs_is_valid(World *world, Entity entity) {
  (void)world;
  return entity.id > 0 && entity.id < STUB_MAX_ENTITIES &&
         g_entity_alive[entity.id];
}

bool ecs_add_component(World *world, Entity entity, ECSComponentID component_id,
                       const void *data) {
  (void)world;
  (void)entity;
  (void)component_id;
  (void)data;
  return true;
}

void *ecs_get_component(World *world, Entity entity,
                        ECSComponentID component_id) {
  (void)world;
  (void)entity;
  (void)component_id;
  return NULL;
}

DamageEvent *damage_event_create(Entity source, Entity target, f32 amount,
                                 DamageType type) {
  (void)source;
  (void)target;
  (void)amount;
  (void)type;
  return NULL;
}

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

// ----------------------------------------------------------------------------
// Checks
// ----------------------------------------------------------------------------

// hitbox.c only needs a non-NULL world; the stubs ignore it.
static World g_world;
static int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failures++;                                                            \
    }                                                                          \
  } while (0)

static const Vec3 kOrigin = {0.0f, 0.0f, 0.0f};
static const Vec3 kFar = {10.0f, 0.0f, 0.0f};
static const Vec3 kForward = {0.0f, 0.0f, 1.0f};

// Number of live hitboxes overlapping a point, via the public query.
static u32 hitboxes_at(Vec3 point) {
  Entity found[8];
  return hitbox_system_query_sphere(&point, 0.01f, INVALID_ENTITY, found, 8);
}

// Alive until the duration has elapsed, then destroyed and no longer queryable.
static void test_expires_when_duration_elapses(void) {
  hitbox_system_init(&g_world);
  Entity hb = hitbox_create_temporary(&g_world, kOrigin, kForward, 1.0f, 0.5f);
  CHECK(ecs_is_valid(&g_world, hb));

  hitbox_system_update(&g_world, 0.25f);
  CHECK(ecs_is_valid(&g_world, hb));
  CHECK(hitboxes_at(kOrigin) == 1);

  hitbox_system_update(&g_world, 0.25f); // 0.5s elapsed == duration
  CHECK(!ecs_is_valid(&g_world, hb));
  CHECK(hitboxes_at(kOrigin) == 0);
  hitbox_system_shutdown();
}

// Zero duration has no lifetime left, so the first update destroys it. The
// destruction_time check treats 0 as "no timer" and keeps it alive forever.
static void test_zero_duration_expires_on_first_update(void) {
  hitbox_system_init(&g_world);
  Entity hb = hitbox_create_temporary(&g_world, kOrigin, kForward, 1.0f, 0.0f);
  hitbox_system_update(&g_world, 0.25f);
  CHECK(!ecs_is_valid(&g_world, hb));
  CHECK(hitboxes_at(kOrigin) == 0);
  hitbox_system_shutdown();
}

// Expiring one hitbox removes only that one; the longer-lived hitbox keeps going.
static void test_short_hitbox_expires_while_long_one_survives(void) {
  hitbox_system_init(&g_world);
  Entity short_hb =
      hitbox_create_temporary(&g_world, kOrigin, kForward, 1.0f, 0.25f);
  Entity long_hb = hitbox_create_temporary(&g_world, kFar, kForward, 1.0f, 1.0f);

  hitbox_system_update(&g_world, 0.25f);
  CHECK(!ecs_is_valid(&g_world, short_hb));
  CHECK(hitboxes_at(kOrigin) == 0);
  CHECK(ecs_is_valid(&g_world, long_hb));
  CHECK(hitboxes_at(kFar) == 1);

  hitbox_system_update(&g_world, 0.75f); // 1.0s elapsed for the long hitbox
  CHECK(!ecs_is_valid(&g_world, long_hb));
  CHECK(hitboxes_at(kFar) == 0);
  hitbox_system_shutdown();
}

int main(void) {
  test_expires_when_duration_elapses();
  test_zero_duration_expires_on_first_update();
  test_short_hitbox_expires_while_long_one_survives();
  if (g_failures > 0) {
    fprintf(stderr, "test_hitbox_timer: %d check(s) failed\n", g_failures);
    return 1;
  }
  printf("test_hitbox_timer: all checks passed\n");
  return 0;
}

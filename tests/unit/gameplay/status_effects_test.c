/* Issue #162: status_effects.c had no unit tests. Burning and healing never
 * reached the target's HealthComponent: the status system keys entities by
 * id alone, so its damage events carried generation 0, which ecs_is_valid
 * rejects, and healing was negative damage that damage_calculate_final clamps
 * to zero. The ECS stand-ins below mirror the generation check in ecs.c.
 *
 * Compiles status_effects.c and damage_system.c directly, because
 * add_engine_test links Engine, which does not build on main.
 */

#include "core/logging/unified_logger.h"
#include "ecs/component_ids.h"
#include "ecs/components/health.h"
#include "ecs/ecs.h"
#include "gameplay/combat/damage.h"
#include "gameplay/combat/status_effects.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

/* ecs.c bumps a record's generation when an entity is created, so a live
 * handle never has generation 0. */
#define TARGET_ID 7u
#define TARGET_GENERATION 3u

static World g_world;
static HealthComponent g_health;
static int g_failures = 0;

static Entity target_handle(void) {
  Entity e = {.id = TARGET_ID, .generation = TARGET_GENERATION};
  return e;
}

Entity ecs_get_entity_by_id(World *world, u32 id) {
  (void)world;
  return id == TARGET_ID ? target_handle() : INVALID_ENTITY;
}

void *ecs_get_component(World *world, Entity entity,
                        ECSComponentID component_id) {
  (void)world;
  Entity live = target_handle();
  if (component_id != HEALTH_COMPONENT_ID)
    return NULL;
  if (entity.id != live.id || entity.generation != live.generation)
    return NULL;
  return &g_health;
}

void ecs_destroy_entity(World *world, Entity entity) {
  (void)world;
  (void)entity;
}

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failures++;                                                            \
    }                                                                          \
  } while (0)

static int approx(float a, float b) { return fabsf(a - b) < 1e-4f; }

static int find_active(StatusEffectType type, ActiveEffectInfo *out) {
  ActiveEffectInfo infos[16];
  uint32_t count = status_sys_get_active_effects(TARGET_ID, infos, 16);
  for (uint32_t i = 0; i < count; i++) {
    if (infos[i].type == type) {
      if (out)
        *out = infos[i];
      return 1;
    }
  }
  return 0;
}

/* One frame: status effects update first, then the damage queue is applied. */
static void advance(float dt) {
  status_sys_update(dt);
  damage_system_process_events(&g_world, dt);
}

static void reset(void) {
  status_sys_shutdown();
  damage_system_shutdown();
  status_sys_init();
  damage_system_init(16);
  memset(&g_health, 0, sizeof g_health);
}

static void test_apply_then_expire(void) {
  ActiveEffectInfo info;
  reset();
  status_sys_apply_effect(TARGET_ID, EFFECT_SHIELD, 1.0f, 5.0f);
  CHECK(find_active(EFFECT_SHIELD, &info));
  CHECK(approx(info.duration_remaining, 1.0f));
  CHECK(approx(info.magnitude, 5.0f));

  status_sys_update(0.25f);
  status_sys_update(0.25f);
  status_sys_update(0.25f);
  CHECK(find_active(EFFECT_SHIELD, &info));
  CHECK(approx(info.duration_remaining, 0.25f));

  status_sys_update(0.25f);
  CHECK(!find_active(EFFECT_SHIELD, NULL));
}

static void test_burning_damages_until_it_expires(void) {
  reset();
  g_health = (HealthComponent){.health = 10.0f, .max_health = 10.0f,
                               .is_alive = true};
  status_sys_apply_effect(TARGET_ID, EFFECT_BURNING, 1.0f, 1.0f);

  advance(0.25f); /* one tick of 1 damage per frame, three ticks in total */
  advance(0.25f);
  advance(0.25f);
  CHECK(approx(g_health.health, 7.0f));

  advance(0.25f); /* expires on this frame, no tick */
  CHECK(!find_active(EFFECT_BURNING, NULL));
  advance(0.25f);
  CHECK(approx(g_health.health, 7.0f));
}

static void test_healing_restores_health_capped_at_max(void) {
  reset();
  g_health = (HealthComponent){.health = 4.0f, .max_health = 10.0f,
                               .is_alive = true};
  status_sys_apply_effect(TARGET_ID, EFFECT_HEALING, 1.0f, 4.0f);

  advance(0.25f);
  CHECK(approx(g_health.health, 8.0f));
  advance(0.25f); /* 12 would exceed max_health */
  CHECK(approx(g_health.health, 10.0f));
  advance(0.25f);
  advance(0.25f); /* expires */
  CHECK(approx(g_health.health, 10.0f));
  CHECK(!find_active(EFFECT_HEALING, NULL));
}

static void test_mutually_exclusive_effects_are_rejected(void) {
  reset();
  status_sys_apply_effect(TARGET_ID, EFFECT_BURNING, 5.0f, 1.0f);
  status_sys_apply_effect(TARGET_ID, EFFECT_FREEZING, 5.0f, 0.5f);
  CHECK(find_active(EFFECT_BURNING, NULL));
  CHECK(!find_active(EFFECT_FREEZING, NULL));

  status_sys_apply_effect(TARGET_ID, EFFECT_SHIELD, 5.0f, 2.0f);
  CHECK(find_active(EFFECT_SHIELD, NULL));
}

int main(void) {
  test_apply_then_expire();
  test_burning_damages_until_it_expires();
  test_healing_restores_health_capped_at_max();
  test_mutually_exclusive_effects_are_rejected();

  status_sys_shutdown();
  damage_system_shutdown();
  if (g_failures) {
    fprintf(stderr, "status_effects: %d check(s) failed\n", g_failures);
    return 1;
  }
  printf("status_effects: all checks passed\n");
  return 0;
}

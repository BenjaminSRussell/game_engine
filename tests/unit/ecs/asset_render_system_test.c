/* Proves asset_render_system hands each visible, linked asset instance to the
 * registered submit callback, with its world matrix (T * R * S, column-major)
 * and render flags. Issue #133.
 *
 * Engine does not build on this host, so the unit is compiled directly and the
 * ECS query, component ids, asset lookup and logger are stubbed below.
 */

#include "core/logging/unified_logger.h"
#include "ecs/components/asset_instance_component.h"
#include "ecs/components/transform.h"
#include "ecs/ecs.h"
#include "ecs/systems/asset_render_system.h"
#include "include/core/resource/asset_instance.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---- Logger stub: LOG_* expands to unified_logger_log ---- */

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

/* ---- ECS stubs: one query over a fixed table of entities ---- */

typedef struct {
  AssetInstanceComponent asset;
  TransformComponent transform;
} FakeEntity;

#define FAKE_MAX_ENTITIES 8
static FakeEntity g_entities[FAKE_MAX_ENTITIES];
static uint32_t g_entity_count = 0;
static uint32_t g_query_cursor = 0;
static int g_query_token;

ECSComponentID asset_instance_component_get_id(World *world) {
  (void)world;
  return 7;
}

ECSComponentID transform_component_get_id(World *world) {
  (void)world;
  return 9;
}

Query *ecs_query_create(World *world, const QueryDesc *desc) {
  (void)world;
  (void)desc;
  g_query_cursor = 0;
  return (Query *)&g_query_token;
}

void ecs_query_destroy(World *world, Query *query) {
  (void)world;
  (void)query;
}

bool ecs_query_next(Query *query, Entity *entity, void **components) {
  (void)query;
  (void)entity;
  if (g_query_cursor >= g_entity_count) {
    return false;
  }
  FakeEntity *e = &g_entities[g_query_cursor++];
  components[0] = &e->asset;
  components[1] = &e->transform;
  return true;
}

/* ---- Asset manager stub: instance id -> AssetInstance ---- */

#define FAKE_MAX_INSTANCES 8
static AssetInstance g_instances[FAKE_MAX_INSTANCES];
static bool g_instance_known[FAKE_MAX_INSTANCES];

AssetInstance *asset_manager_get_instance(AssetManager *manager, uint32_t instance_id) {
  (void)manager;
  if (instance_id >= FAKE_MAX_INSTANCES || !g_instance_known[instance_id]) {
    return NULL;
  }
  return &g_instances[instance_id];
}

/* ---- Recording renderer ---- */

typedef struct {
  const Asset *asset;
  float transform[16];
  uint32_t render_flags;
  void *user_data;
} Submission;

#define FAKE_MAX_SUBMISSIONS 16
static Submission g_submissions[FAKE_MAX_SUBMISSIONS];
static uint32_t g_submission_count = 0;

static void record_submission(const Asset *asset, const float transform[16],
                              uint32_t render_flags, void *user_data) {
  if (g_submission_count < FAKE_MAX_SUBMISSIONS) {
    Submission *s = &g_submissions[g_submission_count];
    s->asset = asset;
    memcpy(s->transform, transform, sizeof(s->transform));
    s->render_flags = render_flags;
    s->user_data = user_data;
  }
  g_submission_count++;
}

/* ---- Harness ---- */

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond)                                                   \
  do {                                                                \
    g_checks++;                                                       \
    if (!(cond)) {                                                    \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      g_failures++;                                                   \
    }                                                                 \
  } while (0)

/* The unit never dereferences assets, so distinct addresses identify them. */
static char g_asset_a;
static char g_asset_b;
#define ASSET_A ((Asset *)&g_asset_a)
#define ASSET_B ((Asset *)&g_asset_b)

static int g_world_token;
static int g_manager_token;
static int g_user_token;
#define TEST_WORLD ((World *)&g_world_token)
#define TEST_MANAGER ((AssetManager *)&g_manager_token)

static void reset_fixture(void) {
  memset(g_entities, 0, sizeof(g_entities));
  memset(g_instances, 0, sizeof(g_instances));
  memset(g_instance_known, 0, sizeof(g_instance_known));
  memset(g_submissions, 0, sizeof(g_submissions));
  g_entity_count = 0;
  g_submission_count = 0;
}

static void add_instance(uint32_t id, Asset *asset) {
  g_instance_known[id] = true;
  g_instances[id].instance_id = id;
  g_instances[id].source_asset = asset;
}

static Vec3 vec3_of(float x, float y, float z) {
  Vec3 v = {0};
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

static Quat quat_of(float x, float y, float z, float w) {
  Quat q = {0};
  q.x = x;
  q.y = y;
  q.z = z;
  q.w = w;
  return q;
}

static void add_entity(uint32_t instance_id, uint32_t render_flags, Vec3 position,
                       Quat rotation, Vec3 scale) {
  FakeEntity *e = &g_entities[g_entity_count++];
  e->asset.asset_instance_id = instance_id;
  e->asset.render_flags = render_flags;
  e->asset.lod_bias = 1.0f;
  e->transform.position = position;
  e->transform.rotation = rotation;
  e->transform.scale = scale;
}

static bool matrix_near(const float a[16], const float b[16]) {
  for (int i = 0; i < 16; i++) {
    if (fabsf(a[i] - b[i]) > 1e-5f) {
      return false;
    }
  }
  return true;
}

/* Runs one update with the recording callback registered, then shuts down. */
static void run_update_with_callback(void) {
  asset_render_system_init(TEST_WORLD, TEST_MANAGER);
  asset_render_system_set_submit_callback(record_submission, &g_user_token);
  asset_render_system_update(1.0f / 60.0f);
  asset_render_system_shutdown();
}

static void test_visible_instance_submitted_with_trs_matrix(void) {
  reset_fixture();
  add_instance(1, ASSET_A);
  add_entity(1, RENDER_FLAG_VISIBLE, vec3_of(1, 2, 3), quat_of(0, 0, 0, 1),
             vec3_of(2, 3, 4));

  run_update_with_callback();

  const float expected[16] = {2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4, 0, 1, 2, 3, 1};
  CHECK(g_submission_count == 1);
  if (g_submission_count == 1) {
    CHECK(g_submissions[0].asset == ASSET_A);
    CHECK(g_submissions[0].render_flags == RENDER_FLAG_VISIBLE);
    CHECK(g_submissions[0].user_data == &g_user_token);
    CHECK(matrix_near(g_submissions[0].transform, expected));
  }
}

static void test_rotation_and_scale_compose_before_translation(void) {
  reset_fixture();
  add_instance(1, ASSET_A);
  const float h = sqrtf(0.5f); /* 90 degrees about Z */
  add_entity(1, RENDER_FLAG_VISIBLE, vec3_of(5, 0, 0), quat_of(0, 0, h, h),
             vec3_of(2, 3, 1));

  run_update_with_callback();

  /* R * S: column 0 = 2 * (0, 1, 0), column 1 = 3 * (-1, 0, 0), column 2 = (0, 0, 1). */
  const float expected[16] = {0, 2, 0, 0, -3, 0, 0, 0, 0, 0, 1, 0, 5, 0, 0, 1};
  CHECK(g_submission_count == 1);
  if (g_submission_count == 1) {
    CHECK(matrix_near(g_submissions[0].transform, expected));
  }
}

static void test_hidden_instance_not_submitted(void) {
  reset_fixture();
  add_instance(1, ASSET_A);
  add_entity(1, RENDER_FLAG_CAST_SHADOW, vec3_of(0, 0, 0), quat_of(0, 0, 0, 1),
             vec3_of(1, 1, 1));

  run_update_with_callback();

  CHECK(g_submission_count == 0);
}

static void test_each_visible_instance_submitted_in_order(void) {
  reset_fixture();
  add_instance(1, ASSET_A);
  add_instance(2, ASSET_B);
  add_entity(1, RENDER_FLAG_VISIBLE, vec3_of(0, 0, 0), quat_of(0, 0, 0, 1),
             vec3_of(1, 1, 1));
  add_entity(1, 0, vec3_of(0, 0, 0), quat_of(0, 0, 0, 1), vec3_of(1, 1, 1));
  add_entity(2, RENDER_FLAG_VISIBLE | RENDER_FLAG_CAST_SHADOW, vec3_of(0, 0, 0),
             quat_of(0, 0, 0, 1), vec3_of(1, 1, 1));

  run_update_with_callback();

  CHECK(g_submission_count == 2);
  if (g_submission_count == 2) {
    CHECK(g_submissions[0].asset == ASSET_A);
    CHECK(g_submissions[1].asset == ASSET_B);
    CHECK(g_submissions[1].render_flags == (RENDER_FLAG_VISIBLE | RENDER_FLAG_CAST_SHADOW));
  }
}

static void test_unlinked_instances_not_submitted(void) {
  reset_fixture();
  add_instance(3, NULL); /* registered, but no source asset */
  add_entity(2, RENDER_FLAG_VISIBLE, vec3_of(0, 0, 0), quat_of(0, 0, 0, 1),
             vec3_of(1, 1, 1)); /* instance 2 is unknown, so the lookup is NULL */
  add_entity(3, RENDER_FLAG_VISIBLE, vec3_of(0, 0, 0), quat_of(0, 0, 0, 1),
             vec3_of(1, 1, 1));

  run_update_with_callback();

  CHECK(g_submission_count == 0);
}

static void test_no_callback_is_safe(void) {
  reset_fixture();
  add_instance(1, ASSET_A);
  add_entity(1, RENDER_FLAG_VISIBLE, vec3_of(0, 0, 0), quat_of(0, 0, 0, 1),
             vec3_of(1, 1, 1));

  asset_render_system_init(TEST_WORLD, TEST_MANAGER);
  asset_render_system_update(1.0f / 60.0f);
  asset_render_system_shutdown();

  CHECK(g_submission_count == 0);
}

static void test_shutdown_clears_callback(void) {
  reset_fixture();
  add_instance(1, ASSET_A);
  add_entity(1, RENDER_FLAG_VISIBLE, vec3_of(0, 0, 0), quat_of(0, 0, 0, 1),
             vec3_of(1, 1, 1));

  asset_render_system_init(TEST_WORLD, TEST_MANAGER);
  asset_render_system_set_submit_callback(record_submission, &g_user_token);
  asset_render_system_shutdown();

  /* Re-initialised without re-registering: nothing may be submitted. */
  asset_render_system_init(TEST_WORLD, TEST_MANAGER);
  asset_render_system_update(1.0f / 60.0f);
  asset_render_system_shutdown();

  CHECK(g_submission_count == 0);
}

int main(void) {
  test_visible_instance_submitted_with_trs_matrix();
  test_rotation_and_scale_compose_before_translation();
  test_hidden_instance_not_submitted();
  test_each_visible_instance_submitted_in_order();
  test_unlinked_instances_not_submitted();
  test_no_callback_is_safe();
  test_shutdown_clears_callback();

  if (g_failures != 0) {
    printf("asset_render_system_test: %d of %d checks failed\n", g_failures, g_checks);
    return 1;
  }
  printf("asset_render_system_test: all %d checks passed\n", g_checks);
  return 0;
}

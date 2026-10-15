/* Proves the ECS world stubs in src/engine/ecs/ecs.c do real work. Issue #184.
 * Covers the archetype count, world save/load round trips and system
 * destruction. Compiles ecs.c directly because add_engine_test links Engine,
 * which does not build on origin/main. The logger is stubbed here.
 */

#include "core/logging/unified_logger.h"
#include "ecs/ecs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct {
  float x;
  float y;
} Position;

typedef struct {
  int hp;
} Health;

static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

static void register_components(World *world, ECSComponentID *position,
                                ECSComponentID *health) {
  *position =
      ecs_register_component_simple(world, "Position", sizeof(Position));
  *health = ecs_register_component_simple(world, "Health", sizeof(Health));
}

static void test_archetype_count_tracks_component_sets(void) {
  World *world = ecs_world_create(NULL);
  ECSComponentID position, health;
  register_components(world, &position, &health);

  EXPECT(ecs_world_get_stats(world).archetype_count == 1,
         "new world has only the empty archetype");

  Entity a = ecs_create_entity(world);
  Position pa = {1.0f, 1.0f};
  EXPECT(ecs_add_component(world, a, position, &pa), "add Position to a");
  EXPECT(ecs_world_get_stats(world).archetype_count == 2,
         "first component set adds one archetype");

  Entity b = ecs_create_entity(world);
  EXPECT(ecs_add_component(world, b, position, &pa), "add Position to b");
  EXPECT(ecs_world_get_stats(world).archetype_count == 2,
         "same component set reuses its archetype");

  Health ha = {10};
  EXPECT(ecs_add_component(world, a, health, &ha), "add Health to a");
  EXPECT(ecs_world_get_stats(world).archetype_count == 3,
         "Position plus Health adds a third archetype");

  ecs_world_destroy(world);
}

static void test_save_load_round_trip_restores_entities(void) {
  const char *path = "ecs_world_state_round_trip.bin";
  World *source = ecs_world_create(NULL);
  ECSComponentID position, health;
  register_components(source, &position, &health);

  Entity e1 = ecs_create_entity(source);
  Position p1 = {1.0f, 2.0f};
  Health h1 = {50};
  EXPECT(ecs_add_component(source, e1, position, &p1), "add Position to e1");
  EXPECT(ecs_add_component(source, e1, health, &h1), "add Health to e1");

  Entity e2 = ecs_create_entity(source);
  Position p2 = {3.0f, 4.0f};
  EXPECT(ecs_add_component(source, e2, position, &p2), "add Position to e2");

  Entity e3 = ecs_create_entity(source);

  Entity e4 = ecs_create_entity(source);
  Health h4 = {9};
  EXPECT(ecs_add_component(source, e4, health, &h4), "add Health to e4");
  ecs_destroy_entity(source, e4);

  EXPECT(ecs_world_save(source, path), "save succeeds");
  ecs_world_destroy(source);

  World *loaded = ecs_world_create(NULL);
  register_components(loaded, &position, &health);
  ecs_create_entity(loaded); /* load must replace this, not merge with it */
  EXPECT(ecs_world_load(loaded, path), "load succeeds");
  EXPECT(ecs_world_get_stats(loaded).entity_count == 3,
         "load replaces existing entities with the three saved ones");
  EXPECT(ecs_is_valid(loaded, e1) && ecs_is_valid(loaded, e2) &&
             ecs_is_valid(loaded, e3),
         "live handles keep their id and generation");
  EXPECT(!ecs_is_valid(loaded, e4), "destroyed handle stays invalid");

  Position *q1 = ecs_get_component(loaded, e1, position);
  EXPECT(q1 && q1->x == 1.0f && q1->y == 2.0f, "e1 Position restored");
  Health *g1 = ecs_get_component(loaded, e1, health);
  EXPECT(g1 && g1->hp == 50, "e1 Health restored");
  Position *q2 = ecs_get_component(loaded, e2, position);
  EXPECT(q2 && q2->x == 3.0f && q2->y == 4.0f, "e2 Position restored");
  EXPECT(ecs_get_component(loaded, e2, health) == NULL,
         "e2 gained no Health");
  EXPECT(ecs_get_component(loaded, e3, position) == NULL,
         "e3 has no components");

  Entity fresh = ecs_create_entity(loaded);
  EXPECT(ecs_is_valid(loaded, fresh), "new entity is valid after load");
  EXPECT(ecs_is_valid(loaded, e1) && ecs_is_valid(loaded, e2) &&
             ecs_is_valid(loaded, e3),
         "new allocation does not disturb live entities");
  EXPECT(!ecs_is_valid(loaded, e4),
         "old handle stays invalid after its id is reused");

  ecs_world_destroy(loaded);
  remove(path);
}

static void test_load_missing_file_fails(void) {
  World *world = ecs_world_create(NULL);
  EXPECT(!ecs_world_load(world, "ecs_world_state_missing/none.bin"),
         "loading a missing file fails");
  ecs_world_destroy(world);
}

static void test_load_rejects_different_registry(void) {
  const char *path = "ecs_world_state_registry.bin";
  World *source = ecs_world_create(NULL);
  ECSComponentID position, health;
  register_components(source, &position, &health);
  Entity e = ecs_create_entity(source);
  Position p = {5.0f, 6.0f};
  EXPECT(ecs_add_component(source, e, position, &p), "add Position");
  EXPECT(ecs_world_save(source, path), "save succeeds");
  ecs_world_destroy(source);

  /* Component 0 is Position in the file but Velocity here. */
  World *other = ecs_world_create(NULL);
  ecs_register_component_simple(other, "Velocity", sizeof(Position));
  ecs_register_component_simple(other, "Position", sizeof(Position));
  EXPECT(!ecs_world_load(other, path),
         "load rejects a different component layout");
  EXPECT(ecs_world_get_stats(other).entity_count == 0,
         "rejected load leaves the world unchanged");
  ecs_world_destroy(other);
  remove(path);
}

static void test_load_truncated_file_leaves_world_unchanged(void) {
  const char *path = "ecs_world_state_truncated.bin";
  World *source = ecs_world_create(NULL);
  ECSComponentID position, health;
  register_components(source, &position, &health);
  Entity e = ecs_create_entity(source);
  Position p = {7.0f, 8.0f};
  EXPECT(ecs_add_component(source, e, position, &p), "add Position");
  EXPECT(ecs_world_save(source, path), "save succeeds");
  ecs_world_destroy(source);

  /* Drop the final byte so the image ends inside the component data. */
  FILE *f = fopen(path, "rb");
  EXPECT(f != NULL, "saved file can be reopened");
  EXPECT(fseek(f, 0, SEEK_END) == 0, "seek to end of saved file");
  long length = ftell(f);
  rewind(f);
  unsigned char *bytes = malloc((size_t)length);
  EXPECT(bytes && fread(bytes, 1, (size_t)length, f) == (size_t)length,
         "read saved image");
  fclose(f);
  f = fopen(path, "wb");
  EXPECT(f != NULL && fwrite(bytes, 1, (size_t)length - 1, f) ==
                          (size_t)length - 1,
         "write truncated image");
  fclose(f);
  free(bytes);

  World *loaded = ecs_world_create(NULL);
  register_components(loaded, &position, &health);
  Entity kept = ecs_create_entity(loaded);
  EXPECT(!ecs_world_load(loaded, path), "truncated image is rejected");
  EXPECT(ecs_world_get_stats(loaded).entity_count == 1 &&
             ecs_is_valid(loaded, kept),
         "rejected load leaves the existing world unchanged");
  ecs_world_destroy(loaded);
  remove(path);
}

static void test_save_to_missing_directory_fails(void) {
  World *world = ecs_world_create(NULL);
  EXPECT(!ecs_world_save(world, "ecs_world_state_missing/out.bin"),
         "saving into a missing directory fails");
  ecs_world_destroy(world);
}

static int g_runs[4];

static void run_first(SystemContext *ctx) {
  (void)ctx;
  g_runs[0]++;
}

static void run_second(SystemContext *ctx) {
  (void)ctx;
  g_runs[1]++;
}

static void run_third(SystemContext *ctx) {
  (void)ctx;
  g_runs[2]++;
}

static void run_fourth(SystemContext *ctx) {
  (void)ctx;
  g_runs[3]++;
}

static void test_system_destroy_stops_execution_and_frees_slot(void) {
  memset(g_runs, 0, sizeof g_runs);
  World *world = ecs_world_create(NULL);
  System *first = ecs_system_create(world, "first", run_first, NULL);
  System *second = ecs_system_create(world, "second", run_second, NULL);
  System *third = ecs_system_create(world, "third", run_third, NULL);
  EXPECT(first && second && third, "create three systems");

  ecs_system_destroy(world, second);
  ecs_system_destroy(world, second);
  ecs_system_destroy(world, NULL);
  EXPECT(ecs_world_get_stats(world).system_count == 2,
         "destroy removes one system and a repeat destroy is a no-op");

  ecs_world_update(world, 0.016f);
  EXPECT(g_runs[0] == 1 && g_runs[1] == 0 && g_runs[2] == 1,
         "destroyed system does not run, the others do");

  ecs_system_set_enabled(world, third, false);
  ecs_world_update(world, 0.016f);
  EXPECT(g_runs[2] == 1, "disabled system stays skipped");
  ecs_system_set_enabled(world, third, true);

  System *fourth = ecs_system_create(world, "fourth", run_fourth, NULL);
  EXPECT(fourth != NULL, "destroyed slot can be reused");
  ecs_world_update(world, 0.016f);
  EXPECT(g_runs[1] == 0 && g_runs[3] == 1,
         "reused slot runs only the new system");
  EXPECT(ecs_world_get_stats(world).system_count == 3,
         "system count is back to three");

  ecs_world_destroy(world);
}

int main(void) {
  test_archetype_count_tracks_component_sets();
  test_save_load_round_trip_restores_entities();
  test_load_missing_file_fails();
  test_load_rejects_different_registry();
  test_load_truncated_file_leaves_world_unchanged();
  test_save_to_missing_directory_fails();
  test_system_destroy_stops_execution_and_frees_slot();

  if (g_failed) {
    fprintf(stderr, "%d check(s) failed\n", g_failed);
    return 1;
  }
  printf("ecs_world_state: all checks passed\n");
  return 0;
}

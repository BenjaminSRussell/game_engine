/* Unit tests for src/engine/ecs/mass_system.c (issue #331).
 *
 * Compiles the unit directly, not Engine: add_engine_test links Engine, which
 * does not build on main. Registered in tests/registry/test_mass_system.cmake.
 */

#include "ecs/mass_system.h"
#include "math/quat.h"
#include "math/vec3.h"

#include <math.h>
#include <stdio.h>

static int g_failed = 0;

/* Each test returns at its first failed check, so later checks never read a
 * NULL transform. */
#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

static int near_f(float a, float b) { return fabsf(a - b) <= 1e-5f; }

static int near_vec(Vec3 a, Vec3 b) {
  return near_f(a.x, b.x) && near_f(a.y, b.y) && near_f(a.z, b.z);
}

static int is_identity(Quat q) {
  return near_f(q.w, 1.0f) && near_f(q.x, 0.0f) && near_f(q.y, 0.0f) &&
         near_f(q.z, 0.0f);
}

/* True when q is a unit quaternion whose forward axis points along dir. */
static int faces(Quat q, Vec3 dir) {
  float len_sq = q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
  return near_f(len_sq, 1.0f) &&
         near_vec(quat_rotate_vec3(q, vec3_forward()), vec3_normalize(dir));
}

static void test_spawn_initialises_defaults(void) {
  mass_system_init(4);
  Vec3 start = vec3_create(1.0f, 2.0f, 3.0f);
  Vec3 velocity = vec3_create(0.5f, 0.0f, 0.0f);
  MassEntity e0 = mass_entity_spawn(start, velocity);
  MassEntity e1 = mass_entity_spawn(vec3_zero(), vec3_zero());
  EXPECT(e0 == 0 && e1 == 1, "ids should follow spawn order from 0");

  const MassFragment_Transform *t = mass_entity_get_transform(e0);
  EXPECT(t != NULL, "spawned entity should have a transform");
  EXPECT(near_vec(t->position, start), "spawn should store the position");
  EXPECT(near_vec(t->velocity, velocity), "spawn should store the velocity");
  EXPECT(near_f(t->scale, 1.0f), "scale should default to 1");
  EXPECT(is_identity(t->rotation), "rotation should start as identity");
  EXPECT(mass_entity_get_transform(2) == NULL,
         "an unspawned id should have no transform");
  mass_system_shutdown();
}

static void test_spawn_respects_capacity(void) {
  mass_system_init(2);
  EXPECT(mass_entity_spawn(vec3_zero(), vec3_zero()) == 0, "first spawn");
  EXPECT(mass_entity_spawn(vec3_zero(), vec3_zero()) == 1, "second spawn");
  EXPECT(mass_entity_spawn(vec3_zero(), vec3_zero()) == 0xFFFFFFFFu,
         "a spawn past capacity should return the full sentinel");
  EXPECT(mass_entity_get_transform(2) == NULL,
         "a failed spawn should not create an entity");
  mass_system_shutdown();
}

static void test_default_target_keeps_entity_still(void) {
  mass_system_init(1);
  Vec3 start = vec3_create(3.0f, 0.0f, -2.0f);
  MassEntity e = mass_entity_spawn(start, vec3_zero());
  for (int i = 0; i < 10; i++)
    mass_system_update(0.1f);

  const MassFragment_Transform *t = mass_entity_get_transform(e);
  EXPECT(t != NULL, "entity should keep its transform");
  EXPECT(near_vec(t->position, start),
         "an entity with no target should stay where it spawned");
  EXPECT(near_vec(t->velocity, vec3_zero()),
         "an entity with no target should have no velocity");
  EXPECT(is_identity(t->rotation),
         "a stationary entity should keep identity rotation");
  mass_system_shutdown();
}

static void test_set_target_steers_entity(void) {
  mass_system_init(1);
  MassEntity e = mass_entity_spawn(vec3_zero(), vec3_zero());
  mass_entity_set_target(e, vec3_create(10.0f, 0.0f, 0.0f));
  mass_system_update(0.1f);

  /* Desired speed is move_speed (1). Blend = dt * 5 = 0.5, so velocity is
   * halfway to desired, and position advances by velocity * dt. */
  const MassFragment_Transform *t = mass_entity_get_transform(e);
  EXPECT(t != NULL, "entity should keep its transform");
  EXPECT(near_vec(t->velocity, vec3_create(0.5f, 0.0f, 0.0f)),
         "velocity should blend halfway toward the desired velocity");
  EXPECT(near_vec(t->position, vec3_create(0.05f, 0.0f, 0.0f)),
         "position should integrate the blended velocity");
  mass_system_shutdown();
}

static void test_set_target_ignores_unknown_entity(void) {
  mass_system_init(1);
  MassEntity e = mass_entity_spawn(vec3_zero(), vec3_zero());
  mass_entity_set_target(7, vec3_create(10.0f, 0.0f, 0.0f));
  mass_system_update(0.1f);

  const MassFragment_Transform *t = mass_entity_get_transform(e);
  EXPECT(t != NULL, "entity should keep its transform");
  EXPECT(near_vec(t->velocity, vec3_zero()),
         "setting a target on an unknown id should not move entity 0");
  mass_system_shutdown();
}

static void test_converges_on_target(void) {
  mass_system_init(1);
  Vec3 target = vec3_create(10.0f, 0.0f, 0.0f);
  MassEntity e = mass_entity_spawn(vec3_zero(), vec3_zero());
  mass_entity_set_target(e, target);
  for (int i = 0; i < 600; i++) /* 30 s of 50 ms steps */
    mass_system_update(0.05f);

  const MassFragment_Transform *t = mass_entity_get_transform(e);
  EXPECT(t != NULL, "entity should keep its transform");
  EXPECT(vec3_distance(t->position, target) < 0.25f,
         "entity should reach its target and settle near it");
  mass_system_shutdown();
}

static void test_long_frames_do_not_overshoot(void) {
  mass_system_init(1);
  Vec3 target = vec3_create(10.0f, 0.0f, 0.0f);
  MassEntity e = mass_entity_spawn(vec3_zero(), vec3_zero());
  mass_entity_set_target(e, target);

  /* A 1 s frame would blend 5x toward desired without a clamp, which
   * diverges. Each step must keep the speed at or below move_speed. */
  for (int i = 0; i < 50; i++) {
    mass_system_update(1.0f);
    const MassFragment_Transform *t = mass_entity_get_transform(e);
    EXPECT(t != NULL, "entity should keep its transform");
    EXPECT(vec3_length(t->velocity) <= 1.0f + 1e-5f,
           "speed must not exceed move_speed on a long frame");
  }
  const MassFragment_Transform *t = mass_entity_get_transform(e);
  EXPECT(vec3_distance(t->position, target) < 0.1f,
         "entity should arrive at its target on long frames");
  mass_system_shutdown();
}

static void test_one_long_frame_advances_at_move_speed(void) {
  mass_system_init(1);
  MassEntity e = mass_entity_spawn(vec3_zero(), vec3_zero());
  mass_entity_set_target(e, vec3_create(10.0f, 0.0f, 0.0f));
  mass_system_update(1.0f);

  const MassFragment_Transform *t = mass_entity_get_transform(e);
  EXPECT(t != NULL, "entity should keep its transform");
  EXPECT(near_vec(t->velocity, vec3_create(1.0f, 0.0f, 0.0f)),
         "velocity should reach desired speed, not overshoot it");
  EXPECT(near_vec(t->position, vec3_create(1.0f, 0.0f, 0.0f)),
         "one second at move_speed should cover 1 unit");
  mass_system_shutdown();
}

static void test_zero_dt_changes_nothing(void) {
  mass_system_init(1);
  MassEntity e = mass_entity_spawn(vec3_create(1.0f, 1.0f, 1.0f),
                                   vec3_create(0.0f, 2.0f, 0.0f));
  mass_entity_set_target(e, vec3_create(9.0f, 9.0f, 9.0f));
  mass_system_update(0.0f);

  const MassFragment_Transform *t = mass_entity_get_transform(e);
  EXPECT(t != NULL, "entity should keep its transform");
  EXPECT(near_vec(t->position, vec3_create(1.0f, 1.0f, 1.0f)),
         "dt of zero should not move the entity");
  EXPECT(near_vec(t->velocity, vec3_create(0.0f, 2.0f, 0.0f)),
         "dt of zero should not change velocity");
  /* Moving straight up is parallel to vec3_up(), the look-rotation fallback. */
  EXPECT(faces(t->rotation, vec3_create(0.0f, 1.0f, 0.0f)),
         "rotation should face the velocity even when moving straight up");
  mass_system_shutdown();
}

static void test_facing_follows_velocity(void) {
  mass_system_init(1);
  MassEntity e = mass_entity_spawn(vec3_zero(), vec3_zero());
  mass_entity_set_target(e, vec3_create(10.0f, 0.0f, 10.0f));
  mass_system_update(0.1f);

  const MassFragment_Transform *t = mass_entity_get_transform(e);
  EXPECT(t != NULL, "entity should keep its transform");
  EXPECT(faces(t->rotation, t->velocity),
         "rotation should face the direction of travel");
  mass_system_shutdown();
}

static void test_look_rotation_faces_direction(void) {
  Vec3 up = vec3_up();
  EXPECT(faces(quat_look_rotation(vec3_create(0, 0, 1), up),
               vec3_create(0, 0, 1)),
         "+Z should map to +Z");
  EXPECT(faces(quat_look_rotation(vec3_create(0, 0, -1), up),
               vec3_create(0, 0, -1)),
         "-Z should map to -Z");
  EXPECT(faces(quat_look_rotation(vec3_create(1, 0, 0), up),
               vec3_create(1, 0, 0)),
         "+X should be faced");
  EXPECT(faces(quat_look_rotation(vec3_create(-1, 0, 0), up),
               vec3_create(-1, 0, 0)),
         "-X should be faced");
  EXPECT(faces(quat_look_rotation(vec3_create(1, 1, 1), up),
               vec3_create(1, 1, 1)),
         "a diagonal direction should be faced");
  EXPECT(faces(quat_look_rotation(vec3_create(0, 1, 0), up),
               vec3_create(0, 1, 0)),
         "a direction parallel to up should still be faced");
  EXPECT(faces(quat_look_rotation(vec3_create(0, -1, 0), up),
               vec3_create(0, -1, 0)),
         "straight down should be faced");
  EXPECT(faces(quat_look_rotation(vec3_create(1, 0, 0), vec3_create(1, 0, 0)),
               vec3_create(1, 0, 0)),
         "an up hint parallel to the direction should still give a rotation");
  EXPECT(is_identity(quat_look_rotation(vec3_zero(), up)),
         "a zero direction should give identity");
}

static void test_shutdown_is_safe_and_reusable(void) {
  mass_system_init(2);
  mass_entity_spawn(vec3_zero(), vec3_zero());
  mass_system_shutdown();
  /* A second shutdown must not free the buffers a second time. */
  mass_system_shutdown();
  EXPECT(mass_entity_get_transform(0) == NULL,
         "shutdown should leave no entities");
  EXPECT(mass_entity_spawn(vec3_zero(), vec3_zero()) == 0xFFFFFFFFu,
         "spawn after shutdown should fail until re-init");

  mass_system_init(1);
  MassEntity e = mass_entity_spawn(vec3_create(4.0f, 5.0f, 6.0f), vec3_zero());
  EXPECT(e == 0, "re-init should restart ids");
  const MassFragment_Transform *t = mass_entity_get_transform(e);
  EXPECT(t != NULL && near_vec(t->position, vec3_create(4.0f, 5.0f, 6.0f)),
         "re-init should give a working system");
  mass_system_shutdown();
}

int main(void) {
  test_spawn_initialises_defaults();
  test_spawn_respects_capacity();
  test_default_target_keeps_entity_still();
  test_set_target_steers_entity();
  test_set_target_ignores_unknown_entity();
  test_converges_on_target();
  test_long_frames_do_not_overshoot();
  test_one_long_frame_advances_at_move_speed();
  test_zero_dt_changes_nothing();
  test_facing_follows_velocity();
  test_look_rotation_faces_direction();
  test_shutdown_is_safe_and_reusable();

  if (g_failed) {
    fprintf(stderr, "mass_system: %d test(s) failed\n", g_failed);
    return 1;
  }
  printf("mass_system: all tests passed\n");
  return 0;
}

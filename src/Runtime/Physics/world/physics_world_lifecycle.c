#include <physics/core/physics_types.h>
#include <physics/physics.h>
#include <stdlib.h>

// External
void collision_system_init(void);

PhysicsWorld *physics_world_create(PhysicsConfig config) {
  PhysicsWorld *world = (PhysicsWorld *)calloc(1, sizeof(PhysicsWorld));
  if (!world)
    return NULL;

  world->body_capacity = 1024;
  world->bodies =
      (RigidBody **)calloc(world->body_capacity, sizeof(RigidBody *));
  world->constraints = (Constraint *)calloc(1024, sizeof(Constraint));

  world->gravity[0] = config.gravity.x;
  world->gravity[1] = config.gravity.y;
  world->gravity[2] = config.gravity.z;
  world->timestep =
      config.fixed_timestep > 0 ? config.fixed_timestep : 1.0f / 60.0f;
  world->velocity_iterations = config.velocity_iterations;
  world->position_iterations = config.position_iterations;

  collision_system_init();

  return world;
}

void physics_world_destroy(PhysicsWorld *world) {
  if (!world)
    return;
  if (world->bodies)
    free(world->bodies);
  if (world->constraints)
    free(world->constraints);
  free(world);
}

void physics_world_init(PhysicsWorld *world, const Vec3 *gravity) {
  if (!world)
    return;
  if (gravity) {
    world->gravity[0] = gravity->x;
    world->gravity[1] = gravity->y;
    world->gravity[2] = gravity->z;
  } else {
    world->gravity[0] = 0.0f;
    world->gravity[1] = -9.81f;
    world->gravity[2] = 0.0f;
  }
  world->timestep = 1.0f / 60.0f;
  world->velocity_iterations = 8;
  world->position_iterations = 3;
}

void physics_world_free(PhysicsWorld *world) { physics_world_destroy(world); }

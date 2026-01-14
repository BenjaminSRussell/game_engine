#include <physics/core/physics_types.h>
#include <physics/physics.h>
#include <stdio.h> // NULL

// External
void collision_system_clear_bodies(void);
void collision_system_add_body(u32 id, const AABB *bounds, Vec3 position);
void collision_system_update(f32 delta_time);
void integrate_body(RigidBody *body, f32 dt,
                    const Vec3 gravity); // From dynamics

static AABB calculate_aabb(RigidBody *body) {
  AABB bounds = {{0}};
  float rx = 0.5f, ry = 0.5f, rz = 0.5f;

  if (body->shape) {
    if (body->shape->type == COLLISION_SHAPE_SPHERE) {
      rx = ry = rz = body->shape->data.sphere.radius;
    } else if (body->shape->type == COLLISION_SHAPE_BOX) {
      rx = body->shape->data.box.half_extents[0];
      ry = body->shape->data.box.half_extents[1];
      rz = body->shape->data.box.half_extents[2];
    }
  }

  bounds.min = (Vec3){body->position[0] - rx, body->position[1] - ry,
                      body->position[2] - rz};
  bounds.max = (Vec3){body->position[0] + rx, body->position[1] + ry,
                      body->position[2] + rz};
  return bounds;
}

void physics_world_step(PhysicsWorld *world, f32 dt) {
  if (!world || world->paused)
    return;

  world->time_accumulator += dt;

  while (world->time_accumulator >= world->timestep) {
    // 1. Update Collision System
    collision_system_clear_bodies();

    // Add active bodies
    for (uint32_t i = 0; i < world->body_count; i++) {
      RigidBody *body = world->bodies[i];
      if (body && body->is_active && body->shape) {
        AABB bounds = calculate_aabb(body);
        Vec3 pos = {body->position[0], body->position[1], body->position[2]};
        collision_system_add_body(body->id, &bounds, pos);
      }
    }

    // Add Ground Plane (Static Body) - ID 999999
    {
      AABB ground_bounds = {{-1000.0f, -50.0f, -1000.0f},
                            {1000.0f, 0.0f, 1000.0f}};
      Vec3 ground_pos = {0.0f, -25.0f, 0.0f};
      collision_system_add_body(999999, &ground_bounds, ground_pos);
    }

    collision_system_update(world->timestep);

    // 2. Integrate
    Vec3 grav = {world->gravity[0], world->gravity[1], world->gravity[2]};

    for (uint32_t i = 0; i < world->body_count; i++) {
      integrate_body(world->bodies[i], world->timestep, grav);
    }

    world->time_accumulator -= world->timestep;
  }
}

RigidBody *physics_world_add_body(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body)
    return NULL;
  if (world->body_count >= world->body_capacity)
    return NULL;

  world->bodies[world->body_count++] = body;
  return body;
}

void physics_world_remove_body(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body)
    return;
  for (uint32_t i = 0; i < world->body_count; i++) {
    if (world->bodies[i] == body) {
      world->bodies[i] = world->bodies[--world->body_count];
      return;
    }
  }
}

void physics_world_add_constraint(PhysicsWorld *world, Constraint *constraint) {
  if (!world || !constraint)
    return;
  if (world->constraint_count < 1024) {
    world->constraints[world->constraint_count++] = *constraint;
  }
}

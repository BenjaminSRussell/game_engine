#include "physics/core/physics_types.h"
#include "physics/physics.h"  // For PhysicsConfig
#include <float.h>
#include <include/math/math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Forward decls of external functions
void physics_world_solve_constraints(PhysicsWorld *world, float dt); // in physics_solver.c
bool shape_raycast(CollisionShape *shape, const float *origin, const float *dir, float max_dist, float *out_hit); // in physics_shapes.c
void broadphase_init(BroadPhase *bp); // in physics_broadphase.c
void broadphase_shutdown(BroadPhase *bp);
void broadphase_insert(BroadPhase *bp, uint32_t body_id, const float *min, const float *max);
void broadphase_remove(BroadPhase *bp, uint32_t body_id);
bool broadphase_query(BroadPhase *bp, uint32_t id1, uint32_t id2);
// Narrowphase stubs for now
bool collision_detect_pair(RigidBody *a, RigidBody *b, ContactManifold *m);
void physics_world_step_fixed(PhysicsWorld *world, float dt);

/* =================================================================================================
 *                                    INTERNAL HELPERS
 * =================================================================================================
 */

static void pvec3_zero(float *v) { v[0] = v[1] = v[2] = 0.0f; }
static void pvec3_copy(float *dst, const float *src) {
  memcpy(dst, src, 3 * sizeof(float));
}
static void pvec3_add(float *dst, const float *a, const float *b) {
  dst[0] = a[0] + b[0];
  dst[1] = a[1] + b[1];
  dst[2] = a[2] + b[2];
}
static void pvec3_scale(float *dst, const float *v, float s) {
  dst[0] = v[0] * s;
  dst[1] = v[1] * s;
  dst[2] = v[2] * s;
}

static void quat_mul_vec(const float *q, const float *v, float *out) {
  float qx = q[0], qy = q[1], qz = q[2], qw = q[3];
  out[0] = qw * v[0] + qy * v[2] - qz * v[1];
  out[1] = qw * v[1] + qz * v[0] - qx * v[2];
  out[2] = qw * v[2] + qx * v[1] - qy * v[0];
  out[3] = -(qx * v[0] + qy * v[1] + qz * v[2]);
}

RigidBody *physics_world_get_body(PhysicsWorld *world, uint32_t id) {
  if (!world) return NULL;
  for (uint32_t i = 0; i < world->body_count; i++) {
    if (world->bodies[i] && world->bodies[i]->id == id) {
      return world->bodies[i];
    }
  }
  return NULL;
}

void core_rigid_body_get_aabb(RigidBody *body, float *min_out, float *max_out) {
  if (!body || !body->shape) {
    pvec3_zero(min_out);
    pvec3_zero(max_out);
    return;
  }
  min_out[0] = body->position[0] + body->shape->bounds_min[0];
  min_out[1] = body->position[1] + body->shape->bounds_min[1];
  min_out[2] = body->position[2] + body->shape->bounds_min[2];
  max_out[0] = body->position[0] + body->shape->bounds_max[0];
  max_out[1] = body->position[1] + body->shape->bounds_max[1];
  max_out[2] = body->position[2] + body->shape->bounds_max[2];
}

static void physics_world_update_broadphase(PhysicsWorld *world) {
  if (!world) return;
  world->broadphase.node_count = 0;
  float aabb_min[3], aabb_max[3];
  for (uint32_t i = 0; i < world->body_count; i++) {
      RigidBody *body = world->bodies[i];
      if (body && body->is_active && body->shape) {
          core_rigid_body_get_aabb(body, aabb_min, aabb_max);
          broadphase_insert(&world->broadphase, body->id, aabb_min, aabb_max);
      }
  }
}

// Stub for now, implemented in narrowphase
bool collision_detect_pair(RigidBody *a, RigidBody *b, ContactManifold *m) {
    // Placeholder - always return false
    return false;
}

void physics_world_find_contacts(PhysicsWorld *world) {
  if (!world) return;
  world->contact_count = 0;
  for (uint32_t i = 0; i < world->body_count; i++) {
    RigidBody *body_a = world->bodies[i];
    if (!body_a->is_active || !body_a->shape) continue;
    for (uint32_t j = i + 1; j < world->body_count; j++) {
      RigidBody *body_b = world->bodies[j];
      if (!body_b->is_active || !body_b->shape) continue;
      if (body_a->is_sleeping && body_b->is_sleeping) continue;
      if (body_a->type == RIGID_BODY_STATIC && body_b->type == RIGID_BODY_STATIC) continue;
      if (!broadphase_query(&world->broadphase, body_a->id, body_b->id)) continue; 

      ContactManifold manifold;
      manifold.point_count = 0;
      if (collision_detect_pair(body_a, body_b, &manifold)) {
        if (world->contact_count < MAX_CONTACTS) {
          world->contacts[world->contact_count++] = manifold;
        }
      }
    }
  }
}

void physics_world_integrate_velocities(PhysicsWorld *world, float dt) {
  if (!world) return;
  for (uint32_t i = 0; i < world->body_count; i++) {
    RigidBody *body = world->bodies[i];
    if (!body || !body->is_active || body->is_sleeping) continue;
    if (body->type != RIGID_BODY_DYNAMIC) continue;

    float accel[3] = {body->accumulated_force[0] * body->inv_mass,
                      body->accumulated_force[1] * body->inv_mass,
                      body->accumulated_force[2] * body->inv_mass};
    body->velocity[0] += accel[0] * dt;
    body->velocity[1] += accel[1] * dt;
    body->velocity[2] += accel[2] * dt;

    float damping = powf(1.0f - body->linear_damping, dt);
    body->velocity[0] *= damping; body->velocity[1] *= damping; body->velocity[2] *= damping;

    // Angular (simplified)
    body->angular_velocity[0] += body->accumulated_torque[0] * body->inv_mass * dt;
    body->angular_velocity[1] += body->accumulated_torque[1] * body->inv_mass * dt;
    body->angular_velocity[2] += body->accumulated_torque[2] * body->inv_mass * dt;
    float ang_damping = powf(1.0f - body->angular_damping, dt);
    body->angular_velocity[0] *= ang_damping; body->angular_velocity[1] *= ang_damping; body->angular_velocity[2] *= ang_damping;
  }
}

void physics_world_integrate_positions(PhysicsWorld *world, float dt) {
  if (!world) return;
  for (uint32_t i = 0; i < world->body_count; i++) {
    RigidBody *body = world->bodies[i];
    if (!body || !body->is_active || body->is_sleeping) continue;
    if (body->type == RIGID_BODY_STATIC) continue;
    
    body->position[0] += body->velocity[0] * dt;
    body->position[1] += body->velocity[1] * dt;
    body->position[2] += body->velocity[2] * dt;

    float omega[4] = {body->angular_velocity[0], body->angular_velocity[1], body->angular_velocity[2], 0.0f};
    float dq[4];
    quat_mul_vec(body->rotation, omega, dq);
    body->rotation[0] += dq[0] * 0.5f * dt;
    body->rotation[1] += dq[1] * 0.5f * dt;
    body->rotation[2] += dq[2] * 0.5f * dt;
    body->rotation[3] += dq[3] * 0.5f * dt;
    
    float len = sqrtf(body->rotation[0]*body->rotation[0] + body->rotation[1]*body->rotation[1] + 
                      body->rotation[2]*body->rotation[2] + body->rotation[3]*body->rotation[3]);
    if (len > 0.0f) {
      body->rotation[0] /= len; body->rotation[1] /= len; body->rotation[2] /= len; body->rotation[3] /= len;
    }
  }
}

void core_rigid_body_check_sleeping(RigidBody *body, float dt) {
    // Stub
}

void physics_world_solve_positions(PhysicsWorld *world) {
    // Stub for penetration resolution
}

PhysicsWorld *physics_world_create(PhysicsConfig config) {
  PhysicsWorld *world = (PhysicsWorld *)calloc(1, sizeof(PhysicsWorld));
  if (!world) return NULL;
  world->body_capacity = MAX_PHYSICS_BODIES;
  world->bodies = (RigidBody **)calloc(MAX_PHYSICS_BODIES, sizeof(RigidBody*));
  world->constraints = (Constraint *)calloc(MAX_CONSTRAINTS, sizeof(Constraint));
  world->contacts = (ContactManifold *)calloc(MAX_CONTACTS, sizeof(ContactManifold));
  if (!world->bodies || !world->constraints || !world->contacts) {
      // free...
      return NULL;
  }
  world->gravity[0] = config.gravity.x; world->gravity[1] = config.gravity.y; world->gravity[2] = config.gravity.z;
  world->timestep = config.fixed_timestep;
  world->velocity_iterations = config.velocity_iterations > 0 ? config.velocity_iterations : 10;
  world->position_iterations = config.position_iterations > 0 ? config.position_iterations : 5;
  broadphase_init(&world->broadphase);
  return world;
}

void physics_world_destroy(PhysicsWorld *world) {
  if (!world) return;
  broadphase_shutdown(&world->broadphase);
  if (world->bodies) {
      for (uint32_t i=0; i<world->body_count; i++) {
          if(world->bodies[i]) free(world->bodies[i]);
      }
      free(world->bodies);
  }
  if (world->constraints) free(world->constraints);
  if (world->contacts) free(world->contacts);
  free(world);
}

void physics_world_step(PhysicsWorld *world, float dt) {
  if (!world || dt <= 0.0f) return;
  static float accumulator = 0.0f;
  const float fixed_dt = 1.0f / 60.0f;
  accumulator += dt;
  while (accumulator >= fixed_dt) {
    physics_world_step_fixed(world, fixed_dt);
    accumulator -= fixed_dt;
  }
}

void physics_world_step_fixed(PhysicsWorld *world, float dt) {
  if (!world) return;
  // Apply forces
  for (uint32_t i = 0; i < world->body_count; i++) {
    RigidBody *body = world->bodies[i];
    if (!body || !body->is_active || body->is_sleeping) continue;
    if (body->type != RIGID_BODY_DYNAMIC) continue;
    body->accumulated_force[0] += world->gravity[0] * body->mass;
    body->accumulated_force[1] += world->gravity[1] * body->mass;
    body->accumulated_force[2] += world->gravity[2] * body->mass;
  }
  physics_world_integrate_velocities(world, dt);
  physics_world_update_broadphase(world);
  physics_world_find_contacts(world);
  physics_world_solve_constraints(world, dt);
  physics_world_integrate_positions(world, dt);
  physics_world_solve_positions(world);
  // Clear forces
  for (uint32_t i = 0; i < world->body_count; i++) {
    RigidBody *body = world->bodies[i];
    if (body) {
        body->accumulated_force[0] = 0.0f; body->accumulated_force[1] = 0.0f; body->accumulated_force[2] = 0.0f;
        body->accumulated_torque[0] = 0.0f; body->accumulated_torque[1] = 0.0f; body->accumulated_torque[2] = 0.0f;
    }
  }
}

RigidBody *physics_world_add_body(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body || world->body_count >= MAX_PHYSICS_BODIES) return NULL;
  world->bodies[world->body_count++] = body;
  float aabb_min[3], aabb_max[3];
  core_rigid_body_get_aabb(body, aabb_min, aabb_max);
  broadphase_insert(&world->broadphase, body->id, aabb_min, aabb_max);
  return body;
}

void physics_world_remove_body(PhysicsWorld *world, RigidBody *body) {
  if (!world || !body) return;
  for (uint32_t i = 0; i < world->body_count; i++) {
    if (world->bodies[i] && world->bodies[i]->id == body->id) {
      broadphase_remove(&world->broadphase, body->id);
      world->bodies[i] = world->bodies[world->body_count - 1];
      world->bodies[world->body_count - 1] = NULL;
      world->body_count--;
      return;
    }
  }
}

bool core_physics_world_raycast(PhysicsWorld *world, const float *origin, const float *dir, float max_dist, RayHit *out_hit) {
  if (!world || !origin || !dir || !out_hit) return false;
  out_hit->hit = false;
  out_hit->distance = max_dist;
  for (uint32_t i = 0; i < world->body_count; i++) {
    RigidBody *body = world->bodies[i];
    if (!body || !body->is_active || !body->shape) continue;
    float hit_dist;
    if (shape_raycast(body->shape, origin, dir, max_dist, &hit_dist)) {
      if (hit_dist < out_hit->distance) {
        out_hit->distance = hit_dist;
        out_hit->hit = true;
        out_hit->body = body;
        out_hit->point[0] = origin[0] + dir[0] * hit_dist;
        out_hit->point[1] = origin[1] + dir[1] * hit_dist;
        out_hit->point[2] = origin[2] + dir[2] * hit_dist;
        out_hit->normal[0] = 0.0f; out_hit->normal[1] = 1.0f; out_hit->normal[2] = 0.0f; // stub normal
      }
    }
  }
  return out_hit->hit;
}

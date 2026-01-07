/**
 * =================================================================================================
 *                          ADVANCED PHYSICS CONSTRAINTS
 *                          Phase 1: Physics Engine Finalization
 * =================================================================================================
 *
 * PURPOSE: Hinge, Slider, and Point-to-Point constraints with limits and motors
 * =================================================================================================
 */

#include "include/math/quat.h"
#include "include/math/vec3.h"
#include "physics/physics_engine_core.h"
#include <math.h>
#include <stdbool.h>

// Helper to access RigidBody float arrays using Vec3
static inline Vec3 rb_get_pos(RigidBody *b) { return *(Vec3 *)b->position; }
static inline Quat rb_get_rot(RigidBody *b) { return *(Quat *)b->rotation; }

static inline void rb_add_vel(RigidBody *b, Vec3 v) {
  Vec3 current = *(Vec3 *)b->velocity;
  *(Vec3 *)b->velocity = vec3_add(current, v);
}

// Helper to get rigid body by ID
static RigidBody *get_body(PhysicsWorld *world, uint32_t id) {
  if (!world || !world->bodies)
    return NULL;
  for (uint32_t i = 0; i < world->body_count; i++) {
    if (world->bodies[i]->id == id)
      return world->bodies[i];
  }
  return NULL;
}

// -----------------------------------------------------------------------------
// Constraint Solvers
// -----------------------------------------------------------------------------

static void solve_hinge_constraint(void *config, RigidBody *b1, RigidBody *b2,
                                   Constraint *c, float dt) {
  // 1. Positional Constraint (Ball-Socket part)
  Vec3 anchor_a = {c->local_anchor_a[0], c->local_anchor_a[1],
                   c->local_anchor_a[2]};
  Vec3 anchor_b = {c->local_anchor_b[0], c->local_anchor_b[1],
                   c->local_anchor_b[2]};

  Vec3 r1 = quat_rotate_vec3(rb_get_rot(b1), anchor_a);
  Vec3 r2 = quat_rotate_vec3(rb_get_rot(b2), anchor_b);

  Vec3 p1 = vec3_add(rb_get_pos(b1), r1);
  Vec3 p2 = vec3_add(rb_get_pos(b2), r2);

  Vec3 delta = vec3_sub(p2, p1);
  float dist = vec3_length(delta);

  if (dist > 0.001f) {
    Vec3 dir = vec3_mul(delta, 1.0f / dist); // Normalized direction
    float bias = 0.2f / dt * dist;           // Baumgarte

    float inv_mass_sum = b1->inv_mass + b2->inv_mass;
    if (inv_mass_sum > 0.0f) {
      float lambda = bias / inv_mass_sum;
      Vec3 impulse = vec3_mul(dir, lambda);

      rb_add_vel(b1, vec3_mul(impulse, b1->inv_mass));
      rb_add_vel(b2, vec3_mul(impulse, -b2->inv_mass));
    }
  }
}

static void solve_slider_constraint(void *config, RigidBody *b1, RigidBody *b2,
                                    Constraint *c, float dt) {
  // Placeholder
}

// -----------------------------------------------------------------------------
// Extended Constraint Implementation
// -----------------------------------------------------------------------------

void physics_world_add_hinge_constraint(PhysicsWorld *world, RigidBody *body_a,
                                        RigidBody *body_b, Vec3 pivot_a,
                                        Vec3 pivot_b, Vec3 axis_a,
                                        Vec3 axis_b) {
  if (!world || world->constraint_count >= 1024)
    return;

  Constraint *c = &world->constraints[world->constraint_count++];
  c->type = CONSTRAINT_HINGE;
  c->body_a = body_a->id;
  c->body_b = body_b->id;

  c->local_anchor_a[0] = pivot_a.x;
  c->local_anchor_a[1] = pivot_a.y;
  c->local_anchor_a[2] = pivot_a.z;
  c->local_anchor_b[0] = pivot_b.x;
  c->local_anchor_b[1] = pivot_b.y;
  c->local_anchor_b[2] = pivot_b.z;

  c->local_axis_a[0] = axis_a.x;
  c->local_axis_a[1] = axis_a.y;
  c->local_axis_a[2] = axis_a.z;
  c->local_axis_b[0] = axis_b.x;
  c->local_axis_b[1] = axis_b.y;
  c->local_axis_b[2] = axis_b.z;
}

void physics_world_add_slider_constraint(PhysicsWorld *world, RigidBody *body_a,
                                         RigidBody *body_b, Vec3 local_frame_a,
                                         Vec3 local_frame_b, float min_dist,
                                         float max_dist) {
  if (!world || world->constraint_count >= 1024)
    return;

  Constraint *c = &world->constraints[world->constraint_count++];
  c->type = CONSTRAINT_SLIDER;
  c->body_a = body_a->id;
  c->body_b = body_b->id;

  c->local_anchor_a[0] = local_frame_a.x;
  c->local_anchor_a[1] = local_frame_a.y;
  c->local_anchor_a[2] = local_frame_a.z;
  c->local_anchor_b[0] = local_frame_b.x;
  c->local_anchor_b[1] = local_frame_b.y;
  c->local_anchor_b[2] = local_frame_b.z;

  c->linear_limits[0][0] = min_dist;
  c->linear_limits[0][1] = max_dist;
  c->limit_enabled[0] = true;
}

void solve_advanced_constraints(PhysicsWorld *world, float dt) {
  for (uint32_t i = 0; i < world->constraint_count; i++) {
    Constraint *c = &world->constraints[i];

    RigidBody *b1 = get_body(world, c->body_a);
    RigidBody *b2 = get_body(world, c->body_b);
    if (!b1 || !b2)
      continue;

    switch (c->type) {
    case CONSTRAINT_HINGE:
      solve_hinge_constraint(NULL, b1, b2, c, dt);
      break;
    case CONSTRAINT_SLIDER:
      solve_slider_constraint(NULL, b1, b2, c, dt);
      break;
    default:
      break;
    }
  }
}

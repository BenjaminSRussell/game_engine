#include "physics/core/physics_types.h"
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// Helper math
static void pvec3_scale(float *dst, const float *v, float s) {
  dst[0] = v[0] * s;
  dst[1] = v[1] * s;
  dst[2] = v[2] * s;
}
static void pvec3_add(float *dst, const float *a, const float *b) {
  dst[0] = a[0] + b[0];
  dst[1] = a[1] + b[1];
  dst[2] = a[2] + b[2];
}

// Get rigid body by ID from world - Extern from physics_core.c
RigidBody *physics_world_get_body(PhysicsWorld *world, uint32_t id);

// Solve a single constraint (internal implementation)
static void constraint_solve_impl(Constraint *c, PhysicsWorld *world, float dt) {
    if (!c || !world || c->is_broken) return;
    
    RigidBody *body_a = physics_world_get_body(world, c->body_a);
    RigidBody *body_b = physics_world_get_body(world, c->body_b);
    
    if (!body_a || !body_b) return;
    
    // Constraint solving using sequential impulses
    switch (c->type) {
    case CONSTRAINT_FIXED:
        // Fixed joint: maintain relative position and orientation
        {
            float delta_pos[3];
            delta_pos[0] = body_b->position[0] - body_a->position[0] - c->data.fixed.relative_position[0];
            delta_pos[1] = body_b->position[1] - body_a->position[1] - c->data.fixed.relative_position[1];
            delta_pos[2] = body_b->position[2] - body_a->position[2] - c->data.fixed.relative_position[2];
            
            // Apply correction impulse
            float correction = 0.2f; // Baumgarte stabilization
            float impulse[3];
            pvec3_scale(impulse, delta_pos, -correction / dt);
            
            if (body_a->type == RIGID_BODY_DYNAMIC) {
                pvec3_add(body_a->velocity, body_a->velocity, impulse);
            }
            if (body_b->type == RIGID_BODY_DYNAMIC) {
                pvec3_scale(impulse, impulse, -1.0f);
                pvec3_add(body_b->velocity, body_b->velocity, impulse);
            }
        }
        break;
        
    case CONSTRAINT_POINT_TO_POINT:
        // Ball socket: keep pivots together
        {
            float world_pivot_a[3], world_pivot_b[3];
            pvec3_add(world_pivot_a, body_a->position, c->data.point_to_point.pivot_a);
            pvec3_add(world_pivot_b, body_b->position, c->data.point_to_point.pivot_b);
            
            float delta[3];
            delta[0] = world_pivot_b[0] - world_pivot_a[0];
            delta[1] = world_pivot_b[1] - world_pivot_a[1];
            delta[2] = world_pivot_b[2] - world_pivot_a[2];
            
            float distance = sqrtf(delta[0]*delta[0] + delta[1]*delta[1] + delta[2]*delta[2]);
            if (distance < 0.001f) break;
            
            float correction = 0.3f;
            float impulse_mag = -distance * correction / dt;
            float impulse[3];
            pvec3_scale(impulse, delta, impulse_mag / distance);
            
            if (body_a->type == RIGID_BODY_DYNAMIC) {
                pvec3_add(body_a->velocity, body_a->velocity, impulse);
            }
            if (body_b->type == RIGID_BODY_DYNAMIC) {
                pvec3_scale(impulse, impulse, -1.0f);
                pvec3_add(body_b->velocity, body_b->velocity, impulse);
            }
        }
        break;
        
    case CONSTRAINT_HINGE:
        // Hinge: allow rotation around axis only
        {
            // Calculate relative position
            float delta_pos[3];
            delta_pos[0] = body_b->position[0] - body_a->position[0];
            delta_pos[1] = body_b->position[1] - body_a->position[1];
            delta_pos[2] = body_b->position[2] - body_a->position[2];
            
            // Project onto hinge axis for linear constraint
            float *axis = c->data.hinge.axis;
            float dot = delta_pos[0]*axis[0] + delta_pos[1]*axis[1] + delta_pos[2]*axis[2];
            
            // Keep pivot points aligned perpendicular to axis
            float perpendicular[3];
            perpendicular[0] = delta_pos[0] - dot * axis[0];
            perpendicular[1] = delta_pos[1] - dot * axis[1];
            perpendicular[2] = delta_pos[2] - dot * axis[2];
            
            float correction = 0.2f;
            float impulse[3];
            pvec3_scale(impulse, perpendicular, -correction / dt);
            
            if (body_a->type == RIGID_BODY_DYNAMIC) {
                pvec3_add(body_a->velocity, body_a->velocity, impulse);
            }
            if (body_b->type == RIGID_BODY_DYNAMIC) {
                pvec3_scale(impulse, impulse, -1.0f);
                pvec3_add(body_b->velocity, body_b->velocity, impulse);
            }
        }
        break;
        
    case CONSTRAINT_SLIDER:
        // Slider: allow translation along axis only
        {
            float delta_pos[3];
            delta_pos[0] = body_b->position[0] - body_a->position[0];
            delta_pos[1] = body_b->position[1] - body_a->position[1];
            delta_pos[2] = body_b->position[2] - body_a->position[2];
            
            float *axis = c->data.slider.axis;
            float along_axis = delta_pos[0]*axis[0] + delta_pos[1]*axis[1] + delta_pos[2]*axis[2];
            
            // Enforce limits
            if (c->data.slider.has_limits) {
                if (along_axis < c->data.slider.min_limit) {
                    along_axis = c->data.slider.min_limit;
                } else if (along_axis > c->data.slider.max_limit) {
                    along_axis = c->data.slider.max_limit;
                }
            }
            
            // Keep perpendicular to axis constrained
            float target[3];
            pvec3_scale(target, axis, along_axis);
            
            float error[3];
            error[0] = delta_pos[0] - target[0];
            error[1] = delta_pos[1] - target[1];
            error[2] = delta_pos[2] - target[2];
            
            float correction = 0.2f;
            float impulse[3];
            pvec3_scale(impulse, error, -correction / dt);
            
            if (body_a->type == RIGID_BODY_DYNAMIC) {
                pvec3_add(body_a->velocity, body_a->velocity, impulse);
            }
            if (body_b->type == RIGID_BODY_DYNAMIC) {
                pvec3_scale(impulse, impulse, -1.0f);
                pvec3_add(body_b->velocity, body_b->velocity, impulse);
            }
        }
        break;
        
    default:
        break;
    }
}

void physics_world_solve_constraints(PhysicsWorld *world, float dt) {
  if (!world) return;
  for (uint32_t i = 0; i < world->constraint_count; i++) {
    constraint_solve_impl(&world->constraints[i], world, dt);
  }
}

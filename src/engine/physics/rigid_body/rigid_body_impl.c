/**
 * RIGID BODY IMPLEMENTATION
 * Implements core rigid body dynamics using physics_engine_core.h
 */

#include "physics/physics_engine_core.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math.h>

// Helper for quaternion multiplication
static void q_mul(float *out, const float *q1, const float *q2) {
    out[0] = q1[3] * q2[0] + q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1];
    out[1] = q1[3] * q2[1] - q1[0] * q2[2] + q1[1] * q2[3] + q1[2] * q2[0];
    out[2] = q1[3] * q2[2] + q1[0] * q2[1] - q1[1] * q2[0] + q1[2] * q2[3];
    out[3] = q1[3] * q2[3] - q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2];
}

// Helper for quaternion normalization
static void q_norm(float *q) {
    float len = sqrtf(q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);
    if (len > 0.00001f) {
        float inv_len = 1.0f / len;
        q[0] *= inv_len; q[1] *= inv_len; q[2] *= inv_len; q[3] *= inv_len;
    } else {
        q[0] = 0; q[1] = 0; q[2] = 0; q[3] = 1; // Identity
    }
}


RigidBody *core_rigid_body_create(uint32_t id, RigidBodyType type) {
  RigidBody *body = (RigidBody *)calloc(1, sizeof(RigidBody));
  if (!body)
    return NULL;
  body->id = id;
  body->type = type;
  body->mass = 1.0f;
  body->inv_mass = 1.0f;
  body->inertia_tensor[0] = 1.0f; body->inertia_tensor[4] = 1.0f; body->inertia_tensor[8] = 1.0f;
  body->rotation[3] = 1.0f; // Identity quaternion
  body->is_active = true;
  return body;
}

void core_rigid_body_destroy(RigidBody *body) {
  if (body) {
      if (body->shape) free(body->shape);
      free(body);
  }
}

void core_rigid_body_set_position(RigidBody *body, const float *pos) {
  if (body && pos) {
      memcpy(body->position, pos, 3 * sizeof(float));
  }
}

void core_rigid_body_set_rotation(RigidBody *body, const float *rot) {
    if (body && rot) {
        memcpy(body->rotation, rot, 4 * sizeof(float));
    }
}

void core_rigid_body_set_velocity(RigidBody *body, const float *vel) {
    if (body && vel) {
        memcpy(body->velocity, vel, 3 * sizeof(float));
    }
}
void core_rigid_body_set_angular_velocity(RigidBody *body, const float *vel) {
    if (body) memcpy(body->angular_velocity, vel, 3 * sizeof(float));
}

void core_rigid_body_apply_force(RigidBody *body, const float *force) {
    if (!body || body->type != RIGID_BODY_DYNAMIC || body->is_sleeping) return;
    
    body->accumulated_force[0] += force[0];
    body->accumulated_force[1] += force[1];
    body->accumulated_force[2] += force[2];
    core_rigid_body_wake_up(body);
}

void core_rigid_body_apply_force_at_point(RigidBody *body, const float *force, const float *point) {
    if (!body || body->type != RIGID_BODY_DYNAMIC || body->is_sleeping) return;
    
    // Add linear force
    core_rigid_body_apply_force(body, force);
    
    // Calculate torque: r x F
    float r[3] = {
        point[0] - body->position[0],
        point[1] - body->position[1],
        point[2] - body->position[2]
    };
    
    float torque[3] = {
        r[1]*force[2] - r[2]*force[1],
        r[2]*force[0] - r[0]*force[2],
        r[0]*force[1] - r[1]*force[0]
    };
    
    core_rigid_body_apply_torque(body, torque);
}

void core_rigid_body_apply_torque(RigidBody *body, const float *torque) {
    if (!body || body->type != RIGID_BODY_DYNAMIC || body->is_sleeping) return;
    
    body->accumulated_torque[0] += torque[0];
    body->accumulated_torque[1] += torque[1];
    body->accumulated_torque[2] += torque[2];
    core_rigid_body_wake_up(body);
}

void core_rigid_body_apply_impulse(RigidBody *body, const float *impulse) {
    if (!body || body->type != RIGID_BODY_DYNAMIC) return;
    
    body->velocity[0] += impulse[0] * body->inv_mass;
    body->velocity[1] += impulse[1] * body->inv_mass;
    body->velocity[2] += impulse[2] * body->inv_mass;
    core_rigid_body_wake_up(body);
}

void core_rigid_body_apply_impulse_at_point(RigidBody *body, const float *impulse, const float *point) {
    if (!body || body->type != RIGID_BODY_DYNAMIC) return;
    
    // Linear impulse
    core_rigid_body_apply_impulse(body, impulse);
    
    // Angular impulse (simplified, assumes diagonal inertia for now)
    float r[3] = {
        point[0] - body->position[0],
        point[1] - body->position[1],
        point[2] - body->position[2]
    };
    
    float torque_impulse[3] = {
        r[1]*impulse[2] - r[2]*impulse[1],
        r[2]*impulse[0] - r[0]*impulse[2],
        r[0]*impulse[1] - r[1]*impulse[0]
    };
    
    // Apply angular impulse using inverse inertia tensor
    body->angular_velocity[0] += torque_impulse[0] * body->inv_inertia_tensor[0];
    body->angular_velocity[1] += torque_impulse[1] * body->inv_inertia_tensor[4];
    body->angular_velocity[2] += torque_impulse[2] * body->inv_inertia_tensor[8];
    
    core_rigid_body_wake_up(body);
}

void core_rigid_body_integrate(RigidBody *body, float dt) {
    if (!body || body->type == RIGID_BODY_STATIC || body->is_sleeping) {
        // Clear accumulators even for static bodies to prevent buildup if they become dynamic
        if (body) {
            body->accumulated_force[0] = body->accumulated_force[1] = body->accumulated_force[2] = 0;
            body->accumulated_torque[0] = body->accumulated_torque[1] = body->accumulated_torque[2] = 0;
        }
        return;
    }
    
    // 1. Semi-Implicit Euler Integration
    
    // Update linear velocity
    float linear_acc[3] = {
        body->accumulated_force[0] * body->inv_mass,
        body->accumulated_force[1] * body->inv_mass,
        body->accumulated_force[2] * body->inv_mass
    };
    
    // Damping should be time-dependent: (1.0 - damping * dt)
    float lin_damping_factor = fmaxf(0.0f, 1.0f - body->linear_damping * dt);
    float ang_damping_factor = fmaxf(0.0f, 1.0f - body->angular_damping * dt);
    
    body->velocity[0] = (body->velocity[0] + linear_acc[0] * dt) * lin_damping_factor;
    body->velocity[1] = (body->velocity[1] + linear_acc[1] * dt) * lin_damping_factor;
    body->velocity[2] = (body->velocity[2] + linear_acc[2] * dt) * lin_damping_factor;
    
    // Update angular velocity
    // Simplified: assuming diagonal inertia tensor for basic integration
    float angular_acc[3] = {
        body->accumulated_torque[0] * body->inv_inertia_tensor[0],
        body->accumulated_torque[1] * body->inv_inertia_tensor[4],
        body->accumulated_torque[2] * body->inv_inertia_tensor[8]
    };
    
    body->angular_velocity[0] = (body->angular_velocity[0] + angular_acc[0] * dt) * ang_damping_factor;
    body->angular_velocity[1] = (body->angular_velocity[1] + angular_acc[1] * dt) * ang_damping_factor;
    body->angular_velocity[2] = (body->angular_velocity[2] + angular_acc[2] * dt) * ang_damping_factor;
    
    // Update position
    body->position[0] += body->velocity[0] * dt;
    body->position[1] += body->velocity[1] * dt;
    body->position[2] += body->velocity[2] * dt;
    
    // Update rotation (Quaternion integration): q += 0.5 * w * q * dt
    // w = angular velocity as quaternion [0, x, y, z]
    float w[4] = { body->angular_velocity[0], body->angular_velocity[1], body->angular_velocity[2], 0.0f };
    float q_vel[4];
    
    // q_vel = w * q
    q_mul(q_vel, w, body->rotation);
    
    body->rotation[0] += 0.5f * q_vel[0] * dt;
    body->rotation[1] += 0.5f * q_vel[1] * dt;
    body->rotation[2] += 0.5f * q_vel[2] * dt;
    body->rotation[3] += 0.5f * q_vel[3] * dt;
    
    q_norm(body->rotation);
    
    // Clear accumulators
    body->accumulated_force[0] = body->accumulated_force[1] = body->accumulated_force[2] = 0;
    body->accumulated_torque[0] = body->accumulated_torque[1] = body->accumulated_torque[2] = 0;
}

void core_rigid_body_wake_up(RigidBody *body) {
    if (body) {
        body->is_sleeping = false;
        body->sleep_timer = 0.0f;
    }
}

void core_rigid_body_put_to_sleep(RigidBody *body) {
    if (body) {
        body->is_sleeping = true;
        body->velocity[0] = body->velocity[1] = body->velocity[2] = 0;
        body->angular_velocity[0] = body->angular_velocity[1] = body->angular_velocity[2] = 0;
    }
}

void core_rigid_body_get_velocity_at_point(RigidBody *body, const float *point, float *out_vel) {
    if (!body) {
        if (out_vel) out_vel[0] = out_vel[1] = out_vel[2] = 0;
        return;
    }
    
    // v_point = v + w x r
    float r[3] = {
        point[0] - body->position[0],
        point[1] - body->position[1],
        point[2] - body->position[2]
    };
    
    float w[3] = { body->angular_velocity[0], body->angular_velocity[1], body->angular_velocity[2] };
    
    // w x r
    float wxr[3] = {
        w[1]*r[2] - w[2]*r[1],
        w[2]*r[0] - w[0]*r[2],
        w[0]*r[1] - w[1]*r[0]
    };
    
    out_vel[0] = body->velocity[0] + wxr[0];
    out_vel[1] = body->velocity[1] + wxr[1];
    out_vel[2] = body->velocity[2] + wxr[2];
}

void core_rigid_body_check_sleeping(RigidBody *body, float dt) {
    if (!body || body->type != RIGID_BODY_DYNAMIC || body->is_sleeping) return;
    
    const float padding = 0.05f;
    float v2 = body->velocity[0]*body->velocity[0] + body->velocity[1]*body->velocity[1] + body->velocity[2]*body->velocity[2];
    float w2 = body->angular_velocity[0]*body->angular_velocity[0] + body->angular_velocity[1]*body->angular_velocity[1] + body->angular_velocity[2]*body->angular_velocity[2];
    
    if (v2 < padding && w2 < padding) {
        body->sleep_timer += dt;
        if (body->sleep_timer > 1.0f) {
            core_rigid_body_put_to_sleep(body);
        }
    } else {
        body->sleep_timer = 0.0f;
    }
}

void core_rigid_body_update_inertia(RigidBody *body) {
    // Stub for now.
    // In full implementation: I_world = R * I_local * R_transpose
}

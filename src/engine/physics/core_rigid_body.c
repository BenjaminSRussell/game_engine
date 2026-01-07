#include "physics_engine_core.h"
#include <stdlib.h>
#include <string.h>

/*
// Implemented in physics_rigid_body_helpers.c
RigidBody *core_rigid_body_create(uint32_t id, RigidBodyType type) {
    return NULL; 
}

void core_rigid_body_destroy(RigidBody *body) {
}
*/

void core_rigid_body_set_position(RigidBody *body, const float *pos) {
    if (body && pos) {
        body->position[0] = pos[0];
        body->position[1] = pos[1];
        body->position[2] = pos[2];
    }
}

void core_rigid_body_set_rotation(RigidBody *body, const float *rot) {
    if (body && rot) {
        body->rotation[0] = rot[0];
        body->rotation[1] = rot[1];
        body->rotation[2] = rot[2];
        body->rotation[3] = rot[3];
    }
}

void core_rigid_body_set_velocity(RigidBody *body, const float *vel) {
    if (body && vel) {
        body->velocity[0] = vel[0];
        body->velocity[1] = vel[1];
        body->velocity[2] = vel[2];
    }
}

void core_rigid_body_set_angular_velocity(RigidBody *body, const float *vel) {
    if (body && vel) {
        body->angular_velocity[0] = vel[0];
        body->angular_velocity[1] = vel[1];
        body->angular_velocity[2] = vel[2];
    }
}

void core_rigid_body_apply_force(RigidBody *body, const float *force) {
    if (body && force) {
        body->accumulated_force[0] += force[0];
        body->accumulated_force[1] += force[1];
        body->accumulated_force[2] += force[2];
    }
}

void core_rigid_body_apply_impulse(RigidBody *body, const float *impulse) {
    if (body && impulse) {
        body->velocity[0] += impulse[0] * body->inv_mass;
        body->velocity[1] += impulse[1] * body->inv_mass;
        body->velocity[2] += impulse[2] * body->inv_mass;
    }
}

void core_rigid_body_apply_force_at_point(RigidBody *body, const float *force, const float *point) {
    // Stub
    core_rigid_body_apply_force(body, force);
}

void core_rigid_body_apply_torque(RigidBody *body, const float *torque) {
    if (body && torque) {
        body->accumulated_torque[0] += torque[0];
        body->accumulated_torque[1] += torque[1];
        body->accumulated_torque[2] += torque[2];
    }
}

void core_rigid_body_apply_impulse_at_point(RigidBody *body, const float *impulse, const float *point) {
    // Stub
    core_rigid_body_apply_impulse(body, impulse);
}

void core_rigid_body_integrate(RigidBody *body, float dt) {
    // Moved logic to physics_core_impl.c, but we can call it if needed.
    // Stub for now as integration is done by world.
}

void core_rigid_body_update_inertia(RigidBody *body) {
    // Stub
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
        body->velocity[0] = 0; body->velocity[1] = 0; body->velocity[2] = 0;
        body->angular_velocity[0] = 0; body->angular_velocity[1] = 0; body->angular_velocity[2] = 0;
    }
}

void core_rigid_body_serialize(RigidBody *body, void *buffer) {}
void core_rigid_body_deserialize(RigidBody *body, void *buffer) {}

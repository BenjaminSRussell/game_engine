#include "physics/core/physics_types.h"
#include <stdlib.h>
#include <string.h>

// Helper functions expected by external code (physics_api_impl.c, demo code)

// Get velocity at a point on a rigid body (for solver)
void core_rigid_body_get_velocity_at_point(RigidBody *body, const float *point, float *out_vel) {
    if (!body || !out_vel) return;
    // Simplified: just copy linear velocity (ignore angular contribution)
    memcpy(out_vel, body->velocity, 3 * sizeof(float));
}

// Create a rigid body
RigidBody *core_rigid_body_create(uint32_t id, RigidBodyType type) {
    RigidBody *body = (RigidBody*)calloc(1, sizeof(RigidBody));
    if (!body) return NULL;
    
    body->id = id;
    body->type = type;
    body->is_active = true;
    body->is_sleeping = false;
    body->mass = 1.0f;
    body->inv_mass = 1.0f;
    body->linear_damping = 0.05f;
    body->angular_damping = 0.05f;
    body->friction = 0.5f;
    body->restitution = 0.5f;
    
    // Identity quaternion
    body->rotation[0] = 0.0f;
    body->rotation[1] = 0.0f;
    body->rotation[2] = 0.0f;
    body->rotation[3] = 1.0f;
    
    return body;
}

void core_rigid_body_destroy(RigidBody *body) {
    if (body) free(body);
}

// Public getters for rendering/debug
bool rigid_body_is_sleeping(const RigidBody *body) {
    return body ? body->is_sleeping : false;
}

CollisionShapeType collider_get_type(const Collider *collider) {
    const Collider *c = collider;
    return c ? c->type : COLLISION_SHAPE_SPHERE;
}

void collider_get_box_half_extents(const Collider *collider, float *out_x, float *out_y, float *out_z) {
    CollisionShape *c = (CollisionShape*)collider;
    if (!c || c->type != COLLISION_SHAPE_BOX) return;
    if (out_x) *out_x = c->data.box.half_extents[0];
    if (out_y) *out_y = c->data.box.half_extents[1];
    if (out_z) *out_z = c->data.box.half_extents[2];
}

float collider_get_sphere_radius(const Collider *collider) {
    CollisionShape *c = (CollisionShape*)collider;
    if (!c || c->type != COLLISION_SHAPE_SPHERE) return 0.0f;
    return c->data.sphere.radius;
}

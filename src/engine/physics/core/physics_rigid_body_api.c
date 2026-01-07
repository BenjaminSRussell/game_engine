#include "physics/core/physics_types.h"
#include "physics/physics.h"

// Implement rigid_body_get_velocity
Vec3 rigid_body_get_velocity(const RigidBody *body) {
    Vec3 result = {0.0f, 0.0f, 0.0f};
    if (body) {
        result.x = body->velocity[0];
        result.y = body->velocity[1];
        result.z = body->velocity[2];
    }
    return result;
}

// Implement rigid_body_get_position
Vec3 rigid_body_get_position(const RigidBody *body) {
    Vec3 result = {0.0f, 0.0f, 0.0f};
    if (body) {
        result.x = body->position[0];
        result.y = body->position[1];
        result.z = body->position[2];
    }
    return result;
}

// Implement physics_raycast stub
RaycastResult physics_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction, f32 max_distance) {
    RaycastResult result = {0};
    result.origin = origin;
    result.direction = direction;
    result.max_distance = max_distance;
    result.hit = false;
    // Stub - always returns no hit
    return result;
}

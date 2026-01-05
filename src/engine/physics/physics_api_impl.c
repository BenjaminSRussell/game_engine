#include "engine/include/physics/physics.h"
#include "physics_engine_core.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

// Global Config State
static PhysicsConfig g_physics_config;

void physics_config_init(void) {
    g_physics_config.gravity = vec3(0, -9.81f, 0);
    g_physics_config.fixed_timestep = 1.0f/60.0f;
    g_physics_config.velocity_iterations = 10;
    g_physics_config.position_iterations = 5;
}

PhysicsConfig physics_config_get_current(void) {
    return g_physics_config;
}

PhysicsConfig physics_config_get_default(void) {
    PhysicsConfig c = {0};
    c.gravity = vec3(0, -9.81f, 0);
    c.fixed_timestep = 1.0f/60.0f;
    c.velocity_iterations = 10;
    c.position_iterations = 5;
    return c;
}

PhysicsConfig physics_config_get_minecraft(void) {
    return physics_config_get_default();
}

void physics_config_load_preset_by_name(const char *name) {
// Stub
}

void physics_config_set_gravity(Vec3 gravity) {
    g_physics_config.gravity = gravity;
}

PhysicsMaterial physics_material_get_by_name(const char *name) {
    PhysicsMaterial m = {0.5f, 0.5f, 1.0f};
    if (strcmp(name, "ice") == 0) m.friction = 0.1f;
    if (strcmp(name, "rubber") == 0) m.restitution = 0.8f;
    return m;
}

// Physics World Implementation
// Physics World Definition is imported from physics_engine_core.h

// PhysicsWorld functions are implemented in physics_core.c
// We rely on linking with physics_core.c to provide:
// - physics_world_create
// - physics_world_destroy
// - physics_world_step
// - physics_world_add_body

void physics_material_list_presets(void) {
    printf("Standard, Ice, Rubber\n");
}

// === Rigid Body Public API Wrappers ===

RigidBody *rigid_body_create(BodyType type, Vec3 position) {
    RigidBodyType internal_type = (type == BODY_TYPE_DYNAMIC) ? RIGID_BODY_DYNAMIC :
                                  (type == BODY_TYPE_KINEMATIC) ? RIGID_BODY_KINEMATIC : RIGID_BODY_STATIC;
    
    // ID generation stub (should use a counter)
    static uint32_t body_id_counter = 1;
    RigidBody *body = core_rigid_body_create(body_id_counter++, internal_type);
    
    float pos[3] = {position.x, position.y, position.z};
    core_rigid_body_set_position(body, pos);
    
    return body;
}

void rigid_body_destroy(RigidBody *body) {
    core_rigid_body_destroy(body);
}

void rigid_body_set_position(RigidBody *body, Vec3 position) {
    float pos[3] = {position.x, position.y, position.z};
    core_rigid_body_set_position(body, pos);
}

void rigid_body_set_rotation(RigidBody *body, Quat rotation) {
    float rot[4] = {rotation.x, rotation.y, rotation.z, rotation.w};
    core_rigid_body_set_rotation(body, rot);
}

void rigid_body_set_velocity(RigidBody *body, Vec3 velocity) {
    float vel[3] = {velocity.x, velocity.y, velocity.z};
    core_rigid_body_set_velocity(body, vel);
}

void rigid_body_set_angular_velocity(RigidBody *body, Vec3 velocity) {
    float vel[3] = {velocity.x, velocity.y, velocity.z};
    core_rigid_body_set_angular_velocity(body, vel);
}

void rigid_body_apply_force(RigidBody *body, Vec3 force) {
    float f[3] = {force.x, force.y, force.z};
    core_rigid_body_apply_force(body, f);
}

void rigid_body_apply_impulse(RigidBody *body, Vec3 impulse) {
    float i[3] = {impulse.x, impulse.y, impulse.z};
    core_rigid_body_apply_impulse(body, i);
}

Vec3 rigid_body_get_position(const RigidBody *body) {
    if (!body) return vec3(0,0,0);
    return vec3(body->position[0], body->position[1], body->position[2]);
}

Vec3 rigid_body_get_velocity(const RigidBody *body) {
    if (!body) return vec3(0,0,0);
    return vec3(body->velocity[0], body->velocity[1], body->velocity[2]);
}

// Rigid Body Helpers
RigidBody *physics_create_box(PhysicsWorld *world, Vec3 pos, Vec3 size, float mass, PhysicsMaterial *mat) {
    RigidBody *body = rigid_body_create(mass > 0 ? BODY_TYPE_DYNAMIC : BODY_TYPE_STATIC, pos);
    
    // Half extents
    body->shape = shape_box_create(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
    
    body->mass = mass;
    if (mass > 0) body->inv_mass = 1.0f / mass;
    
    if (mat) {
        body->friction = mat->friction;
        body->restitution = mat->restitution;
    }
    
    // physics_world_add_body is in physics_core.c
    RigidBody *internal_body = physics_world_add_body(world, body);
    
    return internal_body ? internal_body : body; 
}

// Raycast Wrapper
RaycastResult physics_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction, float max_distance) {
    RaycastResult res = {0};
    res.origin = origin;
    res.direction = direction;
    res.max_distance = max_distance;
    
    RayHit hit;
    float o[3] = {origin.x, origin.y, origin.z};
    float d[3] = {direction.x, direction.y, direction.z};
    
    // Uses core_physics_world_raycast from engine_core.h
    if (core_physics_world_raycast(world, o, d, max_distance, &hit)) {
        res.hit = true;
        res.hit_distance = hit.distance;
        res.hit_point = vec3(hit.point[0], hit.point[1], hit.point[2]);
        res.hit_normal = vec3(hit.normal[0], hit.normal[1], hit.normal[2]);
        res.hit_body = hit.body;
    }
    
    return res;
}

// Character Controller Implementation
struct CharacterController {
    float position[3];
    float velocity[3];
    bool is_grounded;
    RigidBody* body;
};

void character_controller_init_system(void) {}
void character_controller_cleanup_system(void) {}

CharacterController *character_controller_create(PhysicsWorld *world, Vec3 position) {
    CharacterController *cc = (CharacterController*)calloc(1, sizeof(CharacterController));
    cc->position[0] = position.x;
    cc->position[1] = position.y;
    cc->position[2] = position.z;
    return cc;
}
void character_controller_destroy(CharacterController *controller) { free(controller); }
void character_controller_set_move_speed(CharacterController *controller, float speed) {}
void character_controller_set_jump_height(CharacterController *controller, float height) {}
void character_controller_set_slope_limit(CharacterController *controller, float angle_degrees) {}
void character_controller_update(CharacterController *controller, PhysicsWorld *world, float delta_time) {}
bool character_controller_is_grounded(CharacterController *controller) { return true; }
void character_controller_jump(CharacterController *controller, PhysicsWorld *world) {}
Vec3 character_controller_get_position(CharacterController *controller) {
    return vec3(controller->position[0], controller->position[1], controller->position[2]);
}
Vec3 character_controller_get_velocity(CharacterController *controller) { return vec3(0,0,0); }
void character_controller_move(CharacterController *controller, PhysicsWorld *world, Vec3 direction, float delta_time) {}


// === Missing Public API Implementations ===

Collider *collider_create_box(Vec3 half_extents) {
    return (Collider*)shape_box_create(half_extents.x, half_extents.y, half_extents.z);
}

Collider *collider_create_sphere(f32 radius) {
    return (Collider*)shape_sphere_create(radius);
}

void rigid_body_attach_collider(RigidBody *body, Collider *collider) {
    if (body && collider) {
        body->shape = (CollisionShape*)collider;
    }
}

Collider *rigid_body_get_collider(const RigidBody *body) {
    return body ? (Collider*)body->shape : NULL;
}

Quat rigid_body_get_rotation(const RigidBody *body) {
    if (!body) return quat_identity();
    return quat(body->rotation[0], body->rotation[1], body->rotation[2], body->rotation[3]);
}

void rigid_body_set_mass(RigidBody *body, f32 mass) {
    if (!body || mass <= 0.0f) return;
    body->inv_mass = 1.0f / mass;
}

void rigid_body_set_friction(RigidBody *body, f32 friction) {
    if (body) body->friction = friction;
}

void rigid_body_set_restitution(RigidBody *body, f32 restitution) {
    if (body) body->restitution = restitution;
}

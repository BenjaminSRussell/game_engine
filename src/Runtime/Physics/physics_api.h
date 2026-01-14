#ifndef PHYSICS_API_H
#define PHYSICS_API_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct PhysicsWorld PhysicsWorld;
typedef struct RigidBody RigidBody;
typedef struct CollisionShape CollisionShape;
typedef struct RayHit RayHit;

// Physics configuration
typedef struct {
    float gravity[3];
    float timestep;
    int max_substeps;
    bool enable_debug_draw;
} PhysicsConfig;

// Rigid body types
typedef enum {
    RIGID_BODY_STATIC,
    RIGID_BODY_DYNAMIC,
    RIGID_BODY_KINEMATIC
} RigidBodyType;

// Shape types
typedef enum {
    SHAPE_SPHERE,
    SHAPE_BOX,
    SHAPE_CAPSULE,
    SHAPE_CYLINDER,
    SHAPE_CONE,
    SHAPE_CONVEX_HULL,
    SHAPE_MESH,
    SHAPE_HEIGHTFIELD,
    SHAPE_COMPOUND
} ShapeType;

// Ray hit result
struct RayHit {
    float position[3];
    float normal[3];
    float distance;
    RigidBody* body;
    CollisionShape* shape;
};

// === World Management ===
PhysicsWorld* physics_world_create(PhysicsConfig config);
void physics_world_destroy(PhysicsWorld* world);
void physics_world_step(PhysicsWorld* world, float dt);
void physics_world_set_gravity(PhysicsWorld* world, const float* gravity);
void physics_world_get_gravity(PhysicsWorld* world, float* out_gravity);

// === Rigid Body Management ===
RigidBody* physics_body_create(uint32_t id, RigidBodyType type, CollisionShape* shape);
void physics_body_destroy(RigidBody* body);
void physics_body_set_position(RigidBody* body, const float* position);
void physics_body_set_rotation(RigidBody* body, const float* rotation);
void physics_body_set_velocity(RigidBody* body, const float* velocity);
void physics_body_set_angular_velocity(RigidBody* body, const float* angular_velocity);
void physics_body_get_position(RigidBody* body, float* out_position);
void physics_body_get_rotation(RigidBody* body, float* out_rotation);
void physics_body_get_velocity(RigidBody* body, float* out_velocity);
void physics_body_get_angular_velocity(RigidBody* body, float* out_angular_velocity);
void physics_body_apply_force(RigidBody* body, const float* force);
void physics_body_apply_impulse(RigidBody* body, const float* impulse);
void physics_body_apply_torque(RigidBody* body, const float* torque);
void physics_body_set_mass(RigidBody* body, float mass);
float physics_body_get_mass(RigidBody* body);
void physics_body_set_friction(RigidBody* body, float friction);
void physics_body_set_restitution(RigidBody* body, float restitution);
void physics_body_set_user_data(RigidBody* body, void* user_data);
void* physics_body_get_user_data(RigidBody* body);
void physics_body_get_aabb(RigidBody* body, float* min_out, float* max_out);
bool physics_body_get_velocity_at_point(RigidBody* body, const float* point, float* out_vel);

// === Shape Management ===
CollisionShape* physics_shape_sphere_create(float radius);
CollisionShape* physics_shape_box_create(float half_extents[3]);
CollisionShape* physics_shape_capsule_create(float radius, float height);
CollisionShape* physics_shape_cylinder_create(float radius, float height);
CollisionShape* physics_shape_cone_create(float radius, float height);
CollisionShape* physics_shape_convex_hull_create(float* vertices, uint32_t vertex_count);
CollisionShape* physics_shape_mesh_create(float* vertices, uint32_t vertex_count, uint32_t* indices, uint32_t index_count);
CollisionShape* physics_shape_heightfield_create(float* heights, uint32_t width, uint32_t depth, float scale[3]);
CollisionShape* physics_shape_compound_create(void);
void physics_shape_destroy(CollisionShape* shape);
void physics_shape_compound_add_shape(CollisionShape* compound, CollisionShape* shape, float* transform);
void physics_shape_calculate_local_inertia(CollisionShape* shape, float mass, float* inertia);
ShapeType physics_shape_get_type(CollisionShape* shape);

// === World Operations ===
RigidBody* physics_world_add_body(PhysicsWorld* world, RigidBody* body);
void physics_world_remove_body(PhysicsWorld* world, RigidBody* body);
bool physics_world_raycast(PhysicsWorld* world, const float* origin, const float* direction, float max_distance, RayHit* out_hit);
void physics_world_query_aabb(PhysicsWorld* world, const float* min_aabb, const float* max_aabb, RigidBody** out_bodies, uint32_t* out_count, uint32_t max_count);

// === Constraint Management ===
typedef struct Constraint Constraint;
Constraint* physics_constraint_point_to_point_create(RigidBody* body_a, RigidBody* body_b, const float* pivot_a, const float* pivot_b);
Constraint* physics_constraint_hinge_create(RigidBody* body_a, RigidBody* body_b, const float* pivot_a, const float* pivot_b, const float* axis_a, const float* axis_b);
Constraint* physics_constraint_slider_create(RigidBody* body_a, RigidBody* body_b, const float* axis_a, const float* axis_b);
Constraint* physics_constraint_fixed_create(RigidBody* body_a, RigidBody* body_b, const float* transform_a, const float* transform_b);
void physics_constraint_destroy(Constraint* constraint);
void physics_constraint_set_enabled(Constraint* constraint, bool enabled);
void physics_constraint_set_breaking_impulse(Constraint* constraint, float threshold);
bool physics_constraint_is_enabled(Constraint* constraint);

// === Character Controller ===
typedef struct CharacterController CharacterController;
CharacterController* physics_character_controller_create(float radius, float height, float step_height);
void physics_character_controller_destroy(CharacterController* controller);
void physics_character_controller_set_world(CharacterController* controller, PhysicsWorld* world);
void physics_character_controller_set_position(CharacterController* controller, const float* position);
void physics_character_controller_get_position(CharacterController* controller, float* out_position);
void physics_character_controller_set_velocity(CharacterController* controller, const float* velocity);
void physics_character_controller_get_velocity(CharacterController* controller, float* out_velocity);
void physics_character_controller_set_gravity(CharacterController* controller, float gravity);
void physics_character_controller_set_fall_speed(CharacterController* controller, float fall_speed);
void physics_character_controller_set_jump_speed(CharacterController* controller, float jump_speed);
void physics_character_controller_set_slope_angle(CharacterController* controller, float slope_angle);
bool physics_character_controller_on_ground(CharacterController* controller);
void physics_character_controller_jump(CharacterController* controller);
void physics_character_controller_move(CharacterController* controller, const float* direction, float dt);

// === Debug Drawing ===
typedef struct DebugLine {
    float from[3];
    float to[3];
    float color[3];
} DebugLine;

void physics_world_debug_draw(PhysicsWorld* world, DebugLine** out_lines, uint32_t* out_count);
void physics_world_set_debug_drawer(PhysicsWorld* world, void (*draw_line)(const float* from, const float* to, const float* color));

// === Utility Functions ===
void physics_vector3_set(float* vec, float x, float y, float z);
void physics_vector3_copy(float* dest, const float* src);
void physics_vector3_add(float* result, const float* a, const float* b);
void physics_vector3_subtract(float* result, const float* a, const float* b);
void physics_vector3_multiply(float* result, const float* vec, float scalar);
void physics_vector3_cross(float* result, const float* a, const float* b);
float physics_vector3_dot(const float* a, const float* b);
float physics_vector3_length(const float* vec);
void physics_vector3_normalize(float* vec);
void physics_quaternion_identity(float* quat);
void physics_quaternion_from_euler(float* quat, float yaw, float pitch, float roll);
void physics_quaternion_multiply(float* result, const float* a, const float* b);
void physics_quaternion_rotate_vector(float* result, const float* quat, const float* vec);

#endif // PHYSICS_API_H

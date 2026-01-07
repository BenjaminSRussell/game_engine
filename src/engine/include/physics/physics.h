// include/physics/physics.h
//
// Purpose: Core physics engine definitions
//
#ifndef ENGINE_PHYSICS_H
#define ENGINE_PHYSICS_H

#include <common.h>
#include <math/quat.h>
#include <math/vec3.h>

// Forward declarations
typedef struct PhysicsWorld PhysicsWorld;
typedef struct RigidBody RigidBody;
typedef struct Collider Collider;
typedef struct CharacterController CharacterController;

// Rigid Body Types
typedef enum {
  BODY_TYPE_STATIC,    // Zero mass, zero velocity, moved manually
  BODY_TYPE_KINEMATIC, // Zero mass, non-zero velocity, moved by solver
  BODY_TYPE_DYNAMIC    // Positive mass, non-zero velocity, moved by solver
} BodyType;

// Collider Types
typedef enum {
  COLLIDER_TYPE_SPHERE,
  COLLIDER_TYPE_BOX,
  COLLIDER_TYPE_CAPSULE,
  COLLIDER_TYPE_MESH
} ColliderType;

// Collision Event
typedef struct {
  RigidBody *body_a;
  RigidBody *body_b;
  Vec3 point;
  Vec3 normal;
  f32 depth;
} CollisionManifold;

// Physics Config
typedef struct {
  Vec3 gravity;
  f32 fixed_timestep;
  u32 velocity_iterations;
  u32 position_iterations;
} PhysicsConfig;

typedef struct {
  f32 friction;
  f32 restitution;
  f32 density;
} PhysicsMaterial;

// -----------------------------------------------------------------------------
// Physics World
// -----------------------------------------------------------------------------

PhysicsWorld *physics_world_create(PhysicsConfig config);
void physics_world_destroy(PhysicsWorld *world);

// Simulation step
void physics_world_step(PhysicsWorld *world, f32 delta_time);

// Body management
RigidBody *physics_world_add_body(PhysicsWorld *world, RigidBody *body);
void physics_world_remove_body(PhysicsWorld *world, RigidBody *body);

// Raycast
bool physics_world_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                           f32 max_distance, void *out_hit);

typedef struct {
  Vec3 origin;
  Vec3 direction;
  f32 max_distance;
  bool hit;
  Vec3 hit_point;
  Vec3 hit_normal;
  f32 hit_distance;
  RigidBody *hit_body;
} RaycastResult;

RaycastResult physics_raycast(PhysicsWorld *world, Vec3 origin, Vec3 direction,
                              f32 max_distance);

// -----------------------------------------------------------------------------
// Physics Configuration
// -----------------------------------------------------------------------------

// Configuration presets
typedef enum {
  PHYSICS_PRESET_DEFAULT,
  PHYSICS_PRESET_MINECRAFT,
  PHYSICS_PRESET_SPACE,
  PHYSICS_PRESET_LOW_GRAVITY,
  PHYSICS_PRESET_CUSTOM
} PhysicsPreset;

// Configuration management
void physics_config_init(void);
PhysicsConfig physics_config_get_default(void);
PhysicsConfig physics_config_get_minecraft(void);
PhysicsConfig physics_config_get_space(void);
PhysicsConfig physics_config_get_low_gravity(void);
PhysicsConfig physics_config_get_current(void);
void physics_config_set_current(PhysicsConfig config);
void physics_config_load_preset(PhysicsPreset preset);
void physics_config_load_preset_by_name(const char *name);
PhysicsPreset physics_config_get_current_preset(void);

// Runtime adjustments
void physics_config_set_gravity(Vec3 gravity);
void physics_config_set_timestep(f32 timestep);
void physics_config_set_iterations(u32 velocity_iterations,
                                   u32 position_iterations);

// Configuration validation
bool physics_config_validate(const PhysicsConfig *config);

// Material presets
PhysicsMaterial physics_material_get_default(void);
PhysicsMaterial physics_material_get_by_name(const char *name);
bool physics_material_get_preset_by_name(const char *name,
                                         PhysicsMaterial *out_material);
void physics_material_list_presets(void);

// -----------------------------------------------------------------------------
// Character Controller
// -----------------------------------------------------------------------------

CharacterController *character_controller_create(PhysicsWorld *world,
                                                 Vec3 position);
void character_controller_destroy(CharacterController *controller);

// Configuration
void character_controller_set_move_speed(CharacterController *controller,
                                         f32 speed);
void character_controller_set_jump_height(CharacterController *controller,
                                          f32 height);
void character_controller_set_step_height(CharacterController *controller,
                                          f32 height);
void character_controller_set_slope_limit(CharacterController *controller,
                                          f32 angle_degrees);
void character_controller_set_air_control(CharacterController *controller,
                                          f32 air_control);

// State queries
bool character_controller_is_grounded(CharacterController *controller);
Vec3 character_controller_get_velocity(CharacterController *controller);
Vec3 character_controller_get_position(CharacterController *controller);

// Movement
void character_controller_move(CharacterController *controller,
                               PhysicsWorld *world, Vec3 direction,
                               f32 delta_time);
void character_controller_jump(CharacterController *controller,
                               PhysicsWorld *world);
void character_controller_update(CharacterController *controller,
                                 PhysicsWorld *world, f32 delta_time);

// Utilities
void character_controller_teleport(CharacterController *controller,
                                   Vec3 position);
void character_controller_reset(CharacterController *controller);

// System management
void character_controller_init_system(void);
void character_controller_cleanup_system(void);
void character_controller_update_all(PhysicsWorld *world, f32 delta_time);

// -----------------------------------------------------------------------------
// Rigid Body
// -----------------------------------------------------------------------------

RigidBody *rigid_body_create(BodyType type, Vec3 position);
void rigid_body_destroy(RigidBody *body);

// Properties
void rigid_body_set_mass(RigidBody *body, f32 mass);
void rigid_body_set_friction(RigidBody *body, f32 friction);
void rigid_body_set_restitution(RigidBody *body, f32 restitution);
void rigid_body_set_linear_damping(RigidBody *body, f32 damping);
void rigid_body_set_angular_damping(RigidBody *body, f32 damping);

// State
void rigid_body_set_position(RigidBody *body, Vec3 position);
Vec3 rigid_body_get_position(const RigidBody *body);
void rigid_body_set_rotation(RigidBody *body, Quat rotation);
Quat rigid_body_get_rotation(const RigidBody *body);
Vec3 rigid_body_get_velocity(const RigidBody *body);
void rigid_body_set_velocity(RigidBody *body, Vec3 velocity);

// Forces
void rigid_body_set_angular_damping(RigidBody *body, f32 damping);

// Forces and impulses
void rigid_body_add_force(RigidBody *body, Vec3 force);
void rigid_body_add_impulse(RigidBody *body, Vec3 impulse);
void rigid_body_clear_forces(RigidBody *body);

// Accessors
f32 rigid_body_get_inv_mass(const RigidBody *body);
f32 rigid_body_get_restitution(const RigidBody *body);
Collider *rigid_body_get_collider(const RigidBody *body);

// -----------------------------------------------------------------------------
// Collider
// -----------------------------------------------------------------------------

// Helper to create body with box shape
RigidBody *physics_create_box(PhysicsWorld *world, Vec3 pos, Vec3 size, f32 mass, PhysicsMaterial *mat);

Collider *collider_create_sphere(f32 radius);
Collider *collider_create_box(Vec3 half_extents);
void collider_destroy(Collider *collider);

void rigid_body_attach_collider(RigidBody *body, Collider *collider);

// Additional query functions for rendering/debugging
bool rigid_body_is_sleeping(const RigidBody *body);
ColliderType collider_get_type(const Collider *collider);
void collider_get_box_half_extents(const Collider *collider, f32 *out_x, f32 *out_y, f32 *out_z);
f32 collider_get_sphere_radius(const Collider *collider);


// Collision detection (internal/narrowphase)
bool physics_check_collision(RigidBody *a, RigidBody *b,
                             CollisionManifold *out);
void physics_resolve_collision(RigidBody *a, RigidBody *b,
                               CollisionManifold *m);

// -----------------------------------------------------------------------------
// Enhanced Physics System (Hyper-Optimized)
// -----------------------------------------------------------------------------

// Performance statistics structure
typedef struct {
  double integration_time_ms;
  double broadphase_time_ms;
  double narrowphase_time_ms;
  double solver_time_ms;
  double total_time_ms;
  int bodies_processed;
  int collision_pairs;
  int constraint_islands;
  u64 cache_misses;
  u64 branch_mispredictions;
} PhysicsPerformance;

// Enhanced system functions
bool Physics_Initialize(int max_bodies);
void Physics_Shutdown(void);
void Physics_Simulation_Step(float delta_time);
const PhysicsPerformance *Physics_GetPerformanceStats(void);
void Physics_SetDeterministicMode(bool enabled);

// Debug visualization functions
static inline void physics_debug_clear(void) {
  // Stub - clear physics debug data for next frame
}

static inline void physics_debug_get_data(Vec3 **lines, u32 *line_count) {
  // Stub - get physics debug line data
  if (lines)
    *lines = NULL;
  if (line_count)
    *line_count = 0;
}

#endif // ENGINE_PHYSICS_H

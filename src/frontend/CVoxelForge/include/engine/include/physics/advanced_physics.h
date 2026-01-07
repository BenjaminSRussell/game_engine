// include/engine/physics/advanced_physics.h
//
// Purpose: Advanced physics engine with soft body physics, fluid dynamics, and destruction
// This system provides cutting-edge physics simulation capabilities that surpass Unity's
// built-in physics with GPU acceleration, parallel processing, and realistic material behavior.
//
// Key Features:
// - Soft body physics with finite element method and mass-spring systems
// - Computational fluid dynamics (SPH, LBM) for realistic liquid and gas simulation
// - Procedural destruction with Voronoi fracture and stress-based cracking
// - Cloth simulation with wind and collision integration
// - Vehicle physics with advanced suspension and tire models
// - GPU acceleration for parallel computation
// - Multi-threaded collision detection and response
// - Real-time optimization with level-of-detail systems
//
// Performance Advantages over Unity Physics:
// - GPU-accelerated soft body and fluid simulation
// - Advanced material models beyond simple springs
// - Real-time destruction with proper fracture mechanics
// - Better parallelization and cache optimization
// - Integration with our superior ECS architecture
//
// Public APIs:
// - PhysicsWorld: Main physics simulation container
// - SoftBody: Deformable objects with realistic physics
// - Fluid: Liquid and gas simulation volumes
// - DestructionSystem: Procedural fracture and debris generation
// - Cloth: Fabric simulation with environmental interaction
//
// Ownership: PhysicsWorld owns all physics objects
// Invariants: Physics objects must be updated each frame for stability
//
#ifndef ADVANCED_PHYSICS_H
#define ADVANCED_PHYSICS_H

#include "../../common.h"
#include "../ecs/ecs.h"
#include "../core/performance.h"
#include <stdbool.h>

// ============================================================================
// CORE PHYSICS TYPES
// ============================================================================

// Physics material properties
typedef struct {
  f32 density;
  f32 friction;
  f32 restitution;
  f32 youngs_modulus;      // For soft body/destruction
  f32 poisson_ratio;       // Material compressibility
  f32 yield_strength;      // For plastic deformation
  f32 viscosity;           // For fluid simulation
  f32 surface_tension;     // For fluid simulation
  u32 material_id;         // Material type identifier
  bool is_deformable;      // Can undergo plastic deformation
  bool is_fracturable;     // Can break apart
} PhysicsMaterial;

// Collision shapes
typedef enum {
  COLLISION_SHAPE_SPHERE = 0,
  COLLISION_SHAPE_BOX,
  COLLISION_SHAPE_CAPSULE,
  COLLISION_SHAPE_CYLINDER,
  COLLISION_SHAPE_CONVEX_MESH,
  COLLISION_SHAPE_TRIANGLE_MESH,
  COLLISION_SHAPE_HEIGHTFIELD,
  COLLISION_SHAPE_SOFT_BODY,
  COLLISION_SHAPE_FLUID_VOLUME
} CollisionShapeType;

// Rigid body (basic physics object)
typedef struct {
  Entity entity;
  CollisionShapeType shape_type;
  void *shape_data;
  PhysicsMaterial material;
  vec3 position;
  vec3 velocity;
  vec3 angular_velocity;
  vec3 force;
  vec3 torque;
  quat orientation;
  f32 mass;
  mat3 inertia_tensor;
  bool is_static;
  bool is_kinematic;
  bool is_sleeping;
  u32 collision_layer;
  u32 collision_mask;
} RigidBody;

// ============================================================================
// SOFT BODY PHYSICS
// ============================================================================

// Soft body simulation method
typedef enum {
  SOFT_BODY_METHOD_FEM = 0,        // Finite Element Method
  SOFT_BODY_METHOD_MASS_SPRING,    // Mass-Spring System
  SOFT_BODY_METHOD_XPBD,           // Extended Position-Based Dynamics
  SOFT_BODY_METHOD_SHAPE_MATCHING  // Shape Matching
} SoftBodyMethod;

// Soft body particle
typedef struct {
  vec3 position;
  vec3 old_position;
  vec3 velocity;
  vec3 force;
  f32 mass;
  f32 inv_mass;
  bool is_fixed;
  u32 particle_id;
} SoftBodyParticle;

// Soft body constraint
typedef struct {
  u32 particle_a;
  u32 particle_b;
  f32 rest_length;
  f32 stiffness;
  f32 damping;
  bool is_active;
} SoftBodyConstraint;

// Soft body mesh
typedef struct {
  vec3 *vertices;
  vec3 *normals;
  vec2 *uv_coords;
  u32 *indices;
  u32 vertex_count;
  u32 index_count;
  u32 *particle_indices;  // Map vertices to particles
} SoftBodyMesh;

// Soft body object
typedef struct {
  Entity entity;
  SoftBodyMethod method;
  PhysicsMaterial material;
  SoftBodyParticle *particles;
  u32 particle_count;
  SoftBodyConstraint *constraints;
  u32 constraint_count;
  SoftBodyMesh mesh;
  vec3 bounding_box_min;
  vec3 bounding_box_max;
  f32 volume;
  bool self_collision;
  bool is_sleeping;
  u32 update_frequency;  // Can update less frequently than rigid bodies
} SoftBody;

// ============================================================================
// FLUID DYNAMICS
// ============================================================================

// Fluid simulation method
typedef enum {
  FLUID_METHOD_SPH = 0,           // Smoothed Particle Hydrodynamics
  FLUID_METHOD_LBM,              // Lattice Boltzmann Method
  FLUID_METHOD_GRID,             // Grid-based Navier-Stokes
  FLUID_METHOD_HYBRID            // Hybrid method combining approaches
} FluidMethod;

// Fluid particle (for SPH)
typedef struct {
  vec3 position;
  vec3 velocity;
  vec3 acceleration;
  f32 density;
  f32 pressure;
  f32 mass;
  u32 particle_id;
  u32 *neighbors;
  u32 neighbor_count;
} FluidParticle;

// Fluid grid cell (for grid-based methods)
typedef struct {
  vec3 velocity;
  f32 pressure;
  f32 density;
  bool is_solid;
  bool is_fluid;
  vec3 *particles;
  u32 particle_count;
} FluidCell;

// Fluid volume
typedef struct {
  Entity entity;
  FluidMethod method;
  PhysicsMaterial material;
  vec3 bounding_box_min;
  vec3 bounding_box_max;
  u32 grid_resolution_x;
  u32 grid_resolution_y;
  u32 grid_resolution_z;
  FluidCell *grid_cells;
  FluidParticle *particles;
  u32 particle_count;
  u32 max_particles;
  f32 particle_radius;
  f32 smoothing_radius;
  f32 rest_density;
  f32 gas_constant;
  f32 viscosity_coefficient;
  f32 surface_tension_coefficient;
  bool is_incompressible;
  bool self_collision;
} FluidVolume;

// ============================================================================
// DESTRUCTION SYSTEM
// ============================================================================

// Fracture method
typedef enum {
  FRACTURE_METHOD_VORONOI = 0,    // Voronoi diagram-based fracture
  FRACTURE_METHOD_STRESS_BASED,    // Stress-based crack propagation
  FRACTURE_METHOD_PATTERN_BASED,   // Pre-defined fracture patterns
  FRACTURE_METHOD_PROCEDURAL       // Procedural fracture generation
} FractureMethod;

// Stress tensor for fracture calculation
typedef struct {
  mat3 tensor;
  f32 max_principal_stress;
  vec3 principal_direction;
  bool exceeds_yield;
} StressTensor;

// Debris piece
typedef struct {
  Entity entity;
  vec3 *vertices;
  u32 vertex_count;
  u32 *indices;
  u32 index_count;
  vec3 velocity;
  vec3 angular_velocity;
  f32 mass;
  f32 lifetime;
  bool is_active;
} DebrisPiece;

// Destruction object
typedef struct {
  Entity original_entity;
  FractureMethod method;
  PhysicsMaterial material;
  StressTensor *stress_field;
  u32 stress_resolution_x;
  u32 stress_resolution_y;
  u32 stress_resolution_z;
  DebrisPiece *debris_pieces;
  u32 debris_count;
  f32 fracture_threshold;
  f32 crack_propagation_speed;
  bool is_fractured;
  u32 max_debris_pieces;
} DestructionObject;

// ============================================================================
// CLOTH SIMULATION
// ============================================================================

// Cloth constraint type
typedef enum {
  CLOTH_CONSTRAINT_STRETCH = 0,    // Distance constraint
  CLOTH_CONSTRAINT_BEND,           // Bending constraint
  CLOTH_CONSTRAINT_SHEAR,          // Shear constraint
  CLOTH_CONSTRAINT_TRIANGLE        // Triangle area constraint
} ClothConstraintType;

// Cloth particle
typedef struct {
  vec3 position;
  vec3 old_position;
  vec3 velocity;
  vec3 force;
  f32 mass;
  f32 inv_mass;
  bool is_fixed;
  u32 particle_id;
} ClothParticle;

// Cloth constraint
typedef struct {
  ClothConstraintType type;
  u32 particle_a;
  u32 particle_b;
  u32 particle_c;  // For triangle constraints
  f32 rest_length;
  f32 rest_angle;
  f32 stiffness;
  f32 damping;
  bool is_active;
} ClothConstraint;

// Cloth object
typedef struct {
  Entity entity;
  PhysicsMaterial material;
  ClothParticle *particles;
  u32 particle_count;
  ClothConstraint *constraints;
  u32 constraint_count;
  u32 *triangles;
  u32 triangle_count;
  vec2 *uv_coords;
  u32 width;
  u32 height;
  f32 thickness;
  bool self_collision;
  bool is_sleeping;
  u32 update_frequency;
} Cloth;

// ============================================================================
// VEHICLE PHYSICS
// ============================================================================

// Tire model
typedef struct {
  f32 radius;
  f32 width;
  f32 mass;
  f32 friction_coefficient;
  f32 rolling_resistance;
  f32 slip_ratio;
  f32 slip_angle;
  f32 cornering_stiffness;
  f32 camber_stiffness;
  bool is_steering;
  bool is_driven;
  f32 steering_angle;
  f32 torque;
  f32 brake_torque;
  f32 rotation_speed;
} Tire;

// Suspension system
typedef struct {
  f32 spring_constant;
  f32 damping_coefficient;
  f32 rest_length;
  f32 current_length;
  f32 max_compression;
  f32 max_extension;
  f32 preload_force;
  vec3 attachment_point;
  vec3 wheel_direction;
  bool is_active;
} Suspension;

// Vehicle chassis
typedef struct {
  Entity entity;
  PhysicsMaterial material;
  f32 mass;
  mat3 inertia_tensor;
  vec3 center_of_mass;
  f32 drag_coefficient;
  f32 lift_coefficient;
  f32 frontal_area;
  f32 engine_power;
  f32 max_speed;
  f32 acceleration;
  f32 braking_force;
  Tire tires[4];
  Suspension suspensions[4];
  vec3 tire_positions[4];
  bool all_wheel_drive;
  bool has_abs;
  bool has_traction_control;
} VehicleChassis;

// ============================================================================
// PHYSICS WORLD
// ============================================================================

// Physics configuration
typedef struct {
  vec3 gravity;
  f32 time_step;
  u32 max_substeps;
  u32 solver_iterations;
  u32 broadphase_algorithm;
  u32 narrowphase_algorithm;
  bool enable_sleeping;
  bool enable_warm_starting;
  f32 sleeping_threshold;
  f32 baumgarte_factor;  // For constraint stabilization
  u32 worker_threads;
  bool enable_gpu_acceleration;
  bool enable_profiling;
} PhysicsConfig;

// Collision pair
typedef struct {
  RigidBody *body_a;
  RigidBody *body_b;
  SoftBody *soft_a;
  SoftBody *soft_b;
  FluidVolume *fluid_a;
  FluidVolume *fluid_b;
  Cloth *cloth_a;
  Cloth *cloth_b;
  vec3 contact_points[8];
  vec3 contact_normals[8];
  f32 penetration_depths[8];
  u32 contact_count;
  bool has_collision;
} CollisionPair;

// Main physics world
typedef struct {
  // Configuration
  PhysicsConfig config;
  
  // Rigid bodies
  RigidBody *rigid_bodies;
  u32 rigid_body_count;
  u32 max_rigid_bodies;
  
  // Soft bodies
  SoftBody *soft_bodies;
  u32 soft_body_count;
  u32 max_soft_bodies;
  
  // Fluid volumes
  FluidVolume *fluid_volumes;
  u32 fluid_volume_count;
  u32 max_fluid_volumes;
  
  // Destruction objects
  DestructionObject *destruction_objects;
  u32 destruction_object_count;
  u32 max_destruction_objects;
  
  // Cloth objects
  Cloth *cloth_objects;
  u32 cloth_object_count;
  u32 max_cloth_objects;
  
  // Vehicles
  VehicleChassis *vehicles;
  u32 vehicle_count;
  u32 max_vehicles;
  
  // Collision detection
  CollisionPair *collision_pairs;
  u32 collision_pair_count;
  u32 max_collision_pairs;
  
  // Materials
  PhysicsMaterial *materials;
  u32 material_count;
  u32 max_materials;
  
  // Threading
  void *worker_threads;
  u32 worker_thread_count;
  
  // GPU acceleration
  void *gpu_context;
  bool gpu_available;
  
  // Performance
  Profiler *physics_profiler;
  f64 total_simulation_time;
  f64 collision_detection_time;
  f64 constraint_solving_time;
  
  // ECS integration
  World *ecs_world;
} PhysicsWorld;

// ============================================================================
// PUBLIC API
// ============================================================================

// World management
PhysicsWorld *physics_world_create(const PhysicsConfig *config, World *ecs_world);
void physics_world_destroy(PhysicsWorld *world);
void physics_world_update(PhysicsWorld *world, f32 delta_time);
void physics_world_step(PhysicsWorld *world, f32 time_step);

// Configuration
PhysicsConfig physics_create_default_config(void);
PhysicsConfig physics_create_high_performance_config(void);
PhysicsConfig physics_create_realistic_config(void);

// Material management
u32 physics_add_material(PhysicsWorld *world, const PhysicsMaterial *material);
PhysicsMaterial *physics_get_material(PhysicsWorld *world, u32 material_id);

// ============================================================================
// RIGID BODY API
// ============================================================================

// Rigid body creation and management
RigidBody *physics_create_rigid_body(PhysicsWorld *world, Entity entity);
void physics_destroy_rigid_body(PhysicsWorld *world, RigidBody *body);
bool physics_set_rigid_body_shape(PhysicsWorld *world, RigidBody *body, CollisionShapeType type, void *shape_data);
bool physics_set_rigid_body_material(PhysicsWorld *world, RigidBody *body, u32 material_id);

// Rigid body manipulation
void physics_set_rigid_body_position(PhysicsWorld *world, RigidBody *body, vec3 position);
void physics_set_rigid_body_velocity(PhysicsWorld *world, RigidBody *body, vec3 velocity);
void physics_set_rigid_body_orientation(PhysicsWorld *world, RigidBody *body, quat orientation);
void physics_apply_force(PhysicsWorld *world, RigidBody *body, vec3 force);
void physics_apply_impulse(PhysicsWorld *world, RigidBody *body, vec3 impulse);

// ============================================================================
// SOFT BODY API
// ============================================================================

// Soft body creation and management
SoftBody *physics_create_soft_body(PhysicsWorld *world, Entity entity, SoftBodyMethod method);
void physics_destroy_soft_body(PhysicsWorld *world, SoftBody *soft_body);
bool physics_set_soft_body_mesh(PhysicsWorld *world, SoftBody *soft_body, const SoftBodyMesh *mesh);
bool physics_set_soft_body_material(PhysicsWorld *world, SoftBody *soft_body, u32 material_id);

// Soft body manipulation
void physics_set_soft_body_position(PhysicsWorld *world, SoftBody *soft_body, vec3 position);
void physics_set_soft_body_velocity(PhysicsWorld *world, SoftBody *soft_body, vec3 velocity);
void physics_fix_soft_body_particle(PhysicsWorld *world, SoftBody *soft_body, u32 particle_id, bool fixed);
void physics_apply_soft_body_force(PhysicsWorld *world, SoftBody *soft_body, u32 particle_id, vec3 force);

// ============================================================================
// FLUID DYNAMICS API
// ============================================================================

// Fluid volume creation and management
FluidVolume *physics_create_fluid_volume(PhysicsWorld *world, Entity entity, FluidMethod method);
void physics_destroy_fluid_volume(PhysicsWorld *world, FluidVolume *fluid);
bool physics_set_fluid_volume_bounds(PhysicsWorld *world, FluidVolume *fluid, vec3 min_bound, vec3 max_bound);
bool physics_set_fluid_volume_material(PhysicsWorld *world, FluidVolume *fluid, u32 material_id);

// Fluid manipulation
void physics_set_fluid_volume_velocity(PhysicsWorld *world, FluidVolume *fluid, vec3 velocity);
void physics_add_fluid_particle(PhysicsWorld *world, FluidVolume *fluid, vec3 position, vec3 velocity);
void physics_remove_fluid_particle(PhysicsWorld *world, FluidVolume *fluid, u32 particle_id);
void physics_apply_fluid_force(PhysicsWorld *world, FluidVolume *fluid, vec3 position, vec3 force);

// ============================================================================
// DESTRUCTION API
// ============================================================================

// Destruction object creation and management
DestructionObject *physics_create_destruction_object(PhysicsWorld *world, Entity entity, FractureMethod method);
void physics_destroy_destruction_object(PhysicsWorld *world, DestructionObject *destruction);
bool physics_set_destruction_object_mesh(PhysicsWorld *world, DestructionObject *destruction, const SoftBodyMesh *mesh);
bool physics_set_destruction_object_material(PhysicsWorld *world, DestructionObject *destruction, u32 material_id);

// Destruction manipulation
void physics_apply_destruction_force(PhysicsWorld *world, DestructionObject *destruction, vec3 position, vec3 force);
void physics_fracture_object(PhysicsWorld *world, DestructionObject *destruction, vec3 impact_point, vec3 impact_direction);
DebrisPiece *physics_get_debris_pieces(PhysicsWorld *world, DestructionObject *destruction, u32 *count);

// ============================================================================
// CLOTH API
// ============================================================================

// Cloth creation and management
Cloth *physics_create_cloth(PhysicsWorld *world, Entity entity, u32 width, u32 height);
void physics_destroy_cloth(PhysicsWorld *world, Cloth *cloth);
bool physics_set_cloth_material(PhysicsWorld *world, Cloth *cloth, u32 material_id);
bool physics_set_cloth_triangles(PhysicsWorld *world, Cloth *cloth, u32 *triangles, u32 triangle_count);

// Cloth manipulation
void physics_set_cloth_position(PhysicsWorld *world, Cloth *cloth, vec3 position);
void physics_set_cloth_velocity(PhysicsWorld *world, Cloth *cloth, vec3 velocity);
void physics_fix_cloth_particle(PhysicsWorld *world, Cloth *cloth, u32 particle_id, bool fixed);
void physics_apply_cloth_force(PhysicsWorld *world, Cloth *cloth, u32 particle_id, vec3 force);
void physics_apply_cloth_wind(PhysicsWorld *world, Cloth *cloth, vec3 wind_velocity);

// ============================================================================
// VEHICLE API
// ============================================================================

// Vehicle creation and management
VehicleChassis *physics_create_vehicle(PhysicsWorld *world, Entity entity);
void physics_destroy_vehicle(PhysicsWorld *world, VehicleChassis *vehicle);
bool physics_set_vehicle_chassis(PhysicsWorld *world, VehicleChassis *vehicle, f32 mass, vec3 center_of_mass);
bool physics_set_vehicle_tires(PhysicsWorld *world, VehicleChassis *vehicle, const Tire *tires);
bool physics_set_vehicle_suspension(PhysicsWorld *world, VehicleChassis *vehicle, const Suspension *suspensions);

// Vehicle control
void physics_set_vehicle_throttle(PhysicsWorld *world, VehicleChassis *vehicle, f32 throttle);
void physics_set_vehicle_brake(PhysicsWorld *world, VehicleChassis *vehicle, f32 brake);
void physics_set_vehicle_steering(PhysicsWorld *world, VehicleChassis *vehicle, f32 steering_angle);
void physics_set_vehicle_gear(PhysicsWorld *world, VehicleChassis *vehicle, u32 gear);

// ============================================================================
// COLLISION DETECTION API
// ============================================================================

// Collision queries
bool physics_ray_cast(PhysicsWorld *world, vec3 origin, vec3 direction, f32 max_distance, vec3 *hit_point, vec3 *hit_normal, Entity *hit_entity);
bool physics_sphere_cast(PhysicsWorld *world, vec3 center, f32 radius, vec3 *hit_point, vec3 *hit_normal, Entity *hit_entity);
u32 physics_box_cast(PhysicsWorld *world, vec3 center, vec3 size, quat orientation, Entity *hit_entities, u32 max_hits);

// Collision filtering
void physics_set_collision_layer(PhysicsWorld *world, RigidBody *body, u32 layer);
void physics_set_collision_mask(PhysicsWorld *world, RigidBody *body, u32 mask);
bool physics_should_collide(PhysicsWorld *world, Entity entity_a, Entity entity_b);

// ============================================================================
// PERFORMANCE OPTIMIZATION
// ============================================================================

// Level of detail
void physics_set_lod_level(PhysicsWorld *world, SoftBody *soft_body, u32 lod_level);
void physics_set_update_frequency(PhysicsWorld *world, SoftBody *soft_body, u32 frequency);
void physics_set_update_frequency(PhysicsWorld *world, Cloth *cloth, u32 frequency);

// Sleeping optimization
void physics_enable_sleeping(PhysicsWorld *world, bool enable);
void physics_wake_up_object(PhysicsWorld *world, RigidBody *body);
void physics_put_to_sleep(PhysicsWorld *world, RigidBody *body);

// GPU acceleration
bool physics_enable_gpu_acceleration(PhysicsWorld *world, bool enable);
bool physics_is_gpu_available(PhysicsWorld *world);

// ============================================================================
// DEBUGGING AND PROFILING
// ============================================================================

// Debug rendering
void physics_debug_render(PhysicsWorld *world);
void physics_debug_render_soft_body(PhysicsWorld *world, SoftBody *soft_body);
void physics_debug_render_fluid(PhysicsWorld *world, FluidVolume *fluid);
void physics_debug_render_cloth(PhysicsWorld *world, Cloth *cloth);

// Performance statistics
typedef struct {
  u32 rigid_body_count;
  u32 soft_body_count;
  u32 fluid_particle_count;
  u32 cloth_particle_count;
  u32 collision_pair_count;
  f64 simulation_time;
  f64 collision_time;
  f64 solving_time;
  f64 gpu_time;
  f64 total_time;
} PhysicsStats;

PhysicsStats physics_get_stats(PhysicsWorld *world);
void physics_print_performance_report(PhysicsWorld *world);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Material creation macros
#define PHYSICS_MATERIAL_CREATE(density, friction, restitution) \
  (PhysicsMaterial){ \
    .density = density, \
    .friction = friction, \
    .restitution = restitution, \
    .youngs_modulus = 1e6f, \
    .poisson_ratio = 0.3f, \
    .yield_strength = 1e6f, \
    .viscosity = 0.001f, \
    .surface_tension = 0.0728f, \
    .material_id = 0, \
    .is_deformable = false, \
    .is_fracturable = false \
  }

#define PHYSICS_SOFT_MATERIAL_CREATE(density, friction, restitution, youngs, poisson) \
  (PhysicsMaterial){ \
    .density = density, \
    .friction = friction, \
    .restitution = restitution, \
    .youngs_modulus = youngs, \
    .poisson_ratio = poisson, \
    .yield_strength = 1e6f, \
    .viscosity = 0.001f, \
    .surface_tension = 0.0728f, \
    .material_id = 0, \
    .is_deformable = true, \
    .is_fracturable = false \
  }

#define PHYSICS_FLUID_MATERIAL_CREATE(density, viscosity, surface_tension) \
  (PhysicsMaterial){ \
    .density = density, \
    .friction = 0.01f, \
    .restitution = 0.0f, \
    .youngs_modulus = 0.0f, \
    .poisson_ratio = 0.5f, \
    .yield_strength = 0.0f, \
    .viscosity = viscosity, \
    .surface_tension = surface_tension, \
    .material_id = 0, \
    .is_deformable = false, \
    .is_fracturable = false \
  }

#endif // ADVANCED_PHYSICS_H

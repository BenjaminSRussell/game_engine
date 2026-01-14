// physics_enhanced.h
// Enhanced Physics System API with Advanced Features

#ifndef ENGINE_PHYSICS_ENHANCED_H
#define ENGINE_PHYSICS_ENHANCED_H

#include "physics.h"
#include <stdbool.h>
#include <stdint.h>

// -----------------------------------------------------------------------------
// Advanced Physics Types
// -----------------------------------------------------------------------------

typedef enum {
    CONSTRAINT_TYPE_FIXED,
    CONSTRAINT_TYPE_HINGE,
    CONSTRAINT_TYPE_SLIDER,
    CONSTRAINT_TYPE_SPRING,
    CONSTRAINT_TYPE_BALL_SOCKET,
    CONSTRAINT_TYPE_CONE_TWIST
} ConstraintType;

typedef enum {
    SOFT_BODY_TYPE_CLOTH,
    SOFT_BODY_TYPE_ROPE,
    SOFT_BODY_TYPE_JELLY,
    SOFT_BODY_TYPE_BALLOON
} SoftBodyType;

typedef enum {
    FLUID_TYPE_WATER,
    FLUID_TYPE_OIL,
    FLUID_TYPE_AIR,
    FLUID_TYPE_CUSTOM
} FluidType;

typedef enum {
    DESTRUCTION_TYPE_FRACTURE,
    DESTRUCTION_TYPE_EXPLOSION,
    DESTRUCTION_TYPE_IMPACT
} DestructionType;

// -----------------------------------------------------------------------------
// Advanced Data Structures
// -----------------------------------------------------------------------------

typedef struct {
    ConstraintType type;
    RigidBody* body_a;
    RigidBody* body_b;
    Vec3 anchor_a;
    Vec3 anchor_b;
    Vec3 axis;
    f32 lower_limit;
    f32 upper_limit;
    f32 stiffness;
    f32 damping;
    bool enabled;
    void* user_data;
} PhysicsConstraint;

typedef struct {
    SoftBodyType type;
    Vec3* particles;
    u32 particle_count;
    u32* indices;
    u32 index_count;
    f32 mass;
    f32 stiffness;
    f32 damping;
    f32 thickness;
    bool self_collision;
    void* user_data;
} SoftBody;

typedef struct {
    FluidType type;
    Vec3* particles;
    u32 particle_count;
    f32 particle_radius;
    f32 density;
    f32 viscosity;
    f32 pressure_scale;
    f32 surface_tension;
    Vec3 bounds_min;
    Vec3 bounds_max;
    void* user_data;
} FluidSimulation;

typedef struct {
    Vec3 position;
    Vec3 velocity;
    Vec3 angular_velocity;
    f32 mass;
    f32 radius;
    f32 damage;
    f32 lifetime;
    bool active;
} Explosion;

typedef struct {
    Vec3 position;
    Vec3 normal;
    f32 force;
    f32 radius;
    f32 impulse;
    bool active;
} Impact;

typedef struct {
    Vec3 position;
    Vec3 size;
    Vec3 velocity;
    Vec3 angular_velocity;
    f32 mass;
    f32 fracture_threshold;
    f32 fragment_count;
    bool fractured;
    RigidBody* fragments;
    u32 max_fragments;
} DestructibleObject;

// -----------------------------------------------------------------------------
// Advanced Physics World
// -----------------------------------------------------------------------------

typedef struct {
    PhysicsWorld base;
    
    // Advanced systems
    PhysicsConstraint* constraints;
    u32 constraint_count;
    u32 constraint_capacity;
    
    SoftBody* soft_bodies;
    u32 soft_body_count;
    u32 soft_body_capacity;
    
    FluidSimulation* fluids;
    u32 fluid_count;
    u32 fluid_capacity;
    
    Explosion* explosions;
    u32 explosion_count;
    u32 explosion_capacity;
    
    Impact* impacts;
    u32 impact_count;
    u32 impact_capacity;
    
    DestructibleObject* destructibles;
    u32 destructible_count;
    u32 destructible_capacity;
    
    // Advanced settings
    bool soft_body_enabled;
    bool fluid_simulation_enabled;
    bool destruction_enabled;
    f32 global_damping;
    f32 max_velocity;
    f32 max_angular_velocity;
} EnhancedPhysicsWorld;

// -----------------------------------------------------------------------------
// Advanced Physics API
// -----------------------------------------------------------------------------

// World management
EnhancedPhysicsWorld* enhanced_physics_world_create(PhysicsConfig config);
void enhanced_physics_world_destroy(EnhancedPhysicsWorld* world);
void enhanced_physics_world_step(EnhancedPhysicsWorld* world, f32 delta_time);

// Constraint system
PhysicsConstraint* physics_create_constraint(EnhancedPhysicsWorld* world, ConstraintType type,
                                           RigidBody* body_a, RigidBody* body_b);
void physics_destroy_constraint(EnhancedPhysicsWorld* world, PhysicsConstraint* constraint);
void physics_constraint_set_limits(PhysicsConstraint* constraint, f32 lower, f32 upper);
void physics_constraint_set_stiffness(PhysicsConstraint* constraint, f32 stiffness);
void physics_constraint_set_damping(PhysicsConstraint* constraint, f32 damping);
void physics_constraint_enable(PhysicsConstraint* constraint, bool enabled);

// Soft body system
SoftBody* physics_create_soft_body(EnhancedPhysicsWorld* world, SoftBodyType type,
                                  u32 particle_count, u32 index_count);
void physics_destroy_soft_body(EnhancedPhysicsWorld* world, SoftBody* soft_body);
void physics_soft_body_set_mass(SoftBody* soft_body, f32 mass);
void physics_soft_body_set_stiffness(SoftBody* soft_body, f32 stiffness);
void physics_soft_body_set_damping(SoftBody* soft_body, f32 damping);
void physics_soft_body_apply_force(SoftBody* soft_body, Vec3 force, u32 particle_index);
void physics_soft_body_pin_particle(SoftBody* soft_body, u32 particle_index, Vec3 position);

// Fluid simulation
FluidSimulation* physics_create_fluid_simulation(EnhancedPhysicsWorld* world, FluidType type,
                                             u32 particle_count);
void physics_destroy_fluid_simulation(EnhancedPhysicsWorld* world, FluidSimulation* fluid);
void physics_fluid_set_density(FluidSimulation* fluid, f32 density);
void physics_fluid_set_viscosity(FluidSimulation* fluid, f32 viscosity);
void physics_fluid_add_particle(FluidSimulation* fluid, Vec3 position, Vec3 velocity);
void physics_fluid_apply_force(FluidSimulation* fluid, Vec3 force, Vec3 position, f32 radius);

// Destruction system
DestructibleObject* physics_create_destructible_object(EnhancedPhysicsWorld* world,
                                                    Vec3 position, Vec3 size, f32 mass);
void physics_destroy_destructible_object(EnhancedPhysicsWorld* world, DestructibleObject* obj);
void physics_destructible_set_fracture_threshold(DestructibleObject* obj, f32 threshold);
void physics_destructible_apply_impact(DestructibleObject* obj, Vec3 position, Vec3 force);
bool physics_destructible_is_fractured(const DestructibleObject* obj);
RigidBody* physics_destructible_get_fragments(const DestructibleObject* obj, u32* out_count);

// Explosion system
Explosion* physics_create_explosion(EnhancedPhysicsWorld* world, Vec3 position, f32 radius, f32 force);
void physics_destroy_explosion(EnhancedPhysicsWorld* world, Explosion* explosion);
void physics_explosion_set_damage(Explosion* explosion, f32 damage);
void physics_explosion_set_lifetime(Explosion* explosion, f32 lifetime);
bool physics_explosion_is_active(const Explosion* explosion);

// Impact system
Impact* physics_create_impact(EnhancedPhysicsWorld* world, Vec3 position, Vec3 normal, f32 force);
void physics_destroy_impact(EnhancedPhysicsWorld* world, Impact* impact);
void physics_impact_set_radius(Impact* impact, f32 radius);
void physics_impact_set_impulse(Impact* impact, f32 impulse);
bool physics_impact_is_active(const Impact* impact);

// Advanced queries
u32 physics_query_constraints_in_sphere(EnhancedPhysicsWorld* world, Vec3 center, f32 radius,
                                    PhysicsConstraint** out_constraints, u32 max_count);
u32 physics_query_soft_bodies_in_box(EnhancedPhysicsWorld* world, Vec3 min_bounds, Vec3 max_bounds,
                                    SoftBody** out_soft_bodies, u32 max_count);
u32 physics_query_fluid_particles_in_sphere(EnhancedPhysicsWorld* world, Vec3 center, f32 radius,
                                          u32* out_particle_indices, u32 max_count);

// Advanced raycasting
typedef struct {
    Vec3 point;
    Vec3 normal;
    f32 distance;
    void* hit_object;
    enum {
        HIT_TYPE_RIGID_BODY,
        HIT_TYPE_SOFT_BODY,
        HIT_TYPE_FLUID,
        HIT_TYPE_CONSTRAINT
    } hit_type;
} EnhancedRaycastResult;

EnhancedRaycastResult enhanced_physics_raycast(EnhancedPhysicsWorld* world, 
                                           Vec3 origin, Vec3 direction, f32 max_distance);

// Performance monitoring
typedef struct {
    u32 rigid_body_count;
    u32 constraint_count;
    u32 soft_body_count;
    u32 fluid_particle_count;
    u32 explosion_count;
    u32 impact_count;
    f64 constraint_solve_time_ms;
    f64 soft_body_time_ms;
    f64 fluid_time_ms;
    f64 destruction_time_ms;
    f64 total_time_ms;
} EnhancedPhysicsPerformance;

const EnhancedPhysicsPerformance* enhanced_physics_get_performance_stats(EnhancedPhysicsWorld* world);

// Settings
void enhanced_physics_set_global_damping(EnhancedPhysicsWorld* world, f32 damping);
void enhanced_physics_set_max_velocity(EnhancedPhysicsWorld* world, f32 max_velocity);
void enhanced_physics_set_max_angular_velocity(EnhancedPhysicsWorld* world, f32 max_angular_velocity);
void enhanced_physics_enable_soft_bodies(EnhancedPhysicsWorld* world, bool enabled);
void enhanced_physics_enable_fluid_simulation(EnhancedPhysicsWorld* world, bool enabled);
void enhanced_physics_enable_destruction(EnhancedPhysicsWorld* world, bool enabled);

// -----------------------------------------------------------------------------
// Material System
// -----------------------------------------------------------------------------

typedef struct {
    char name[64];
    f32 density;
    f32 friction;
    f32 restitution;
    f32 youngs_modulus;
    f32 poissons_ratio;
    f32 yield_strength;
    f32 ultimate_strength;
    f32 hardness;
    f32 toughness;
    bool is_destructible;
} AdvancedPhysicsMaterial;

// Material management
AdvancedPhysicsMaterial* physics_material_create_advanced(const char* name);
void physics_material_destroy_advanced(AdvancedPhysicsMaterial* material);
void physics_material_set_density(AdvancedPhysicsMaterial* material, f32 density);
void physics_material_set_mechanical_properties(AdvancedPhysicsMaterial* material,
                                            f32 youngs_modulus, f32 poissons_ratio);
void physics_material_set_strength_properties(AdvancedPhysicsMaterial* material,
                                         f32 yield_strength, f32 ultimate_strength);
void physics_material_set_hardness(AdvancedPhysicsMaterial* material, f32 hardness);
void physics_material_set_toughness(AdvancedPhysicsMaterial* material, f32 toughness);

// Material presets
AdvancedPhysicsMaterial physics_material_get_concrete(void);
AdvancedPhysicsMaterial physics_material_get_steel(void);
AdvancedPhysicsMaterial physics_material_get_wood(void);
AdvancedPhysicsMaterial physics_material_get_glass(void);
AdvancedPhysicsMaterial physics_material_get_rubber(void);

// -----------------------------------------------------------------------------
// Debugging and Visualization
// -----------------------------------------------------------------------------

typedef struct {
    Vec3 start;
    Vec3 end;
    Vec3 color;
    f32 lifetime;
} PhysicsDebugLine;

typedef struct {
    Vec3 center;
    f32 radius;
    Vec3 color;
    f32 lifetime;
} PhysicsDebugSphere;

typedef struct {
    Vec3 min_bounds;
    Vec3 max_bounds;
    Vec3 color;
    f32 lifetime;
} PhysicsDebugBox;

// Debug visualization
void physics_debug_clear_lines(EnhancedPhysicsWorld* world);
void physics_debug_add_line(EnhancedPhysicsWorld* world, Vec3 start, Vec3 end, Vec3 color, f32 lifetime);
void physics_debug_add_sphere(EnhancedPhysicsWorld* world, Vec3 center, f32 radius, Vec3 color, f32 lifetime);
void physics_debug_add_box(EnhancedPhysicsWorld* world, Vec3 min_bounds, Vec3 max_bounds, Vec3 color, f32 lifetime);
void physics_debug_render_constraints(EnhancedPhysicsWorld* world, bool enabled);
void physics_debug_render_soft_bodies(EnhancedPhysicsWorld* world, bool enabled);
void physics_debug_render_fluid_particles(EnhancedPhysicsWorld* world, bool enabled);
void physics_debug_render_destruction(EnhancedPhysicsWorld* world, bool enabled);

// Debug information
u32 physics_debug_get_line_count(const EnhancedPhysicsWorld* world);
const PhysicsDebugLine* physics_debug_get_lines(const EnhancedPhysicsWorld* world);
u32 physics_debug_get_sphere_count(const EnhancedPhysicsWorld* world);
const PhysicsDebugSphere* physics_debug_get_spheres(const EnhancedPhysicsWorld* world);
u32 physics_debug_get_box_count(const EnhancedPhysicsWorld* world);
const PhysicsDebugBox* physics_debug_get_boxes(const EnhancedPhysicsWorld* world);

// -----------------------------------------------------------------------------
// Serialization and Networking
// -----------------------------------------------------------------------------

typedef struct {
    u32 magic;
    u32 version;
    u32 body_count;
    u32 constraint_count;
    u32 soft_body_count;
    u32 fluid_particle_count;
    u32 checksum;
} PhysicsSnapshotHeader;

// Snapshot system
bool physics_snapshot_save(EnhancedPhysicsWorld* world, const char* filename);
bool physics_snapshot_load(EnhancedPhysicsWorld* world, const char* filename);
bool physics_snapshot_capture_state(EnhancedPhysicsWorld* world, void** out_data, u32* out_size);
bool physics_snapshot_restore_state(EnhancedPhysicsWorld* world, const void* data, u32 size);

// Network replication
typedef struct {
    u32 body_id;
    Vec3 position;
    Quat rotation;
    Vec3 velocity;
    Vec3 angular_velocity;
    f32 timestamp;
} PhysicsBodyState;

typedef struct {
    PhysicsBodyState* bodies;
    u32 body_count;
    u32 frame_number;
    f64 timestamp;
} PhysicsNetworkUpdate;

PhysicsNetworkUpdate* physics_create_network_update(EnhancedPhysicsWorld* world, u32 frame_number);
void physics_destroy_network_update(PhysicsNetworkUpdate* update);
bool physics_apply_network_update(EnhancedPhysicsWorld* world, const PhysicsNetworkUpdate* update);
void physics_set_interpolation_method(EnhancedPhysicsWorld* world, int method);

// -----------------------------------------------------------------------------
// Threading and Optimization
// -----------------------------------------------------------------------------

typedef enum {
    PHYSICS_THREAD_MODE_SINGLE,
    PHYSICS_THREAD_MODE_MULTI,
    PHYSICS_THREAD_MODE_GPU_ACCELERATED
} PhysicsThreadMode;

typedef struct {
    PhysicsThreadMode thread_mode;
    u32 worker_thread_count;
    bool use_simd;
    bool use_gpu_acceleration;
    u32 batch_size;
    f32 max_thread_time_ms;
} PhysicsThreadingConfig;

// Threading control
void physics_set_threading_config(EnhancedPhysicsWorld* world, const PhysicsThreadingConfig* config);
PhysicsThreadingConfig physics_get_threading_config(const EnhancedPhysicsWorld* world);
void physics_set_worker_thread_count(EnhancedPhysicsWorld* world, u32 count);
void physics_enable_simd(EnhancedPhysicsWorld* world, bool enabled);
void physics_enable_gpu_acceleration(EnhancedPhysicsWorld* world, bool enabled);

// Performance optimization
void physics_optimize_broadphase(EnhancedPhysicsWorld* world);
void physics_optimize_constraint_ordering(EnhancedPhysicsWorld* world);
void physics_compact_memory(EnhancedPhysicsWorld* world);
void physics_warm_up_systems(EnhancedPhysicsWorld* world);

#endif // ENGINE_PHYSICS_ENHANCED_H

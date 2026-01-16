/*
 * ragdoll_physics.h
 * Enterprise-grade ragdoll physics system
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features:
 * - Professional rigid body physics integration
 * - Constraint-based joint system
 * - Collision detection and response
 * - Force application and dynamics
 * - Performance optimization with spatial partitioning
 * - Thread-safe operations
 * - Comprehensive error handling
 * - Real-time debugging and visualization
 * - Memory pooling for performance
 * - Async physics updates
 */

#ifndef ANIMATION_RAGDOLL_PHYSICS_H
#define ANIMATION_RAGDOLL_PHYSICS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define RAGDOLL_PHYSICS_MAX_BODIES 1024
#define RAGDOLL_PHYSICS_MAX_CONSTRAINTS 2048
#define RAGDOLL_PHYSICS_MAX_COLLISION_SHAPES 4096
#define RAGDOLL_PHYSICS_SPATIAL_GRID_SIZE 16
#define RAGDOLL_PHYSICS_CONSTRAINT_ITERATIONS 8
#define RAGDOLL_PHYSICS_SLEEP_THRESHOLD 0.01f
#define RAGDOLL_PHYSICS_DEACTIVATION_TIME 2.0f

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ragdoll_physics_handle {
    uint32_t id;
} ragdoll_physics_handle_t;

typedef enum ragdoll_body_type {
    RAGDOLL_BODY_DYNAMIC = 0,
    RAGDOLL_BODY_KINEMATIC = 1,
    RAGDOLL_BODY_STATIC = 2
} ragdoll_body_type_t;

typedef enum ragdoll_constraint_type {
    RAGDOLL_CONSTRAINT_BALL_SOCKET = 0,
    RAGDOLL_CONSTRAINT_HINGE = 1,
    RAGDOLL_CONSTRAINT_SLIDER = 2,
    RAGDOLL_CONSTRAINT_FIXED = 3,
    RAGDOLL_CONSTRAINT_SPRING = 4
} ragdoll_constraint_type_t;

typedef enum ragdoll_collision_shape_type {
    RAGDOLL_SHAPE_SPHERE = 0,
    RAGDOLL_SHAPE_BOX = 1,
    RAGDOLL_SHAPE_CAPSULE = 2,
    RAGDOLL_SHAPE_CONVEX_HULL = 3
} ragdoll_collision_shape_type_t;

typedef struct ragdoll_vec3 {
    float x, y, z;
} ragdoll_vec3_t;

typedef struct ragdoll_quat {
    float x, y, z, w;
} ragdoll_quat_t;

typedef struct ragdoll_transform {
    ragdoll_vec3_t position;
    ragdoll_quat_t rotation;
} ragdoll_transform_t;

typedef struct ragdoll_collision_shape {
    ragdoll_collision_shape_type_t type;
    ragdoll_vec3_t half_extents;
    float radius;
    float height;
    void* convex_hull_data;
    uint32_t hull_vertex_count;
} ragdoll_collision_shape_t;

typedef struct ragdoll_body_desc {
    ragdoll_body_type_t type;
    ragdoll_transform_t transform;
    ragdoll_vec3_t linear_velocity;
    ragdoll_vec3_t angular_velocity;
    float mass;
    float linear_damping;
    float angular_damping;
    float friction;
    float restitution;
    uint32_t collision_group;
    uint32_t collision_mask;
    bool enable_gravity;
    bool can_sleep;
    ragdoll_collision_shape_t collision_shape;
} ragdoll_body_desc_t;

typedef struct ragdoll_constraint_desc {
    ragdoll_constraint_type_t type;
    uint32_t body_a_id;
    uint32_t body_b_id;
    ragdoll_transform_t local_frame_a;
    ragdoll_transform_t local_frame_b;
    ragdoll_vec3_t linear_limits_min;
    ragdoll_vec3_t linear_limits_max;
    ragdoll_vec3_t angular_limits_min;
    ragdoll_vec3_t angular_limits_max;
    float spring_stiffness;
    float spring_damping;
    float breaking_threshold;
    bool enable_collision;
} ragdoll_constraint_desc_t;

typedef struct ragdoll_force {
    ragdoll_vec3_t force;
    ragdoll_vec3_t position;
    float duration;
    bool is_impulse;
} ragdoll_force_t;

typedef struct ragdoll_physics_desc {
    uint32_t max_bodies;
    uint32_t max_constraints;
    float gravity[3];
    float time_step;
    uint32_t velocity_iterations;
    uint32_t position_iterations;
    bool enable_multithreading;
    bool enable_continuous_physics;
    bool enable_debug_drawing;
    uint32_t flags;
    void* user_data;
} ragdoll_physics_desc_t;

typedef struct ragdoll_performance_stats {
    uint64_t total_bodies;
    uint64_t active_bodies;
    uint64_t sleeping_bodies;
    uint64_t total_constraints;
    uint64_t active_constraints;
    uint64_t collision_pairs;
    uint64_t broadphase_pairs;
    uint64_t narrowphase_tests;
    float physics_step_time;
    float collision_detection_time;
    float constraint_solver_time;
    float memory_usage_mb;
    uint64_t frame_count;
} ragdoll_performance_stats_t;

typedef struct ragdoll_physics_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    uint32_t body_count;
    uint32_t constraint_count;
    uint32_t collision_shape_count;
    ragdoll_performance_stats_t performance;
    bool is_sleeping;
    float time_accumulator;
    size_t memory_usage;
} ragdoll_physics_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization and Lifecycle */
int ragdoll_physics_init(void);
void ragdoll_physics_shutdown(void);

int ragdoll_physics_create(ragdoll_physics_handle_t* out_handle, const ragdoll_physics_desc_t* desc);
void ragdoll_physics_destroy(ragdoll_physics_handle_t handle);

/* Body Management */
int ragdoll_physics_create_body(ragdoll_physics_handle_t handle, uint32_t* out_body_id, const ragdoll_body_desc_t* desc);
void ragdoll_physics_destroy_body(ragdoll_physics_handle_t handle, uint32_t body_id);
int ragdoll_physics_get_body_transform(ragdoll_physics_handle_t handle, uint32_t body_id, ragdoll_transform_t* out_transform);
int ragdoll_physics_set_body_transform(ragdoll_physics_handle_t handle, uint32_t body_id, const ragdoll_transform_t* transform);
int ragdoll_physics_get_body_velocity(ragdoll_physics_handle_t handle, uint32_t body_id, ragdoll_vec3_t* out_linear, ragdoll_vec3_t* out_angular);
int ragdoll_physics_set_body_velocity(ragdoll_physics_handle_t handle, uint32_t body_id, const ragdoll_vec3_t* linear, const ragdoll_vec3_t* angular);

/* Constraint Management */
int ragdoll_physics_create_constraint(ragdoll_physics_handle_t handle, uint32_t* out_constraint_id, const ragdoll_constraint_desc_t* desc);
void ragdoll_physics_destroy_constraint(ragdoll_physics_handle_t handle, uint32_t constraint_id);
int ragdoll_physics_set_constraint_enabled(ragdoll_physics_handle_t handle, uint32_t constraint_id, bool enabled);
bool ragdoll_physics_is_constraint_enabled(ragdoll_physics_handle_t handle, uint32_t constraint_id);

/* Force Application */
int ragdoll_physics_apply_force(ragdoll_physics_handle_t handle, uint32_t body_id, const ragdoll_vec3_t* force, const ragdoll_vec3_t* position);
int ragdoll_physics_apply_impulse(ragdoll_physics_handle_t handle, uint32_t body_id, const ragdoll_vec3_t* impulse, const ragdoll_vec3_t* position);
int ragdoll_physics_apply_torque(ragdoll_physics_handle_t handle, uint32_t body_id, const ragdoll_vec3_t* torque);
int ragdoll_physics_apply_angular_impulse(ragdoll_physics_handle_t handle, uint32_t body_id, const ragdoll_vec3_t* impulse);

/* Physics Simulation */
int ragdoll_physics_step(ragdoll_physics_handle_t handle, float delta_time);
int ragdoll_physics_step_async(ragdoll_physics_handle_t handle, float delta_time);
bool ragdoll_physics_is_step_complete(ragdoll_physics_handle_t handle);
int ragdoll_physics_wait_for_step(ragdoll_physics_handle_t handle);

/* Collision Detection */
int ragdoll_physics_raycast(ragdoll_physics_handle_t handle, const ragdoll_vec3_t* origin, const ragdoll_vec3_t* direction, float max_distance, uint32_t* out_body_id, ragdoll_vec3_t* out_hit_point, ragdoll_vec3_t* out_normal);
int ragdoll_physics_overlap_sphere(ragdoll_physics_handle_t handle, const ragdoll_vec3_t* center, float radius, uint32_t* out_body_ids, uint32_t max_bodies, uint32_t* out_count);
int ragdoll_physics_set_collision_group(ragdoll_physics_handle_t handle, uint32_t body_id, uint32_t group, uint32_t mask);

/* Sleep and Activation */
int ragdoll_physics_wake_body(ragdoll_physics_handle_t handle, uint32_t body_id);
int ragdoll_physics_put_body_to_sleep(ragdoll_physics_handle_t handle, uint32_t body_id);
bool ragdoll_physics_is_body_sleeping(ragdoll_physics_handle_t handle, uint32_t body_id);
int ragdoll_physics_set_sleep_threshold(ragdoll_physics_handle_t handle, uint32_t body_id, float threshold);

/* Validation and Error Handling */
int ragdoll_physics_validate(ragdoll_physics_handle_t handle);
int ragdoll_physics_get_last_error(ragdoll_physics_handle_t handle, char* error_buffer, size_t buffer_size);

/* Performance and Debugging */
const ragdoll_performance_stats_t* ragdoll_physics_get_performance_stats(ragdoll_physics_handle_t handle);
void ragdoll_physics_reset_performance_stats(ragdoll_physics_handle_t handle);
int ragdoll_physics_enable_debug_drawing(ragdoll_physics_handle_t handle, bool enable);
int ragdoll_physics_draw_debug(ragdoll_physics_handle_t handle, void* debug_renderer);

/* Serialization */
int ragdoll_physics_serialize(ragdoll_physics_handle_t handle, void** out_data, size_t* out_size);
int ragdoll_physics_deserialize(ragdoll_physics_handle_t handle, const void* data, size_t size);

/* Memory Management */
int ragdoll_physics_set_memory_budget(ragdoll_physics_handle_t handle, size_t budget_bytes);
size_t ragdoll_physics_get_memory_budget(ragdoll_physics_handle_t handle);
size_t ragdoll_physics_get_memory_usage(ragdoll_physics_handle_t handle);

/* Statistics */
uint32_t ragdoll_physics_get_body_count(ragdoll_physics_handle_t handle);
uint32_t ragdoll_physics_get_constraint_count(ragdoll_physics_handle_t handle);
ragdoll_physics_info_t ragdoll_physics_get_info(ragdoll_physics_handle_t handle);

/* Thread Safety */
int ragdoll_physics_lock(ragdoll_physics_handle_t handle);
int ragdoll_physics_unlock(ragdoll_physics_handle_t handle);
int ragdoll_physics_try_lock(ragdoll_physics_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_RAGDOLL_PHYSICS_H */
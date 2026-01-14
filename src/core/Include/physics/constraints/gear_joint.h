#ifndef GEAR_JOINT_H
#define GEAR_JOINT_H

#include "physics/constraint_solver.h"
#include "math/vec3.h"
#include "math/quat.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Gear Joint Types
typedef enum {
    GEAR_TYPE_REVOLUTE,    // Rotational gear coupling
    GEAR_TYPE_PRISMATIC,   // Linear gear coupling (rack and pinion)
    GEAR_TYPE_WORM         // Worm gear coupling
} GearType;

// Gear Joint Context
typedef struct {
    EntityID body_a;
    EntityID body_b;
    
    // Gear parameters
    GearType type;
    float gear_ratio;      // Ratio of angular velocities (_b = gear_ratio * _a)
    float reference_angle_a;
    float reference_angle_b;
    
    // Joint axes (in local space)
    Vec3 axis_a;          // Rotation/translation axis for body A
    Vec3 axis_b;          // Rotation/translation axis for body B
    
    // Motor parameters
    bool motor_enabled;
    float motor_target_velocity;
    float motor_max_force;
    
    // Runtime data
    Vec3 world_axis_a;
    Vec3 world_axis_b;
    float current_angle_a;
    float current_angle_b;
    float current_velocity_a;
    float current_velocity_b;
    
    // Jacobian entries
    JacobianEntry jacobian_row;  // 1 constraint row
    
    // Constraint state
    float accumulated_impulse;
    float previous_lambda;
    
    // Warm starting data
    bool warm_started;
    
    // Breaking
    float break_torque;
    bool broken;
} GearJoint;

// Gear Joint Functions
GearJoint* gear_create(EntityID body_a, EntityID body_b, GearType type, float gear_ratio,
                      const Vec3* axis_a, const Vec3* axis_b);
void gear_destroy(GearJoint* gear);

// Constraint solving
void gear_prepare(GearJoint* gear, const RigidBody* body_a, const RigidBody* body_b);
void gear_solve_velocity(GearJoint* gear, RigidBody* body_a, RigidBody* body_b, float dt);
void gear_solve_position(GearJoint* gear, RigidBody* body_a, RigidBody* body_b, float dt);

// Motor control
void gear_set_motor(GearJoint* gear, float target_velocity, float max_force);
void gear_enable_motor(GearJoint* gear, bool enabled);

// Gear parameters
void gear_set_ratio(GearJoint* gear, float ratio);
void gear_set_reference_angles(GearJoint* gear, float angle_a, float angle_b);

// Breaking
void gear_set_break_torque(GearJoint* gear, float break_torque);
bool gear_is_broken(const GearJoint* gear);

// Warm starting
void gear_warm_start(GearJoint* gear);
void gear_save_impulse(GearJoint* gear);

// Utility functions
float gear_get_ratio(const GearJoint* gear);
float gear_get_angle_error(const GearJoint* gear);
float gear_get_velocity_error(const GearJoint* gear);

// Debug visualization
void gear_debug_draw(const GearJoint* gear);

#ifdef __cplusplus
}
#endif

#endif // GEAR_JOINT_H

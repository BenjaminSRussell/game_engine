#ifndef HINGE_JOINT_H
#define HINGE_JOINT_H

#include "physics/constraint_solver.h"
#include "math/vec3.h"
#include "math/quat.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hinge Joint Context
typedef struct {
    HingeConstraint constraint;
    
    // Runtime data
    Vec3 world_pivot_a;
    Vec3 world_pivot_b;
    Vec3 world_axis_a;
    Vec3 world_axis_b;
    
    // Jacobian entries for constraint rows
    JacobianEntry jacobian_rows[5]; // 3 linear + 2 angular constraints
    
    // Motor and limit state
    float current_angle;
    float angular_velocity;
    float accumulated_impulse[5];
    
    // Warm starting data
    bool warm_started;
    float previous_lambda[5];
} HingeJointContext;

// Hinge Joint Functions
HingeJointContext* hinge_create(EntityID body_a, EntityID body_b,
                               const Vec3* pivot_a, const Vec3* pivot_b,
                               const Vec3* axis_a, const Vec3* axis_b);
void hinge_destroy(HingeJointContext* hinge);

// Constraint solving
void hinge_prepare(HingeJointContext* hinge, const RigidBody* body_a, const RigidBody* body_b);
void hinge_solve_velocity(HingeJointContext* hinge, RigidBody* body_a, RigidBody* body_b, float dt);
void hinge_solve_position(HingeJointContext* hinge, RigidBody* body_a, RigidBody* body_b, float dt);

// Motor control
void hinge_set_motor_target(HingeJointContext* hinge, float target_velocity, float max_force);
void hinge_set_motor_position(HingeJointContext* hinge, float target_angle, float position_gain, float velocity_gain);
void hinge_enable_motor(HingeJointContext* hinge, bool enabled);

// Limit control
void hinge_set_angular_limits(HingeJointContext* hinge, float min_angle, float max_angle);
void hinge_disable_limits(HingeJointContext* hinge);
bool hinge_is_at_limit(const HingeJointContext* hinge);

// Friction
void hinge_set_friction(HingeJointContext* hinge, float friction_torque);

// Warm starting
void hinge_warm_start(HingeJointContext* hinge);
void hinge_save_impulse(HingeJointContext* hinge);

// Utility functions
float hinge_get_angle(const HingeJointContext* hinge);
float hinge_get_angular_velocity(const HingeJointContext* hinge);
bool hinge_is_broken(const HingeJointContext* hinge);

// Debug visualization
void hinge_debug_draw(const HingeJointContext* hinge);

#ifdef __cplusplus
}
#endif

#endif // HINGE_JOINT_H

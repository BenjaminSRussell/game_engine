#ifndef SLIDER_JOINT_H
#define SLIDER_JOINT_H

#include "physics/constraint_solver.h"
#include "math/vec3.h"
#include "math/quat.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Slider Joint Context
typedef struct {
    SliderConstraint constraint;
    
    // Runtime data
    Vec3 world_axis_a;
    Vec3 world_axis_b;
    Vec3 perpendicular_axis1;
    Vec3 perpendicular_axis2;
    
    // Jacobian entries for constraint rows
    JacobianEntry jacobian_rows[5]; // 2 linear + 3 angular constraints
    
    // Motor and limit state
    float current_position;
    float linear_velocity;
    float current_angle;
    float angular_velocity;
    float accumulated_impulse[5];
    
    // Warm starting data
    bool warm_started;
    float previous_lambda[5];
} SliderJointContext;

// Slider Joint Functions
SliderJointContext* slider_create(EntityID body_a, EntityID body_b,
                                  const Vec3* axis_a, const Vec3* axis_b);
void slider_destroy(SliderJointContext* slider);

// Constraint solving
void slider_prepare(SliderJointContext* slider, const RigidBody* body_a, const RigidBody* body_b);
void slider_solve_velocity(SliderJointContext* slider, RigidBody* body_a, RigidBody* body_b, float dt);
void slider_solve_position(SliderJointContext* slider, RigidBody* body_a, RigidBody* body_b, float dt);

// Motor control
void slider_set_linear_motor(SliderJointContext* slider, float target_velocity, float max_force);
void slider_set_angular_motor(SliderJointContext* slider, float target_velocity, float max_force);
void slider_enable_linear_motor(SliderJointContext* slider, bool enabled);
void slider_enable_angular_motor(SliderJointContext* slider, bool enabled);

// Limit control
void slider_set_linear_limits(SliderJointContext* slider, float min_position, float max_position);
void slider_set_angular_limits(SliderJointContext* slider, float min_angle, float max_angle);
void slider_disable_linear_limits(SliderJointContext* slider);
void slider_disable_angular_limits(SliderJointContext* slider);
bool slider_is_at_linear_limit(const SliderJointContext* slider);
bool slider_is_at_angular_limit(const SliderJointContext* slider);

// Friction
void slider_set_linear_friction(SliderJointContext* slider, float friction_force);
void slider_set_angular_friction(SliderJointContext* slider, float friction_torque);

// Warm starting
void slider_warm_start(SliderJointContext* slider);
void slider_save_impulse(SliderJointContext* slider);

// Utility functions
float slider_get_linear_position(const SliderJointContext* slider);
float slider_get_angular_position(const SliderJointContext* slider);
float slider_get_linear_velocity(const SliderJointContext* slider);
float slider_get_angular_velocity(const SliderJointContext* slider);
bool slider_is_broken(const SliderJointContext* slider);

// Debug visualization
void slider_debug_draw(const SliderJointContext* slider);

#ifdef __cplusplus
}
#endif

#endif // SLIDER_JOINT_H

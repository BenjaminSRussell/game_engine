#ifndef BALL_SOCKET_H
#define BALL_SOCKET_H

#include "physics/constraint_solver.h"
#include "math/vec3.h"
#include "math/quat.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Ball Socket Joint Context
typedef struct {
    EntityID body_a;
    EntityID body_b;
    Vec3 anchor_a;        // Anchor point in body A's local space
    Vec3 anchor_b;        // Anchor point in body B's local space
    
    // Cone limit parameters
    bool cone_limit_enabled;
    float cone_angle;     // Maximum angle from rest position (radians)
    Quat rest_rotation;  // Rest orientation of body B relative to A
    
    // Twist limit parameters
    bool twist_limit_enabled;
    float twist_min_angle;
    float twist_max_angle;
    
    // Soft limits (spring-damper)
    bool soft_limits_enabled;
    float spring_stiffness;
    float spring_damping;
    
    // Motor parameters
    bool motor_enabled;
    Vec3 motor_target_velocity;
    float motor_max_force;
    
    // Runtime data
    Vec3 world_anchor_a;
    Vec3 world_anchor_b;
    Vec3 world_error;
    
    // Jacobian entries
    JacobianEntry jacobian_rows[3]; // 3 linear constraints
    
    // Limit and motor state
    float current_cone_angle;
    float current_twist_angle;
    Vec3 current_angular_velocity;
    float accumulated_impulse[3];
    
    // Warm starting data
    bool warm_started;
    float previous_lambda[3];
    
    // Breaking
    float break_force;
    bool broken;
} BallSocketJoint;

// Ball Socket Joint Functions
BallSocketJoint* ball_socket_create(EntityID body_a, EntityID body_b,
                                   const Vec3* anchor_a, const Vec3* anchor_b);
void ball_socket_destroy(BallSocketJoint* joint);

// Constraint solving
void ball_socket_prepare(BallSocketJoint* joint, const RigidBody* body_a, const RigidBody* body_b);
void ball_socket_solve_velocity(BallSocketJoint* joint, RigidBody* body_a, RigidBody* body_b, float dt);
void ball_socket_solve_position(BallSocketJoint* joint, RigidBody* body_a, RigidBody* body_b, float dt);

// Cone limits
void ball_socket_set_cone_limit(BallSocketJoint* joint, float cone_angle, const Quat* rest_rotation);
void ball_socket_disable_cone_limit(BallSocketJoint* joint);
bool ball_socket_is_cone_limit_active(const BallSocketJoint* joint);

// Twist limits
void ball_socket_set_twist_limits(BallSocketJoint* joint, float min_angle, float max_angle);
void ball_socket_disable_twist_limit(BallSocketJoint* joint);
bool ball_socket_is_twist_limit_active(const BallSocketJoint* joint);

// Soft limits
void ball_socket_enable_soft_limits(BallSocketJoint* joint, float stiffness, float damping);
void ball_socket_disable_soft_limits(BallSocketJoint* joint);

// Motor control
void ball_socket_set_motor(BallSocketJoint* joint, const Vec3* target_velocity, float max_force);
void ball_socket_enable_motor(BallSocketJoint* joint, bool enabled);

// Breaking
void ball_socket_set_break_force(BallSocketJoint* joint, float break_force);
bool ball_socket_is_broken(const BallSocketJoint* joint);

// Warm starting
void ball_socket_warm_start(BallSocketJoint* joint);
void ball_socket_save_impulse(BallSocketJoint* joint);

// Utility functions
float ball_socket_get_cone_angle(const BallSocketJoint* joint);
float ball_socket_get_twist_angle(const BallSocketJoint* joint);
Vec3 ball_socket_get_angular_velocity(const BallSocketJoint* joint);

// Debug visualization
void ball_socket_debug_draw(const BallSocketJoint* joint);

#ifdef __cplusplus
}
#endif

#endif // BALL_SOCKET_H

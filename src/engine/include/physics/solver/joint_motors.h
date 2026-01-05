#pragma once

#include "../../core/types.h"
#include "../../math/vec3.h"
#include "../../math/quat.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct JointMotor JointMotor;
typedef struct MotorController MotorController;
typedef struct RigidBody RigidBody;

// Motor types
typedef enum {
    MOTOR_TYPE_LINEAR,
    MOTOR_TYPE_ANGULAR,
    MOTOR_TYPE_SERVO,
    MOTOR_TYPE_CONE_TWIST
} MotorType;

// Motor modes
typedef enum {
    MOTOR_MODE_DISABLED,
    MOTOR_MODE_VELOCITY,
    MOTOR_MODE_POSITION,
    MOTOR_MODE_POSITION_SERVO
} MotorMode;

// Angular velocity constraint type
typedef enum {
    CONE_LIMIT_NONE,
    CONE_LIMIT_SIMPLE,
    CONE_LIMIT_TWIST_CONE
} ConeLimitType;

// Motor configuration
typedef struct {
    MotorType type;
    MotorMode mode;
    f32 target_value;
    f32 max_force;
    f32 max_torque;
    f32 stiffness;
    f32 damping;
    bool enable_limits;
    f32 min_limit;
    f32 max_limit;
} MotorConfig;

// Servo controller parameters
typedef struct {
    f32 target_position;
    f32 target_velocity;
    f32 p_gain;           // Proportional gain
    f32 i_gain;           // Integral gain
    f32 d_gain;           // Derivative gain
    f32 integral_error;
    f32 last_error;
} ServoController;

// Cone limit parameters
typedef struct {
    ConeLimitType type;
    f32 cone_angle;       // Half-angle of cone
    f32 twist_angle;      // Twist angle limit
    f32 stiffness;
    f32 damping;
} ConeLimitInfo;

/**
 * JointMotor: Actuators for robotic and mechanical control
 *
 * Properties:
 *   - Linear and angular motors
 *   - Velocity and position control
 *   - Servo control with PID
 *   - Configurable force/torque limits
 *   - Cone constraints for articulation
 *   - Robotic arm test support
 *
 * Use cases:
 *   - Character joint control
 *   - Animated limbs
 *   - Vehicle wheels
 *   - Robotic manipulators
 *   - Animated mechanisms
 */

// ============================================================================
// Motor Creation and Destruction
// ============================================================================

/**
 * Create joint motor
 *
 * Args:
 *   config: Motor configuration
 *   body_a: First body
 *   body_b: Second body (NULL for fixed joint)
 *
 * Returns:
 *   New JointMotor (must be freed with motor_destroy)
 */
JointMotor *motor_create(MotorConfig config, RigidBody *body_a, RigidBody *body_b);

/**
 * Create linear motor
 *
 * Args:
 *   body_a: First body
 *   body_b: Second body
 *   axis: Direction of motion (normalized)
 *   max_force: Maximum force in Newtons
 *
 * Returns:
 *   New JointMotor
 */
JointMotor *motor_create_linear(RigidBody *body_a, RigidBody *body_b, Vec3 axis, f32 max_force);

/**
 * Create angular motor
 *
 * Args:
 *   body_a: First body
 *   body_b: Second body
 *   axis: Rotation axis (normalized)
 *   max_torque: Maximum torque in Newton-meters
 *
 * Returns:
 *   New JointMotor
 */
JointMotor *motor_create_angular(RigidBody *body_a, RigidBody *body_b, Vec3 axis, f32 max_torque);

/**
 * Create servo motor (position control)
 *
 * Args:
 *   body_a: First body
 *   body_b: Second body
 *   axis: Control axis
 *   max_force: Maximum force
 *   stiffness: Servo stiffness (0-1)
 *   damping: Servo damping
 *
 * Returns:
 *   New JointMotor in servo mode
 */
JointMotor *motor_create_servo(RigidBody *body_a, RigidBody *body_b, Vec3 axis, f32 max_force, f32 stiffness, f32 damping);

/**
 * Destroy motor
 *
 * Args:
 *   motor: JointMotor to destroy (NULL-safe)
 */
void motor_destroy(JointMotor *motor);

// ============================================================================
// Motor Configuration
// ============================================================================

/**
 * Set motor mode
 *
 * Args:
 *   motor: JointMotor
 *   mode: New mode (velocity, position, servo, etc.)
 */
void motor_set_mode(JointMotor *motor, MotorMode mode);

/**
 * Get motor mode
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   Current MotorMode
 */
MotorMode motor_get_mode(JointMotor *motor);

/**
 * Enable/disable motor
 *
 * Args:
 *   motor: JointMotor
 *   enable: Whether motor is active
 */
void motor_set_enabled(JointMotor *motor, bool enable);

/**
 * Check if motor is enabled
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   true if enabled
 */
bool motor_is_enabled(JointMotor *motor);

/**
 * Set maximum force (for linear motors)
 *
 * Args:
 *   motor: JointMotor
 *   max_force: Maximum force in Newtons
 */
void motor_set_max_force(JointMotor *motor, f32 max_force);

/**
 * Get maximum force
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   Current max force
 */
f32 motor_get_max_force(JointMotor *motor);

/**
 * Set maximum torque (for angular motors)
 *
 * Args:
 *   motor: JointMotor
 *   max_torque: Maximum torque in Newton-meters
 */
void motor_set_max_torque(JointMotor *motor, f32 max_torque);

/**
 * Get maximum torque
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   Current max torque
 */
f32 motor_get_max_torque(JointMotor *motor);

// ============================================================================
// Velocity Control
// ============================================================================

/**
 * Set target velocity for motor
 *
 * Args:
 *   motor: JointMotor
 *   velocity: Target velocity/angular velocity
 */
void motor_set_target_velocity(JointMotor *motor, f32 velocity);

/**
 * Get target velocity
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   Current target velocity
 */
f32 motor_get_target_velocity(JointMotor *motor);

/**
 * Set velocity gain (stiffness)
 *
 * Args:
 *   motor: JointMotor
 *   stiffness: Stiffness parameter (0-1)
 */
void motor_set_velocity_stiffness(JointMotor *motor, f32 stiffness);

/**
 * Set velocity damping
 *
 * Args:
 *   motor: JointMotor
 *   damping: Damping coefficient
 */
void motor_set_velocity_damping(JointMotor *motor, f32 damping);

// ============================================================================
// Position Control and Servo
// ============================================================================

/**
 * Set target position for servo motor
 *
 * Args:
 *   motor: JointMotor
 *   target_position: Desired position/angle
 */
void motor_set_target_position(JointMotor *motor, f32 target_position);

/**
 * Get target position
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   Current target position
 */
f32 motor_get_target_position(JointMotor *motor);

/**
 * Get current position
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   Current position/angle
 */
f32 motor_get_current_position(JointMotor *motor);

/**
 * Get position error
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   Difference between target and current position
 */
f32 motor_get_position_error(JointMotor *motor);

// ============================================================================
// PID Servo Control
// ============================================================================

/**
 * Set PID gains for servo
 *
 * Args:
 *   motor: JointMotor in servo mode
 *   p_gain: Proportional gain
 *   i_gain: Integral gain
 *   d_gain: Derivative gain
 */
void motor_set_pid_gains(JointMotor *motor, f32 p_gain, f32 i_gain, f32 d_gain);

/**
 * Get PID gains
 *
 * Args:
 *   motor: JointMotor
 *   p_gain: Output for P gain
 *   i_gain: Output for I gain
 *   d_gain: Output for D gain
 */
void motor_get_pid_gains(JointMotor *motor, f32 *p_gain, f32 *i_gain, f32 *d_gain);

/**
 * Update servo controller with new position
 *
 * Args:
 *   motor: JointMotor
 *   current_position: Current position/angle
 *   dt: Time step
 */
void motor_update_servo(JointMotor *motor, f32 current_position, f32 dt);

/**
 * Compute servo output force/torque
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   Output force/torque
 */
f32 motor_compute_servo_output(JointMotor *motor);

/**
 * Reset servo integral error
 *
 * Args:
 *   motor: JointMotor
 */
void motor_reset_servo_integral(JointMotor *motor);

// ============================================================================
// Cone and Twist Limits
// ============================================================================

/**
 * Add cone limit to motor
 *
 * Args:
 *   motor: JointMotor
 *   cone_angle: Half-angle of cone in radians
 *   twist_angle: Maximum twist around axis
 *   stiffness: Limit stiffness
 *   damping: Limit damping
 */
void motor_add_cone_limit(JointMotor *motor, f32 cone_angle, f32 twist_angle, f32 stiffness, f32 damping);

/**
 * Set cone limit type
 *
 * Args:
 *   motor: JointMotor
 *   limit_type: ConeLimitType
 */
void motor_set_cone_limit_type(JointMotor *motor, ConeLimitType limit_type);

/**
 * Get cone limit info
 *
 * Args:
 *   motor: JointMotor
 *   limit: Output ConeLimitInfo
 */
void motor_get_cone_limit(JointMotor *motor, ConeLimitInfo *limit);

/**
 * Check if motor is at cone limit
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   true if orientation at limit boundary
 */
bool motor_at_cone_limit(JointMotor *motor);

/**
 * Get angle from cone axis
 *
 * Args:
 *   motor: JointMotor
 *
 * Returns:
 *   Current angle from cone axis
 */
f32 motor_get_cone_angle(JointMotor *motor);

// ============================================================================
// Motor Solving
// ============================================================================

/**
 * Solve motor constraint (compute impulse)
 *
 * Args:
 *   motor: JointMotor
 *   body_a: First body
 *   body_b: Second body (may be NULL)
 *   dt: Time step
 *
 * Returns:
 *   Impulse magnitude applied
 */
f32 motor_solve(JointMotor *motor, RigidBody *body_a, RigidBody *body_b, f32 dt);

/**
 * Solve linear motor
 *
 * Args:
 *   motor: JointMotor
 *   body_a: First body
 *   body_b: Second body
 *   r_a: Contact point on body A
 *   r_b: Contact point on body B
 *   dt: Time step
 *
 * Returns:
 *   Force applied
 */
f32 motor_solve_linear(JointMotor *motor, RigidBody *body_a, RigidBody *body_b, Vec3 r_a, Vec3 r_b, f32 dt);

/**
 * Solve angular motor
 *
 * Args:
 *   motor: JointMotor
 *   body_a: First body
 *   body_b: Second body
 *   dt: Time step
 *
 * Returns:
 *   Torque applied
 */
f32 motor_solve_angular(JointMotor *motor, RigidBody *body_a, RigidBody *body_b, f32 dt);

/**
 * Apply cone limit constraint if needed
 *
 * Args:
 *   motor: JointMotor
 *   body_a: First body
 *   body_b: Second body
 *   dt: Time step
 *
 * Returns:
 *   true if constraint was applied
 */
bool motor_apply_cone_limit(JointMotor *motor, RigidBody *body_a, RigidBody *body_b, f32 dt);

// ============================================================================
// Motor Controller System
// ============================================================================

/**
 * Create motor controller (manages multiple motors)
 *
 * Args:
 *   capacity: Maximum number of motors
 *
 * Returns:
 *   New MotorController (must be freed with motor_controller_destroy)
 */
MotorController *motor_controller_create(u32 capacity);

/**
 * Destroy motor controller
 *
 * Args:
 *   controller: MotorController to destroy
 */
void motor_controller_destroy(MotorController *controller);

/**
 * Add motor to controller
 *
 * Args:
 *   controller: MotorController
 *   motor: JointMotor to add
 *
 * Returns:
 *   Motor ID in controller
 */
u32 motor_controller_add_motor(MotorController *controller, JointMotor *motor);

/**
 * Remove motor from controller
 *
 * Args:
 *   controller: MotorController
 *   motor_id: ID of motor to remove
 *
 * Returns:
 *   true if removed
 */
bool motor_controller_remove_motor(MotorController *controller, u32 motor_id);

/**
 * Get motor from controller
 *
 * Args:
 *   controller: MotorController
 *   motor_id: Motor ID
 *
 * Returns:
 *   JointMotor pointer or NULL
 */
JointMotor *motor_controller_get_motor(MotorController *controller, u32 motor_id);

/**
 * Update all motors in controller
 *
 * Args:
 *   controller: MotorController
 *   dt: Time step
 */
void motor_controller_update(MotorController *controller, f32 dt);

/**
 * Solve all motor constraints
 *
 * Args:
 *   controller: MotorController
 *   dt: Time step
 */
void motor_controller_solve(MotorController *controller, f32 dt);

/**
 * Get motor count
 *
 * Args:
 *   controller: MotorController
 *
 * Returns:
 *   Number of motors
 */
u32 motor_controller_count(MotorController *controller);

// ============================================================================
// Statistics and Diagnostics
// ============================================================================

/**
 * Get motor statistics
 *
 * Args:
 *   motor: JointMotor
 *   stats_out: Output buffer
 *   stats_size: Buffer size
 *
 * Returns:
 *   Number of bytes written
 */
u32 motor_get_statistics(JointMotor *motor, char *stats_out, u32 stats_size);

/**
 * Print motor information
 *
 * Args:
 *   motor: JointMotor
 */
void motor_print_info(JointMotor *motor);

// ============================================================================
// Testing and Validation
// ============================================================================

/**
 * Run comprehensive tests
 *
 * Returns:
 *   0 if all tests passed
 */
u32 motor_run_tests(void);

/**
 * Test robotic arm simulation
 *
 * Args:
 *   num_segments: Number of arm segments
 *   duration_ms: Test duration
 *
 * Returns:
 *   true if arm simulation works correctly
 */
bool motor_test_robotic_arm(u32 num_segments, u32 duration_ms);

/**
 * Test servo positioning accuracy
 *
 * Args:
 *   num_servos: Number of servos
 *
 * Returns:
 *   true if servos reach target positions
 */
bool motor_test_servo_accuracy(u32 num_servos);

/**
 * Test cone limits
 *
 * Returns:
 *   true if limits are enforced correctly
 */
bool motor_test_cone_limits(void);

#ifdef __cplusplus
}
#endif

#endif // JOINT_MOTORS_H

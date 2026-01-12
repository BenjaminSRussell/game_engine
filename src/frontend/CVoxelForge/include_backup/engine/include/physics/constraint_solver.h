#ifndef CONSTRAINT_SOLVER_H
#define CONSTRAINT_SOLVER_H

#include <ecs/ecs.h>
#include <math/quat.h>
#include <math/vec3.h>
#include <physics/physics_engine_core.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constraint Types
typedef enum {
  CONSTRAINT_POINT = 0,
  CONSTRAINT_HINGE,
  CONSTRAINT_SLIDER,
  CONSTRAINT_TWIST,
  CONSTRAINT_SPRING,
  CONSTRAINT_DISTANCE,
  CONSTRAINT_ANGULAR,
  CONSTRAINT_CONE,
  CONSTRAINT_WELD,
  CONSTRAINT_COUNT
} ConstraintType;

// Constraint Solver Types
typedef enum {
  SOLVER_SEQUENTIAL_IMPULSES = 0,
  SOLVER_GAUSS_SEIDEL,
  SOLVER_JACOBI,
  SOLVER_SOR,
  SOLVER_COUNT
} SolverType;

// Constraint Flags
typedef enum {
  CONSTRAINT_FLAG_NONE = 0,
  CONSTRAINT_FLAG_ENABLED = 1 << 0,
  CONSTRAINT_FLAG_COLLISION_BETWEEN_LINKED = 1 << 1,
  CONSTRAINT_FLAG_DISABLE_COLLISION_BETWEEN_LINKED = 1 << 2,
  CONSTRAINT_FLAG_BREAKABLE = 1 << 3,
  CONSTRAINT_FLAG_MOTOR_ENABLED = 1 << 4
} ConstraintFlags;

// Constraint Limit
typedef struct {
  float min;
  float max;
  bool has_min;
  bool has_max;
} ConstraintLimit;

// Constraint Motor
typedef struct {
  bool enabled;
  float target_velocity;
  float max_force;
  float position_gain;
  float velocity_gain;
} ConstraintMotor;

// Point Constraint (connects two bodies at a specific point)
typedef struct {
  EntityID body_a;
  EntityID body_b;
  Vec3 pivot_a; // Pivot point in body A's local space
  Vec3 pivot_b; // Pivot point in body B's local space
  float break_force;
  float break_torque;
  uint32_t flags;
} PointConstraint;

// Hinge Constraint (allows rotation around a single axis)
typedef struct {
  EntityID body_a;
  EntityID body_b;
  Vec3 pivot_a; // Pivot point in body A's local space
  Vec3 pivot_b; // Pivot point in body B's local space
  Vec3 axis_a;  // Hinge axis in body A's local space
  Vec3 axis_b;  // Hinge axis in body B's local space
  ConstraintLimit angle_limit;
  ConstraintMotor motor;
  float break_force;
  float break_torque;
  uint32_t flags;
} HingeConstraint;

// Slider Constraint (allows translation along a single axis)
typedef struct {
  EntityID body_a;
  EntityID body_b;
  Vec3 axis_a; // Slider axis in body A's local space
  Vec3 axis_b; // Slider axis in body B's local space
  ConstraintLimit linear_limit;
  ConstraintLimit angular_limit;
  ConstraintMotor linear_motor;
  ConstraintMotor angular_motor;
  float break_force;
  float break_torque;
  uint32_t flags;
} SliderConstraint;

// Spring Constraint (elastic connection between bodies)
typedef struct {
  EntityID body_a;
  EntityID body_b;
  Vec3 anchor_a; // Anchor point in body A's local space
  Vec3 anchor_b; // Anchor point in body B's local space
  float rest_length;
  float stiffness;
  float damping;
  float max_force;
  uint32_t flags;
} SpringConstraint;

// Generic Constraint
typedef struct {
  ConstraintType type;
  union {
    PointConstraint point;
    HingeConstraint hinge;
    SliderConstraint slider;
    SpringConstraint spring;
  } data;
  bool enabled;
  float break_threshold;
  bool broken;
} Constraint;

// Constraint Solver Context
typedef struct {
  Constraint *constraints;
  uint32_t constraint_count;
  uint32_t constraint_capacity;

  // Solver configuration
  SolverType solver_type;
  int max_iterations;
  float baumgarte_factor;
  float cfm;
  float erp;
  bool warm_start;

  // Performance statistics
  uint32_t iterations_used;
  float solve_time;
  uint32_t constraints_solved;
  uint32_t constraints_failed;
} ConstraintSolver;

// Jacobian Matrix Entry
typedef struct {
  float effective_mass;
  float jacobian[6]; // [linear_x, linear_y, linear_z, angular_x, angular_y,
                     // angular_z]
  float rhs;
  float lambda;
  float lambda_min;
  float lambda_max;
} JacobianEntry;

// Constraint Solver Functions
bool constraint_solver_init(ConstraintSolver *solver, uint32_t max_constraints);
void constraint_solver_cleanup(ConstraintSolver *solver);
void constraint_solver_update(ConstraintSolver *solver, float time_step);

// Constraint Management
uint32_t constraint_add_point(ConstraintSolver *solver,
                              const PointConstraint *constraint);
uint32_t constraint_add_hinge(ConstraintSolver *solver,
                              const HingeConstraint *constraint);
uint32_t constraint_add_slider(ConstraintSolver *solver,
                               const SliderConstraint *constraint);
uint32_t constraint_add_spring(ConstraintSolver *solver,
                               const SpringConstraint *constraint);
bool constraint_remove(ConstraintSolver *solver, uint32_t constraint_id);
bool constraint_enable(ConstraintSolver *solver, uint32_t constraint_id,
                       bool enabled);
bool constraint_set_breaking_threshold(ConstraintSolver *solver,
                                       uint32_t constraint_id, float force,
                                       float torque);

// Constraint Solving
void constraint_solver_solve(ConstraintSolver *solver, float time_step);
void constraint_solver_sequential_impulses(ConstraintSolver *solver,
                                           float time_step);
void constraint_solver_gauss_seidel(ConstraintSolver *solver, float time_step);
void constraint_solver_jacobi(ConstraintSolver *solver, float time_step);

// Individual Constraint Solvers
void constraint_solve_point(ConstraintSolver *solver, uint32_t constraint_id,
                            float time_step);
void constraint_solve_hinge(ConstraintSolver *solver, uint32_t constraint_id,
                            float time_step);
void constraint_solve_slider(ConstraintSolver *solver, uint32_t constraint_id,
                             float time_step);
void constraint_solve_spring(ConstraintSolver *solver, uint32_t constraint_id,
                             float time_step);

// Jacobian Computation
void constraint_compute_point_jacobian(const PointConstraint *constraint,
                                       JacobianEntry *jacobian);
void constraint_compute_hinge_jacobian(const HingeConstraint *constraint,
                                       JacobianEntry *jacobian);
void constraint_compute_slider_jacobian(const SliderConstraint *constraint,
                                        JacobianEntry *jacobian);
void constraint_compute_spring_jacobian(const SpringConstraint *constraint,
                                        JacobianEntry *jacobian);

// Constraint Validation
bool constraint_validate_point(const PointConstraint *constraint);
bool constraint_validate_hinge(const HingeConstraint *constraint);
bool constraint_validate_slider(const SliderConstraint *constraint);
bool constraint_validate_spring(const SpringConstraint *constraint);

// Constraint Breaking
bool constraint_check_breaking(ConstraintSolver *solver,
                               uint32_t constraint_id);
void constraint_break(ConstraintSolver *solver, uint32_t constraint_id);

// Motor Control
void constraint_set_motor_target(ConstraintSolver *solver,
                                 uint32_t constraint_id, float target_velocity);
void constraint_set_motor_force(ConstraintSolver *solver,
                                uint32_t constraint_id, float max_force);
bool constraint_is_motor_enabled(const ConstraintSolver *solver,
                                 uint32_t constraint_id);

// Limit Control
void constraint_set_limit(ConstraintSolver *solver, uint32_t constraint_id,
                          float min, float max);
void constraint_get_limit(const ConstraintSolver *solver,
                          uint32_t constraint_id, float *min, float *max);

// Utility Functions
Vec3 constraint_get_world_position(Vec3 local_pos, Vec3 body_pos,
                                   Quat body_rot);
Vec3 constraint_get_world_direction(Vec3 local_dir, Quat body_rot);
Quat constraint_get_relative_rotation(Quat rot_a, Quat rot_b);
float constraint_get_angle_between_vectors(Vec3 a, Vec3 b);

// Debug and Visualization
void constraint_debug_draw(const ConstraintSolver *solver);
void constraint_debug_draw_jacobians(const ConstraintSolver *solver);
void constraint_debug_draw_limits(const ConstraintSolver *solver);

// Performance Profiling
typedef struct {
  uint32_t total_constraints;
  uint32_t active_constraints;
  uint32_t broken_constraints;
  float average_solve_time;
  float max_solve_time;
  uint32_t average_iterations;
  uint32_t max_iterations;
} ConstraintPerformanceStats;

void constraint_get_performance_stats(const ConstraintSolver *solver,
                                      ConstraintPerformanceStats *stats);
void constraint_reset_performance_stats(ConstraintSolver *solver);

// Helper Functions
bool constraint_is_enabled(const Constraint *constraint);
bool constraint_is_broken(const Constraint *constraint);
ConstraintType constraint_get_type(const Constraint *constraint);
float constraint_get_error(const Constraint *constraint);

#ifdef __cplusplus
}
#endif

#endif // CONSTRAINT_SOLVER_H

#include <common.h>
#include <core/logger.h>
#include <core/memory.h>
#include <core/utils.h>
#include <include/math/math.h>
#include <math/quat.h>
#include <physics/constraint_solver.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration
static void constraint_solve_point_helper(const PointConstraint *constraint,
                                          float time_step);

// Global constraint solver instance
static ConstraintSolver g_constraint_solver = {0};

bool constraint_solver_init(ConstraintSolver *solver,
                            uint32_t max_constraints) {
  if (!solver || max_constraints == 0)
    return false;

  memset(solver, 0, sizeof(ConstraintSolver));

  // Allocate constraint array
  solver->constraint_capacity = max_constraints;
  solver->constraints = (Constraint *)core_alloc(solver->constraint_capacity *
                                                 sizeof(Constraint));
  if (!solver->constraints)
    return false;
  memset(solver->constraints, 0,
         solver->constraint_capacity * sizeof(Constraint));

  // Set default solver configuration
  solver->solver_type = SOLVER_SEQUENTIAL_IMPULSES;
  solver->max_iterations = 10;
  solver->baumgarte_factor = 0.2f;
  solver->cfm = 0.0f;
  solver->erp = 0.2f;
  solver->warm_start = true;

  LOG_INFO("Constraint solver initialized with capacity for %u constraints",
           max_constraints);
  return true;
}

void constraint_solver_cleanup(ConstraintSolver *solver) {
  if (!solver)
    return;

  if (solver->constraints) {
    core_free(solver->constraints);
    solver->constraints = NULL;
  }

  memset(solver, 0, sizeof(ConstraintSolver));
  LOG_INFO("Constraint solver cleaned up");
}

void constraint_solver_update(ConstraintSolver *solver, float time_step) {
  if (!solver)
    return;

  float start_time = get_time();

  // Remove broken constraints
  for (uint32_t i = 0; i < solver->constraint_count; i++) {
    if (solver->constraints[i].broken) {
      constraint_remove(solver, i);
      i--; // Recheck this index
    }
  }

  // Solve constraints
  constraint_solver_solve(solver, time_step);

  solver->solve_time = get_time() - start_time;
}

uint32_t constraint_add_point(ConstraintSolver *solver,
                              const PointConstraint *constraint) {
  if (!solver || !constraint ||
      solver->constraint_count >= solver->constraint_capacity)
    return UINT32_MAX;

  if (!constraint_validate_point(constraint)) {
    LOG_ERROR("Invalid point constraint");
    return UINT32_MAX;
  }

  Constraint *new_constraint = &solver->constraints[solver->constraint_count];
  new_constraint->type = CONSTRAINT_POINT;
  new_constraint->data.point = *constraint;
  new_constraint->enabled = true;
  new_constraint->broken = false;
  new_constraint->break_threshold = constraint->break_force;

  return solver->constraint_count++;
}

uint32_t constraint_add_hinge(ConstraintSolver *solver,
                              const HingeConstraint *constraint) {
  if (!solver || !constraint ||
      solver->constraint_count >= solver->constraint_capacity)
    return UINT32_MAX;

  if (!constraint_validate_hinge(constraint)) {
    LOG_ERROR("Invalid hinge constraint");
    return UINT32_MAX;
  }

  Constraint *new_constraint = &solver->constraints[solver->constraint_count];
  new_constraint->type = CONSTRAINT_HINGE;
  new_constraint->data.hinge = *constraint;
  new_constraint->enabled = true;
  new_constraint->broken = false;
  new_constraint->break_threshold = constraint->break_force;

  return solver->constraint_count++;
}

uint32_t constraint_add_spring(ConstraintSolver *solver,
                               const SpringConstraint *constraint) {
  if (!solver || !constraint ||
      solver->constraint_count >= solver->constraint_capacity)
    return UINT32_MAX;

  if (!constraint_validate_spring(constraint)) {
    LOG_ERROR("Invalid spring constraint");
    return UINT32_MAX;
  }

  Constraint *new_constraint = &solver->constraints[solver->constraint_count];
  new_constraint->type = CONSTRAINT_SPRING;
  new_constraint->data.spring = *constraint;
  new_constraint->enabled = true;
  new_constraint->broken = false;
  new_constraint->break_threshold = constraint->max_force;

  return solver->constraint_count++;
}

void constraint_solver_solve(ConstraintSolver *solver, float time_step) {
  if (!solver)
    return;

  solver->iterations_used = 0;
  solver->constraints_solved = 0;
  solver->constraints_failed = 0;

  switch (solver->solver_type) {
  case SOLVER_SEQUENTIAL_IMPULSES:
    constraint_solver_sequential_impulses(solver, time_step);
    break;
  case SOLVER_GAUSS_SEIDEL:
    constraint_solver_gauss_seidel(solver, time_step);
    break;
  case SOLVER_JACOBI:
    constraint_solver_jacobi(solver, time_step);
    break;
  default:
    constraint_solver_sequential_impulses(solver, time_step);
    break;
  }
}

void constraint_solver_sequential_impulses(ConstraintSolver *solver,
                                           float time_step) {
  if (!solver)
    return;

  for (int iteration = 0; iteration < solver->max_iterations; iteration++) {
    bool any_constraint_solved = false;

    for (uint32_t i = 0; i < solver->constraint_count; i++) {
      Constraint *constraint = &solver->constraints[i];

      if (!constraint->enabled || constraint->broken)
        continue;

      bool solved = false;
      switch (constraint->type) {
      case CONSTRAINT_POINT:
        constraint_solve_point(solver, i, time_step);
        solved = true;
        break;
      case CONSTRAINT_HINGE:
        constraint_solve_hinge(solver, i, time_step);
        solved = true;
        break;
      case CONSTRAINT_SLIDER:
        constraint_solve_slider(solver, i, time_step);
        solved = true;
        break;
      case CONSTRAINT_SPRING:
        constraint_solve_spring(solver, i, time_step);
        solved = true;
        break;
      default:
        break;
      }

      if (solved) {
        any_constraint_solved = true;
        solver->constraints_solved++;

        // Check if constraint should break
        if (constraint_check_breaking(solver, i)) {
          constraint_break(solver, i);
          solver->constraints_failed++;
        }
      }
    }

    solver->iterations_used++;

    if (!any_constraint_solved) {
      break; // No constraints were solved, exit early
    }
  }
}

void constraint_solve_point(ConstraintSolver *solver, uint32_t constraint_id,
                            float time_step) {
  if (!solver || constraint_id >= solver->constraint_count)
    return;

  Constraint *constraint = &solver->constraints[constraint_id];
  PointConstraint *point = &constraint->data.point;

  // Get body positions and velocities (simplified - would need actual physics
  // bodies)
  Vec3 pos_a = {0}; // Would get from physics world
  Vec3 pos_b = {0};
  Vec3 vel_a = {0};
  Vec3 vel_b = {0};
  float inv_mass_a = 1.0f;
  float inv_mass_b = 1.0f;

  // Transform pivot points to world space
  Vec3 world_pivot_a =
      constraint_get_world_position(point->pivot_a, pos_a, quat_identity());
  Vec3 world_pivot_b =
      constraint_get_world_position(point->pivot_b, pos_b, quat_identity());

  // Calculate constraint error (distance between pivot points)
  Vec3 error = vec3_sub(world_pivot_b, world_pivot_a);
  float error_magnitude = vec3_length(error);

  if (error_magnitude < 0.001f)
    return; // Constraint already satisfied

  Vec3 error_normal = vec3_div(error, error_magnitude);

  // Calculate relative velocity at constraint point
  Vec3 relative_velocity = vec3_sub(vel_b, vel_a);
  float velocity_along_normal = vec3_dot(relative_velocity, error_normal);

  // Calculate impulse magnitude
  float effective_mass = 1.0f / (inv_mass_a + inv_mass_b);
  float restitution = 0.0f; // Point constraints are typically hard constraints
  float j = -(velocity_along_normal +
              solver->baumgarte_factor * error_magnitude / time_step) *
            effective_mass;

  // Apply impulse
  Vec3 impulse = vec3_mul(error_normal, j);

  // In a real implementation, this would apply impulses to the physics bodies
  LOG_DEBUG("Applied point constraint impulse: %.3f", j);
}

void constraint_solve_hinge(ConstraintSolver *solver, uint32_t constraint_id,
                            float time_step) {
  if (!solver || constraint_id >= solver->constraint_count)
    return;

  Constraint *constraint = &solver->constraints[constraint_id];
  HingeConstraint *hinge = &constraint->data.hinge;

  // Hinge constraints solve 3 degrees of freedom:
  // 1. Point constraint (pivot points must coincide)
  // 2. Angular constraint (axes must be aligned)
  // 3. Angular limit (if specified)

  // Solve point constraint part
  PointConstraint point_constraint = {.body_a = hinge->body_a,
                                      .body_b = hinge->body_b,
                                      .pivot_a = hinge->pivot_a,
                                      .pivot_b = hinge->pivot_b,
                                      .break_force = hinge->break_force,
                                      .break_torque = hinge->break_torque,
                                      .flags = hinge->flags};

  constraint_solve_point_helper(&point_constraint, time_step);

  // Solve angular constraint (axes must be aligned)
  Vec3 world_axis_a =
      constraint_get_world_direction(hinge->axis_a, quat_identity());
  Vec3 world_axis_b =
      constraint_get_world_direction(hinge->axis_b, quat_identity());

  Vec3 cross = vec3_cross(world_axis_a, world_axis_b);
  float cross_magnitude = vec3_length(cross);

  if (cross_magnitude > 0.001f) {
    Vec3 rotation_axis = vec3_div(cross, cross_magnitude);
    float rotation_angle = asinf(fminf(cross_magnitude, 1.0f));

    // Apply angular impulse to align axes
    float angular_impulse =
        rotation_angle / time_step * solver->baumgarte_factor;
    LOG_DEBUG("Applied hinge angular impulse: %.3f", angular_impulse);
  }

  // Solve angular limit if specified
  if (hinge->angle_limit.has_min || hinge->angle_limit.has_max) {
    // Calculate current angle between bodies around hinge axis
    float current_angle =
        constraint_get_angle_between_vectors(world_axis_a, world_axis_b);

    bool limit_violated = false;
    if (hinge->angle_limit.has_min && current_angle < hinge->angle_limit.min) {
      limit_violated = true;
    } else if (hinge->angle_limit.has_max &&
               current_angle > hinge->angle_limit.max) {
      limit_violated = true;
    }

    if (limit_violated) {
      // Apply corrective impulse
      float limit_error = 0.0f;
      if (current_angle < hinge->angle_limit.min) {
        limit_error = hinge->angle_limit.min - current_angle;
      } else {
        limit_error = hinge->angle_limit.max - current_angle;
      }

      float limit_impulse = limit_error / time_step * solver->baumgarte_factor;
      LOG_DEBUG("Applied hinge limit impulse: %.3f", limit_impulse);
    }
  }

  // Apply motor if enabled
  if (hinge->motor.enabled) {
    // Apply motor torque to achieve target velocity
    float motor_impulse =
        hinge->motor.target_velocity * hinge->motor.max_force * time_step;
    LOG_DEBUG("Applied hinge motor impulse: %.3f", motor_impulse);
  }
}

void constraint_solve_spring(ConstraintSolver *solver, uint32_t constraint_id,
                             float time_step) {
  if (!solver || constraint_id >= solver->constraint_count)
    return;

  Constraint *constraint = &solver->constraints[constraint_id];
  SpringConstraint *spring = &constraint->data.spring;

  // Get body positions
  Vec3 pos_a = {0}; // Would get from physics world
  Vec3 pos_b = {0};

  // Transform anchor points to world space
  Vec3 world_anchor_a =
      constraint_get_world_position(spring->anchor_a, pos_a, quat_identity());
  Vec3 world_anchor_b =
      constraint_get_world_position(spring->anchor_b, pos_b, quat_identity());

  // Calculate spring vector and length
  Vec3 spring_vector = vec3_sub(world_anchor_b, world_anchor_a);
  float current_length = vec3_length(spring_vector);

  if (current_length < 0.001f)
    return; // Bodies are at same position

  Vec3 spring_direction = vec3_div(spring_vector, current_length);

  // Calculate spring force (Hooke's law)
  float extension = current_length - spring->rest_length;
  float spring_force = spring->stiffness * extension;

  // Calculate damping force
  Vec3 relative_velocity = {0}; // Would get from physics bodies
  float damping_force =
      spring->damping * vec3_dot(relative_velocity, spring_direction);

  // Total force
  float total_force = spring_force + damping_force;

  // Clamp to maximum force
  total_force =
      fmaxf(-spring->max_force, fminf(total_force, spring->max_force));

  // Apply force as impulse
  Vec3 impulse = vec3_mul(spring_direction, total_force * time_step);

  // Check if spring should break
  if (fabsf(total_force) > spring->max_force) {
    constraint_break(solver, constraint_id);
    return;
  }

  // In a real implementation, this would apply impulses to the physics bodies
  LOG_DEBUG("Applied spring impulse: %.3f N", total_force * time_step);
}

bool constraint_validate_point(const PointConstraint *constraint) {
  if (!constraint)
    return false;

  // Check that body IDs are valid (non-zero and different)
  if (constraint->body_a == 0 || constraint->body_b == 0)
    return false;
  if (constraint->body_a == constraint->body_b)
    return false;

  // Check that break values are reasonable
  if (constraint->break_force < 0.0f || constraint->break_torque < 0.0f)
    return false;

  return true;
}

bool constraint_validate_hinge(const HingeConstraint *constraint) {
  if (!constraint)
    return false;

  // Check body IDs
  if (constraint->body_a == 0 || constraint->body_b == 0)
    return false;
  if (constraint->body_a == constraint->body_b)
    return false;

  // Check that axes are normalized
  if (vec3_length(constraint->axis_a) < 0.9f ||
      vec3_length(constraint->axis_a) > 1.1f)
    return false;
  if (vec3_length(constraint->axis_b) < 0.9f ||
      vec3_length(constraint->axis_b) > 1.1f)
    return false;

  // Check angle limits
  if (constraint->angle_limit.has_min && constraint->angle_limit.has_max) {
    if (constraint->angle_limit.min > constraint->angle_limit.max)
      return false;
  }

  return true;
}

bool constraint_validate_spring(const SpringConstraint *constraint) {
  if (!constraint)
    return false;

  // Check body IDs
  if (constraint->body_a == 0 || constraint->body_b == 0)
    return false;
  if (constraint->body_a == constraint->body_b)
    return false;

  // Check physical parameters
  if (constraint->stiffness < 0.0f || constraint->damping < 0.0f)
    return false;
  if (constraint->rest_length < 0.0f || constraint->max_force < 0.0f)
    return false;

  return true;
}

bool constraint_check_breaking(ConstraintSolver *solver,
                               uint32_t constraint_id) {
  if (!solver || constraint_id >= solver->constraint_count)
    return false;

  Constraint *constraint = &solver->constraints[constraint_id];
  if (constraint->broken)
    return false;

  // In a real implementation, this would check the actual forces/torques
  // For now, we'll use a simplified check based on constraint error

  float current_force = 0.0f; // Would get from physics simulation

  if (current_force > constraint->break_threshold) {
    return true;
  }

  return false;
}

void constraint_break(ConstraintSolver *solver, uint32_t constraint_id) {
  if (!solver || constraint_id >= solver->constraint_count)
    return;

  Constraint *constraint = &solver->constraints[constraint_id];
  constraint->broken = true;
  constraint->enabled = false;

  LOG_INFO("Constraint %u broken", constraint_id);
}

bool constraint_remove(ConstraintSolver *solver, uint32_t constraint_id) {
  if (!solver || constraint_id >= solver->constraint_count)
    return false;

  // Shift remaining constraints to fill the gap
  for (uint32_t i = constraint_id; i < solver->constraint_count - 1; i++) {
    solver->constraints[i] = solver->constraints[i + 1];
  }

  solver->constraint_count--;
  return true;
}

// Utility Functions
Vec3 constraint_get_world_position(Vec3 local_pos, Vec3 body_pos,
                                   Quat body_rot) {
  return vec3_add(body_pos, quat_rotate_vec3(body_rot, local_pos));
}

Vec3 constraint_get_world_direction(Vec3 local_dir, Quat body_rot) {
  return quat_rotate_vec3(body_rot, local_dir);
}

Quat constraint_get_relative_rotation(Quat rot_a, Quat rot_b) {
  return quat_mul(quat_inverse(rot_a), rot_b);
}

float constraint_get_angle_between_vectors(Vec3 a, Vec3 b) {
  float dot_product = vec3_dot(a, b);
  float lengths = vec3_length(a) * vec3_length(b);

  if (lengths < 0.001f)
    return 0.0f;

  float cos_angle = dot_product / lengths;
  cos_angle = fmaxf(-1.0f, fminf(cos_angle, 1.0f));

  return acosf(cos_angle);
}

// Helper function for point constraint solving
void constraint_solve_point_helper(const PointConstraint *constraint,
                                   float time_step) {
  if (!constraint)
    return;

  // Simplified point constraint solving
  Vec3 world_pivot_a = constraint->pivot_a; // Would transform to world space
  Vec3 world_pivot_b = constraint->pivot_b;

  Vec3 error = vec3_sub(world_pivot_b, world_pivot_a);
  float error_magnitude = vec3_length(error);

  if (error_magnitude > 0.001f) {
    Vec3 correction = vec3_mul(vec3_normalize(error), error_magnitude * 0.5f);
    // Would apply position correction to bodies
    LOG_DEBUG("Point constraint correction: %.3f", error_magnitude);
  }
}

// Performance Statistics
void constraint_get_performance_stats(const ConstraintSolver *solver,
                                      ConstraintPerformanceStats *stats) {
  if (!solver || !stats)
    return;

  stats->total_constraints = solver->constraint_count;
  stats->active_constraints = 0;
  stats->broken_constraints = 0;

  for (uint32_t i = 0; i < solver->constraint_count; i++) {
    const Constraint *constraint = &solver->constraints[i];
    if (constraint->enabled && !constraint->broken) {
      stats->active_constraints++;
    } else if (constraint->broken) {
      stats->broken_constraints++;
    }
  }

  stats->average_solve_time = solver->solve_time;
  stats->max_solve_time = solver->solve_time; // Would track maximum over time
  stats->average_iterations = solver->iterations_used;
  stats->max_iterations =
      solver->iterations_used; // Would track maximum over time
}

void constraint_reset_performance_stats(ConstraintSolver *solver) {
  if (!solver)
    return;

  solver->iterations_used = 0;
  solver->solve_time = 0.0f;
  solver->constraints_solved = 0;
  solver->constraints_failed = 0;
}

// Global accessor functions
ConstraintSolver *get_constraint_solver(void) { return &g_constraint_solver; }

bool init_constraint_system(uint32_t max_constraints) {
  return constraint_solver_init(&g_constraint_solver, max_constraints);
}

void cleanup_constraint_system(void) {
  constraint_solver_cleanup(&g_constraint_solver);
}

#include "physics/time_step.h"
#include "core/logger.h"
#include "core/memory.h"
#include "math/math.h"
#include <include/math/math.h>
#include <stdlib.h>
#include <string.h>

// Global time step instance
static TimeStep g_time_step = {0};

// Default configuration
static const TimeStepConfig DEFAULT_CONFIG = {
    .fixed_time_step = 1.0f / 60.0f, // 60 FPS
    .max_time_step = 1.0f / 30.0f,   // 30 FPS minimum
    .min_time_step = 1.0f / 120.0f,  // 120 FPS maximum
    .max_sub_steps = 8,
    .min_sub_steps = 1,
    .adaptive_sub_stepping = true,
    .enable_interpolation = true,
    .interpolation_alpha = 0.1f,
    .stability_threshold = 0.01f,
    .velocity_threshold = 100.0f,
    .angular_velocity_threshold = 10.0f,
    .position_threshold = 0.1f,
    .rotation_threshold = 0.1f};

// Sub step state names
static const char *SUB_STEP_STATE_NAMES[] = {
    "Normal", "Adaptive", "Stability Critical", "Performance Critical"};

bool time_step_init(TimeStep *time_step, const TimeStepConfig *config) {
  if (!time_step)
    return false;

  memset(time_step, 0, sizeof(TimeStep));
  time_step->config = config ? *config : DEFAULT_CONFIG;

  // Allocate state arrays for interpolation
  time_step->state_capacity = 1000; // Estimate max physics objects
  time_step->previous_states = (PhysicsState *)core_alloc(
      time_step->state_capacity * sizeof(PhysicsState));
  time_step->current_states = (PhysicsState *)core_alloc(
      time_step->state_capacity * sizeof(PhysicsState));

  if (!time_step->previous_states || !time_step->current_states) {
    time_step_cleanup(time_step);
    return false;
  }

  // Initialize state
  time_step->current_time_step = time_step->config.fixed_time_step;
  time_step->current_sub_steps = time_step->config.min_sub_steps;
  time_step->state = SUB_STEP_STATE_NORMAL;
  time_step->accumulated_time = 0.0f;
  time_step->remaining_time = 0.0f;
  time_step->interpolation_factor = 0.0f;

  // Initialize stability monitoring
  time_step->stability.max_velocity = 0.0f;
  time_step->stability.max_angular_velocity = 0.0f;
  time_step->stability.max_position_change = 0.0f;
  time_step->stability.max_rotation_change = 0.0f;
  time_step->stability.instability_count = 0;
  time_step->stability.stability_frames = 0;
  time_step->stability.is_stable = true;

  // Initialize performance monitoring
  time_step->performance.frame_time = 0.0f;
  time_step->performance.simulation_time = 0.0f;
  time_step->performance.render_time = 0.0f;
  time_step->performance.performance_frames = 0;
  time_step->performance.is_performance_critical = false;

  LOG_INFO("Time step system initialized: fixed step=%.3f, max sub-steps=%u, "
           "adaptive=%s",
           time_step->config.fixed_time_step, time_step->config.max_sub_steps,
           time_step->config.adaptive_sub_stepping ? "enabled" : "disabled");
  return true;
}

void time_step_cleanup(TimeStep *time_step) {
  if (!time_step)
    return;

  if (time_step->previous_states)
    core_free(time_step->previous_states);
  if (time_step->current_states)
    core_free(time_step->current_states);

  memset(time_step, 0, sizeof(TimeStep));
  LOG_INFO("Time step system cleaned up");
}

void time_step_update(TimeStep *time_step, PhysicsWorld *world,
                      float delta_time) {
  if (!time_step || !world)
    return;

  float start_time = get_time();

  // Clamp delta time to prevent spiral of death
  delta_time = time_step_clamp_time_step(time_step, delta_time);

  // Monitor performance
  time_step_monitor_performance(time_step, delta_time);

  // Accumulate time
  time_step->accumulated_time += delta_time;
  time_step->remaining_time = time_step->accumulated_time;

  // Save current states for interpolation
  if (time_step->config.enable_interpolation) {
    time_step_save_states(time_step, world);
  }

  // Calculate required sub-steps
  uint32_t sub_steps = time_step_calculate_sub_steps(time_step, delta_time);
  time_step->current_sub_steps = sub_steps;

  // Execute sub-steps
  time_step_execute_sub_steps(time_step, world, delta_time);

  // Monitor stability
  time_step_monitor_stability(time_step, world);

  // Update statistics
  time_step->stats.total_steps++;
  time_step->stats.sub_steps_used += sub_steps;
  time_step->stats.total_simulation_time += get_time() - start_time;

  if (time_step->stats.total_steps > 0) {
    time_step->stats.average_time_step =
        time_step->stats.total_simulation_time / time_step->stats.total_steps;
  }

  // Calculate interpolation factor
  if (time_step->config.enable_interpolation &&
      time_step->accumulated_time > 0.0f) {
    time_step->interpolation_factor =
        time_step->accumulated_time / time_step->config.fixed_time_step;
    time_step->interpolation_factor =
        fmaxf(0.0f, fminf(time_step->interpolation_factor, 1.0f));
  }

  // Reset accumulated time
  time_step->accumulated_time = 0.0f;
}

uint32_t time_step_calculate_sub_steps(TimeStep *time_step, float delta_time) {
  if (!time_step)
    return 1;

  uint32_t sub_steps;

  if (time_step->config.adaptive_sub_stepping &&
      time_step_should_adapt(time_step)) {
    sub_steps = time_step_calculate_adaptive_sub_steps(time_step, delta_time);
    time_step->state = SUB_STEP_STATE_ADAPTIVE;
    time_step->stats.adaptive_steps++;
  } else {
    sub_steps = (uint32_t)ceilf(delta_time / time_step->config.fixed_time_step);
    time_step->state = SUB_STEP_STATE_NORMAL;
  }

  // Clamp to valid range
  sub_steps = fmaxf(time_step->config.min_sub_steps,
                    fminf(sub_steps, time_step->config.max_sub_steps));

  return sub_steps;
}

void time_step_execute_sub_steps(TimeStep *time_step, PhysicsWorld *world,
                                 float delta_time) {
  if (!time_step || !world)
    return;

  float sub_time_step = delta_time / time_step->current_sub_steps;
  sub_time_step = time_step_clamp_time_step(time_step, sub_time_step);

  for (uint32_t i = 0; i < time_step->current_sub_steps; i++) {
    // Call sub-step start callback
    if (time_step->on_sub_step_start) {
      time_step->on_sub_step_start(i, sub_time_step);
    }

    // Execute physics sub-step
    time_step_integrate_forces(time_step, world, sub_time_step);
    time_step_integrate_velocities(time_step, world, sub_time_step);
    time_step_broad_phase_collision(time_step, world, sub_time_step);
    time_step_narrow_phase_collision(time_step, world, sub_time_step);
    time_step_solve_constraints(time_step, world, sub_time_step);
    time_step_integrate_positions(time_step, world, sub_time_step);
    time_step_collision_response(time_step, world, sub_time_step);

    // Call sub-step end callback
    if (time_step->on_sub_step_end) {
      time_step->on_sub_step_end(i, sub_time_step);
    }

    time_step->remaining_time -= sub_time_step;
  }
}

uint32_t time_step_calculate_adaptive_sub_steps(TimeStep *time_step,
                                                float delta_time) {
  if (!time_step)
    return 1;

  uint32_t base_sub_steps =
      (uint32_t)ceilf(delta_time / time_step->config.fixed_time_step);

  // Adjust based on stability
  if (!time_step->stability.is_stable) {
    base_sub_steps *= 2; // Double sub-steps for stability
    time_step->stats.stability_corrections++;
  }

  // Adjust based on performance
  if (time_step->performance.is_performance_critical) {
    base_sub_steps =
        fmaxf(time_step->config.min_sub_steps,
              base_sub_steps / 2); // Halve sub-steps for performance
    time_step->stats.performance_corrections++;
  }

  // Adjust based on object velocities
  float velocity_factor = 1.0f;
  if (time_step->stability.max_velocity >
      time_step->config.velocity_threshold) {
    velocity_factor = time_step->stability.max_velocity /
                      time_step->config.velocity_threshold;
    base_sub_steps = (uint32_t)(base_sub_steps * velocity_factor);
  }

  return base_sub_steps;
}

bool time_step_should_adapt(TimeStep *time_step) {
  if (!time_step || !time_step->config.adaptive_sub_stepping)
    return false;

  // Adapt if unstable
  if (!time_step->stability.is_stable)
    return true;

  // Adapt if performance critical
  if (time_step->performance.is_performance_critical)
    return true;

  // Adapt if velocities are high
  if (time_step->stability.max_velocity > time_step->config.velocity_threshold)
    return true;
  if (time_step->stability.max_angular_velocity >
      time_step->config.angular_velocity_threshold)
    return true;

  return false;
}

void time_step_monitor_stability(TimeStep *time_step, PhysicsWorld *world) {
  if (!time_step || !world)
    return;

  // Reset stability metrics
  time_step->stability.max_velocity = 0.0f;
  time_step->stability.max_angular_velocity = 0.0f;
  time_step->stability.max_position_change = 0.0f;
  time_step->stability.max_rotation_change = 0.0f;

  // In a real implementation, this would iterate through all physics bodies
  // and calculate stability metrics
  // For now, we'll use placeholder values

  // Example stability check
  float sample_velocity = 10.0f; // Would get from physics bodies
  float sample_angular_velocity = 1.0f;

  time_step->stability.max_velocity = sample_velocity;
  time_step->stability.max_angular_velocity = sample_angular_velocity;

  // Check stability conditions
  bool is_stable = true;
  const char *instability_reason = NULL;

  if (sample_velocity > time_step->config.velocity_threshold) {
    is_stable = false;
    instability_reason = "High linear velocity";
  }

  if (sample_angular_velocity > time_step->config.angular_velocity_threshold) {
    is_stable = false;
    instability_reason = "High angular velocity";
  }

  // Update stability state
  if (is_stable) {
    time_step->stability.stability_frames++;
    time_step->stability.instability_count = 0;

    if (time_step->stability.stability_frames > 60) { // 1 second at 60 FPS
      time_step->stability.is_stable = true;
    }
  } else {
    time_step->stability.stability_frames = 0;
    time_step->stability.instability_count++;

    if (time_step->stability.instability_count >
        10) { // 10 frames of instability
      time_step->stability.is_stable = false;
      time_step->state = SUB_STEP_STATE_STABILITY_CRITICAL;

      if (time_step->on_stability_issue) {
        time_step->on_stability_issue(
            time_step->stability.instability_count / 10.0f, instability_reason);
      }
    }
  }

  // Calculate stability score
  time_step->stats.stability_score =
      time_step_calculate_stability_score(time_step);
}

void time_step_monitor_performance(TimeStep *time_step, float frame_time) {
  if (!time_step)
    return;

  time_step->performance.frame_time = frame_time;

  // Check if performance is critical (frame time > 16.67ms for 60 FPS)
  float target_frame_time = 1.0f / 60.0f;
  bool is_critical = frame_time > target_frame_time * 1.5f; // 50% over budget

  if (is_critical) {
    time_step->performance.performance_frames++;

    if (time_step->performance.performance_frames >
        10) { // 10 frames of poor performance
      time_step->performance.is_performance_critical = true;
      time_step->state = SUB_STEP_STATE_PERFORMANCE_CRITICAL;

      if (time_step->on_performance_issue) {
        time_step->on_performance_issue(frame_time, target_frame_time);
      }
    }
  } else {
    time_step->performance.performance_frames = 0;
    time_step->performance.is_performance_critical = false;
  }
}

void time_step_save_states(TimeStep *time_step, PhysicsWorld *world) {
  if (!time_step || !world || !time_step->config.enable_interpolation)
    return;

  // In a real implementation, this would save the current state of all physics
  // objects For now, we'll use placeholder data

  // Swap previous and current states
  PhysicsState *temp = time_step->previous_states;
  time_step->previous_states = time_step->current_states;
  time_step->current_states = temp;

  // Save current states
  for (uint32_t i = 0;
       i < time_step->state_count && i < time_step->state_capacity; i++) {
    // physics_world_get_object_state(world, i, &time_step->current_states[i]);

    // Placeholder data
    time_step->current_states[i].position = (Vec3){(float)i, 0.0f, 0.0f};
    time_step->current_states[i].rotation = quat_identity();
    time_step->current_states[i].linear_velocity = vec3_zero();
    time_step->current_states[i].angular_velocity = vec3_zero();
    time_step->current_states[i].mass = 1.0f;
    time_step->current_states[i].inv_mass = 1.0f;
  }
}

void time_step_interpolate_states(TimeStep *time_step, float alpha) {
  if (!time_step || !time_step->config.enable_interpolation)
    return;

  alpha = fmaxf(0.0f, fminf(alpha, 1.0f));

  for (uint32_t i = 0;
       i < time_step->state_count && i < time_step->state_capacity; i++) {
    PhysicsState interpolated;
    time_step_interpolate_state(&time_step->previous_states[i],
                                &time_step->current_states[i], alpha,
                                &interpolated);

    // Apply interpolated state to physics world
    // physics_world_set_object_state(world, i, &interpolated);
  }
}

void time_step_interpolate_state(const PhysicsState *previous,
                                 const PhysicsState *current, float alpha,
                                 PhysicsState *result) {
  if (!previous || !current || !result)
    return;

  // Linear interpolation for position
  result->position = vec3_lerp(previous->position, current->position, alpha);

  // Spherical interpolation for rotation
  result->rotation = quat_slerp(previous->rotation, current->rotation, alpha);

  // Linear interpolation for velocities
  result->linear_velocity =
      vec3_lerp(previous->linear_velocity, current->linear_velocity, alpha);
  result->angular_velocity =
      vec3_lerp(previous->angular_velocity, current->angular_velocity, alpha);

  // Mass doesn't change during interpolation
  result->mass = current->mass;
  result->inv_mass = current->inv_mass;
}

// Integration functions (simplified)
void time_step_integrate_forces(TimeStep *time_step, PhysicsWorld *world,
                                float sub_time_step) {
  // In a real implementation, this would apply forces to all physics bodies
  // physics_world_integrate_forces(world, sub_time_step);
}

void time_step_integrate_velocities(TimeStep *time_step, PhysicsWorld *world,
                                    float sub_time_step) {
  // physics_world_integrate_velocities(world, sub_time_step);
}

void time_step_integrate_positions(TimeStep *time_step, PhysicsWorld *world,
                                   float sub_time_step) {
  // physics_world_integrate_positions(world, sub_time_step);
}

void time_step_solve_constraints(TimeStep *time_step, PhysicsWorld *world,
                                 float sub_time_step) {
  // physics_world_solve_constraints(world, sub_time_step);
}

void time_step_broad_phase_collision(TimeStep *time_step, PhysicsWorld *world,
                                     float sub_time_step) {
  // physics_world_broad_phase_collision(world, sub_time_step);
}

void time_step_narrow_phase_collision(TimeStep *time_step, PhysicsWorld *world,
                                      float sub_time_step) {
  // physics_world_narrow_phase_collision(world, sub_time_step);
}

void time_step_collision_response(TimeStep *time_step, PhysicsWorld *world,
                                  float sub_time_step) {
  // physics_world_collision_response(world, sub_time_step);
}

// Utility functions
float time_step_clamp_time_step(TimeStep *time_step, float dt) {
  if (!time_step)
    return 1.0f / 60.0f;

  return fmaxf(time_step->config.min_time_step,
               fminf(dt, time_step->config.max_time_step));
}

float time_step_calculate_stability_score(const TimeStep *time_step) {
  if (!time_step)
    return 0.0f;

  float score = 1.0f;

  // Penalize high velocities
  float velocity_factor = fminf(time_step->stability.max_velocity /
                                    time_step->config.velocity_threshold,
                                2.0f);
  score *= 1.0f / velocity_factor;

  // Penalize instability
  float instability_factor =
      fminf(time_step->stability.instability_count / 60.0f, 1.0f);
  score *= (1.0f - instability_factor);

  return fmaxf(0.0f, fminf(score, 1.0f));
}

const char *time_step_state_get_name(SubStepState state) {
  if (state >= sizeof(SUB_STEP_STATE_NAMES) / sizeof(SUB_STEP_STATE_NAMES[0]))
    return "Unknown";
  return SUB_STEP_STATE_NAMES[state];
}

void time_step_get_statistics(const TimeStep *time_step, uint32_t *total_steps,
                              uint32_t *sub_steps_used,
                              float *average_time_step, float *efficiency,
                              float *stability_score) {
  if (!time_step)
    return;

  if (total_steps)
    *total_steps = time_step->stats.total_steps;
  if (sub_steps_used)
    *sub_steps_used = time_step->stats.sub_steps_used;
  if (average_time_step)
    *average_time_step = time_step->stats.average_time_step;
  if (efficiency)
    *efficiency = time_step->stats.sub_step_efficiency;
  if (stability_score)
    *stability_score = time_step->stats.stability_score;
}

// Global accessor functions
TimeStep *get_time_step(void) { return &g_time_step; }

bool init_time_step(const TimeStepConfig *config) {
  return time_step_init(&g_time_step, config);
}

void cleanup_time_step(void) { time_step_cleanup(&g_time_step); }

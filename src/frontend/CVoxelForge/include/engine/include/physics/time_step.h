#ifndef PHYSICS_TIME_STEP_H
#define PHYSICS_TIME_STEP_H

#include "math/quat.h"
#include "math/vec3.h"
#include "physics/physics.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Time Step Configuration
typedef struct {
  float fixed_time_step;
  float max_time_step;
  float min_time_step;
  uint32_t max_sub_steps;
  uint32_t min_sub_steps;
  bool adaptive_sub_stepping;
  bool enable_interpolation;
  float interpolation_alpha;
  float stability_threshold;
  float velocity_threshold;
  float angular_velocity_threshold;
  float position_threshold;
  float rotation_threshold;
} TimeStepConfig;

// Sub Step State
typedef enum {
  SUB_STEP_STATE_NORMAL = 0,
  SUB_STEP_STATE_ADAPTIVE,
  SUB_STEP_STATE_STABILITY_CRITICAL,
  SUB_STEP_STATE_PERFORMANCE_CRITICAL
} SubStepState;

// Time Step Statistics
typedef struct {
  uint32_t total_steps;
  uint32_t sub_steps_used;
  uint32_t adaptive_steps;
  uint32_t stability_corrections;
  uint32_t performance_corrections;
  float average_time_step;
  float max_time_step;
  float min_time_step;
  float total_simulation_time;
  float sub_step_efficiency;
  float stability_score;
} TimeStepStats;

// Physics State for Interpolation
typedef struct {
  Vec3 position;
  Quat rotation;
  Vec3 linear_velocity;
  Vec3 angular_velocity;
  float mass;
  float inv_mass;
} PhysicsState;

// Time Step Context
typedef struct {
  TimeStepConfig config;

  // Current time step state
  float current_time_step;
  uint32_t current_sub_steps;
  SubStepState state;
  float accumulated_time;
  float remaining_time;

  // Interpolation
  PhysicsState *previous_states;
  PhysicsState *current_states;
  uint32_t state_count;
  uint32_t state_capacity;
  float interpolation_factor;

  // Stability monitoring
  struct {
    float max_velocity;
    float max_angular_velocity;
    float max_position_change;
    float max_rotation_change;
    uint32_t instability_count;
    uint32_t stability_frames;
    bool is_stable;
  } stability;

  // Performance monitoring
  struct {
    float frame_time;
    float simulation_time;
    float render_time;
    uint32_t performance_frames;
    bool is_performance_critical;
  } performance;

  // Statistics
  TimeStepStats stats;

  // Callbacks
  void (*on_sub_step_start)(uint32_t sub_step_index, float sub_time_step);
  void (*on_sub_step_end)(uint32_t sub_step_index, float sub_time_step);
  void (*on_adaptive_step)(uint32_t old_sub_steps, uint32_t new_sub_steps);
  void (*on_stability_issue)(float severity, const char *reason);
  void (*on_performance_issue)(float frame_time, float target_time);
} TimeStep;

// Time Step Functions
bool time_step_init(TimeStep *time_step, const TimeStepConfig *config);
void time_step_cleanup(TimeStep *time_step);
void time_step_reset(TimeStep *time_step);

// Time Step Management
void time_step_update(TimeStep *time_step, PhysicsWorld *world,
                      float delta_time);
uint32_t time_step_calculate_sub_steps(TimeStep *time_step, float delta_time);
void time_step_execute_sub_steps(TimeStep *time_step, PhysicsWorld *world,
                                 float delta_time);
void time_step_interpolate_states(TimeStep *time_step, float alpha);

// Adaptive Sub-stepping
uint32_t time_step_calculate_adaptive_sub_steps(TimeStep *time_step,
                                                float delta_time);
bool time_step_should_adapt(TimeStep *time_step);
void time_step_adapt_sub_steps(TimeStep *time_step, uint32_t new_sub_steps);

// Stability Monitoring
void time_step_monitor_stability(TimeStep *time_step, PhysicsWorld *world);
bool time_step_is_stable(const TimeStep *time_step);
void time_step_correct_stability(TimeStep *time_step);
float time_step_calculate_stability_score(const TimeStep *time_step);

// Performance Monitoring
void time_step_monitor_performance(TimeStep *time_step, float frame_time);
bool time_step_is_performance_critical(const TimeStep *time_step);
void time_step_optimize_performance(TimeStep *time_step);

// State Management
void time_step_save_states(TimeStep *time_step, PhysicsWorld *world);
void time_step_restore_states(TimeStep *time_step, PhysicsWorld *world);
void time_step_interpolate_state(const PhysicsState *previous,
                                 const PhysicsState *current, float alpha,
                                 PhysicsState *result);

// Configuration Functions
void time_step_set_fixed_time_step(TimeStep *time_step, float fixed_step);
void time_step_set_max_sub_steps(TimeStep *time_step, uint32_t max_sub_steps);
void time_step_enable_adaptive_sub_stepping(TimeStep *time_step, bool enable);
void time_step_enable_interpolation(TimeStep *time_step, bool enable);
void time_step_set_stability_thresholds(TimeStep *time_step, float velocity,
                                        float angular_velocity, float position,
                                        float rotation);

// Query Functions
float time_step_get_current_time_step(const TimeStep *time_step);
uint32_t time_step_get_current_sub_steps(const TimeStep *time_step);
SubStepState time_step_get_state(const TimeStep *time_step);
float time_step_get_interpolation_factor(const TimeStep *time_step);
bool time_step_is_interpolating(const TimeStep *time_step);

// Statistics and Performance
void time_step_get_statistics(const TimeStep *time_step, uint32_t *total_steps,
                              uint32_t *sub_steps_used,
                              float *average_time_step, float *efficiency,
                              float *stability_score);
void time_step_get_performance_stats(const TimeStep *time_step,
                                     float *frame_time, float *simulation_time,
                                     float *render_time, bool *is_critical);
void time_step_reset_statistics(TimeStep *time_step);

// Debug and Visualization
void time_step_debug_draw_sub_steps(TimeStep *time_step);
void time_step_debug_draw_stability(TimeStep *time_step);
void time_step_debug_draw_performance(TimeStep *time_step);
void time_step_debug_print_statistics(TimeStep *time_step);

// Utility Functions
float time_step_clamp_time_step(TimeStep *time_step, float dt);
uint32_t time_step_calculate_optimal_sub_steps(float delta_time,
                                               float fixed_time_step,
                                               uint32_t max_sub_steps);
bool time_step_is_valid_time_step(float time_step);
float time_step_smooth_time_step(float current, float target,
                                 float smoothing_factor);

// Validation and Testing
bool time_step_validate(const TimeStep *time_step);
bool time_step_test_stability(TimeStep *time_step, uint32_t iterations);
void time_step_run_performance_test(TimeStep *time_step, PhysicsWorld *world,
                                    uint32_t frames);

// Helper Functions
const char *time_step_state_get_name(SubStepState state);
float time_step_calculate_frame_budget(float target_fps);
bool time_step_should_reduce_quality(const TimeStep *time_step);
uint32_t time_step_estimate_required_sub_steps(const TimeStep *time_step,
                                               const PhysicsWorld *world);

// Integration with Physics Systems
void time_step_integrate_forces(TimeStep *time_step, PhysicsWorld *world,
                                float sub_time_step);
void time_step_integrate_velocities(TimeStep *time_step, PhysicsWorld *world,
                                    float sub_time_step);
void time_step_integrate_positions(TimeStep *time_step, PhysicsWorld *world,
                                   float sub_time_step);
void time_step_solve_constraints(TimeStep *time_step, PhysicsWorld *world,
                                 float sub_time_step);

// Collision Detection Integration
void time_step_broad_phase_collision(TimeStep *time_step, PhysicsWorld *world,
                                     float sub_time_step);
void time_step_narrow_phase_collision(TimeStep *time_step, PhysicsWorld *world,
                                      float sub_time_step);
void time_step_collision_response(TimeStep *time_step, PhysicsWorld *world,
                                  float sub_time_step);

// Callback Management
void time_step_set_sub_step_callbacks(TimeStep *time_step,
                                      void (*start_callback)(uint32_t, float),
                                      void (*end_callback)(uint32_t, float));
void time_step_set_adaptive_callback(TimeStep *time_step,
                                     void (*callback)(uint32_t, uint32_t));
void time_step_set_stability_callback(TimeStep *time_step,
                                      void (*callback)(float, const char *));
void time_step_set_performance_callback(TimeStep *time_step,
                                        void (*callback)(float, float));

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_TIME_STEP_H

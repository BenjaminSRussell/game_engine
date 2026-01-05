#ifndef PHYSICS_SLEEP_SYSTEM_H
#define PHYSICS_SLEEP_SYSTEM_H

#include "math/quat.h"
#include "math/vec3.h"
#include "physics/physics.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Sleep States
typedef enum {
  SLEEP_STATE_ACTIVE = 0,
  SLEEP_STATE_MAY_SLEEP,
  SLEEP_STATE_SLEEPING,
  SLEEP_STATE_AWAKING
} SleepState;

// Sleep Thresholds
typedef struct {
  float linear_velocity_threshold;
  float angular_velocity_threshold;
  float position_threshold;
  float rotation_threshold;
  float energy_threshold;
} SleepThresholds;

// Sleep Timers
typedef struct {
  float sleep_delay;
  float awake_delay;
  float check_interval;
  float max_sleep_time;
} SleepTimers;

// Sleep Body Data
typedef struct {
  SleepState state;
  EntityID entity_id;
  SleepThresholds thresholds;
  SleepTimers timers;

  // Current values
  Vec3 last_position;
  Quat last_rotation;
  Vec3 last_linear_velocity;
  Vec3 last_angular_velocity;

  // Timers
  float sleep_timer;
  float awake_timer;
  float check_timer;
  float total_sleep_time;

  // Energy tracking
  float kinetic_energy;
  float potential_energy;
  float total_energy;

  // Motion tracking
  float linear_motion_accumulator;
  float angular_motion_accumulator;
  uint32_t motion_samples;
  uint32_t max_motion_samples;

  // Configuration
  bool can_sleep;
  bool auto_sleep;
  bool force_awake;
  bool persistent_awake;

  // Statistics
  struct {
    uint32_t sleep_count;
    uint32_t awake_count;
    float total_sleep_time;
    float total_active_time;
    float last_sleep_time;
    float last_awake_time;
  } stats;
} SleepBody;

// Sleep System Context
typedef struct {
  SleepBody *bodies;
  uint32_t body_count;
  uint32_t body_capacity;

  // Global settings
  SleepThresholds default_thresholds;
  SleepTimers default_timers;
  bool enable_sleeping;
  bool allow_auto_wake;
  float global_energy_threshold;

  // Performance tracking
  struct {
    uint32_t active_bodies;
    uint32_t sleeping_bodies;
    uint32_t bodies_put_to_sleep;
    uint32_t bodies_awakened;
    float sleep_check_time;
    float total_simulation_time;
    float sleep_efficiency; // Percentage of time bodies spend sleeping
  } stats;

  // Callbacks
  void (*on_body_sleep)(EntityID entity_id);
  void (*on_body_awake)(EntityID entity_id);
  void (*on_sleep_state_change)(EntityID entity_id, SleepState old_state,
                                SleepState new_state);
} SleepSystem;

// Sleep System Functions
bool sleep_system_init(SleepSystem *sleep_sys, uint32_t max_bodies);
void sleep_system_cleanup(SleepSystem *sleep_sys);
void sleep_system_update(SleepSystem *sleep_sys, PhysicsWorld *world,
                         float time_step);

// Body Management
uint32_t sleep_system_add_body(SleepSystem *sleep_sys, EntityID entity_id,
                               const SleepThresholds *thresholds,
                               const SleepTimers *timers);
bool sleep_system_remove_body(SleepSystem *sleep_sys, EntityID entity_id);
bool sleep_system_remove_body_by_index(SleepSystem *sleep_sys, uint32_t index);
SleepBody *sleep_system_get_body(SleepSystem *sleep_sys, EntityID entity_id);
SleepBody *sleep_system_get_body_by_index(SleepSystem *sleep_sys,
                                          uint32_t index);

// Sleep Control
bool sleep_system_put_to_sleep(SleepSystem *sleep_sys, EntityID entity_id);
bool sleep_system_wake_up(SleepSystem *sleep_sys, EntityID entity_id);
bool sleep_system_force_awake(SleepSystem *sleep_sys, EntityID entity_id,
                              bool force);
bool sleep_system_set_can_sleep(SleepSystem *sleep_sys, EntityID entity_id,
                                bool can_sleep);
bool sleep_system_set_auto_sleep(SleepSystem *sleep_sys, EntityID entity_id,
                                 bool auto_sleep);

// State Queries
SleepState sleep_system_get_state(const SleepSystem *sleep_sys,
                                  EntityID entity_id);
bool sleep_system_is_sleeping(const SleepSystem *sleep_sys, EntityID entity_id);
bool sleep_system_is_active(const SleepSystem *sleep_sys, EntityID entity_id);
bool sleep_system_can_sleep(const SleepSystem *sleep_sys, EntityID entity_id);
float sleep_system_get_sleep_time(const SleepSystem *sleep_sys,
                                  EntityID entity_id);
float sleep_system_get_energy(const SleepSystem *sleep_sys, EntityID entity_id);

// Threshold and Timer Management
void sleep_system_set_thresholds(SleepSystem *sleep_sys, EntityID entity_id,
                                 const SleepThresholds *thresholds);
void sleep_system_set_timers(SleepSystem *sleep_sys, EntityID entity_id,
                             const SleepTimers *timers);
void sleep_system_get_thresholds(const SleepSystem *sleep_sys,
                                 EntityID entity_id,
                                 SleepThresholds *thresholds);
void sleep_system_get_timers(const SleepSystem *sleep_sys, EntityID entity_id,
                             SleepTimers *timers);

// Global Configuration
void sleep_system_set_default_thresholds(SleepSystem *sleep_sys,
                                         const SleepThresholds *thresholds);
void sleep_system_set_default_timers(SleepSystem *sleep_sys,
                                     const SleepTimers *timers);
void sleep_system_enable_global_sleeping(SleepSystem *sleep_sys, bool enable);
void sleep_system_set_global_energy_threshold(SleepSystem *sleep_sys,
                                              float threshold);

// Energy Calculations
float sleep_calculate_kinetic_energy(Vec3 linear_velocity,
                                     Vec3 angular_velocity, float mass,
                                     float inertia);
float sleep_calculate_potential_energy(Vec3 position, Vec3 gravity, float mass);
float sleep_calculate_total_energy(Vec3 position, Vec3 linear_velocity,
                                   Vec3 angular_velocity, Vec3 gravity,
                                   float mass, float inertia);
bool sleep_is_energy_below_threshold(const SleepBody *body, float threshold);

// Motion Detection
bool sleep_detect_motion(const SleepBody *body, Vec3 current_position,
                         Quat current_rotation, Vec3 current_linear_velocity,
                         Vec3 current_angular_velocity);
void sleep_update_motion_tracking(SleepBody *body, Vec3 position, Quat rotation,
                                  Vec3 linear_velocity, Vec3 angular_velocity);
void sleep_reset_motion_tracking(SleepBody *body);

// Sleep State Machine
void sleep_update_state_machine(SleepBody *body, float time_step);
void sleep_transition_to_state(SleepBody *body, SleepState new_state);
bool sleep_should_transition_to_sleep(const SleepBody *body);
bool sleep_should_transition_to_active(const SleepBody *body);

// Collision-based Wake-up
void sleep_on_collision(SleepSystem *sleep_sys, EntityID entity_id,
                        EntityID other_entity_id, float impulse);
void sleep_on_force_applied(SleepSystem *sleep_sys, EntityID entity_id,
                            Vec3 force, Vec3 point);
void sleep_on_position_changed(SleepSystem *sleep_sys, EntityID entity_id,
                               Vec3 new_position);
void sleep_on_rotation_changed(SleepSystem *sleep_sys, EntityID entity_id,
                               Quat new_rotation);

// Group Sleep Management
void sleep_put_group_to_sleep(SleepSystem *sleep_sys, EntityID *entity_ids,
                              uint32_t count);
void sleep_wake_up_group(SleepSystem *sleep_sys, EntityID *entity_ids,
                         uint32_t count);
void sleep_wake_up_nearby_bodies(SleepSystem *sleep_sys, EntityID entity_id,
                                 float radius);

// Debug and Visualization
void sleep_debug_draw_sleep_state(const SleepSystem *sleep_sys,
                                  PhysicsWorld *world);
void sleep_debug_draw_energy_levels(const SleepSystem *sleep_sys,
                                    PhysicsWorld *world);
void sleep_debug_draw_sleep_thresholds(const SleepSystem *sleep_sys,
                                       PhysicsWorld *world);

// Statistics and Performance
void sleep_get_statistics(const SleepSystem *sleep_sys, uint32_t *active_count,
                          uint32_t *sleeping_count, float *efficiency);
void sleep_get_body_statistics(const SleepSystem *sleep_sys, EntityID entity_id,
                               uint32_t *sleep_count, uint32_t *awake_count,
                               float *total_sleep_time);
void sleep_reset_statistics(SleepSystem *sleep_sys);
void sleep_reset_body_statistics(SleepSystem *sleep_sys, EntityID entity_id);

// Utility Functions
const char *sleep_state_get_name(SleepState state);
bool sleep_state_can_transition(SleepState from, SleepState to);
float sleep_get_default_linear_threshold(void);
float sleep_get_default_angular_threshold(void);
float sleep_get_default_sleep_delay(void);
SleepThresholds sleep_create_default_thresholds(void);
SleepTimers sleep_create_default_timers(void);

// Validation and Debugging
bool sleep_validate_body(const SleepBody *body);
bool sleep_validate_system(const SleepSystem *sleep_sys);
void sleep_debug_print_body_state(const SleepSystem *sleep_sys,
                                  EntityID entity_id);
void sleep_debug_print_system_stats(const SleepSystem *sleep_sys);

// Callback Management
void sleep_set_sleep_callback(SleepSystem *sleep_sys,
                              void (*callback)(EntityID entity_id));
void sleep_set_awake_callback(SleepSystem *sleep_sys,
                              void (*callback)(EntityID entity_id));
void sleep_set_state_change_callback(SleepSystem *sleep_sys,
                                     void (*callback)(EntityID entity_id,
                                                      SleepState old_state,
                                                      SleepState new_state));

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_SLEEP_SYSTEM_H

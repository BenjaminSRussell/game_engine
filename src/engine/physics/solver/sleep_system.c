#include "physics/sleep_system.h"
#include "core/logger.h"
#include "core/memory.h"
#include "math/math.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Global sleep system instance
static SleepSystem g_sleep_system = {0};

// Default thresholds and timers
static const SleepThresholds DEFAULT_THRESHOLDS = {
    .linear_velocity_threshold = 0.1f,
    .angular_velocity_threshold = 0.1f,
    .position_threshold = 0.01f,
    .rotation_threshold = 0.01f,
    .energy_threshold = 0.01f};

static const SleepTimers DEFAULT_TIMERS = {.sleep_delay = 0.5f,
                                           .awake_delay = 0.1f,
                                           .check_interval = 0.1f,
                                           .max_sleep_time = 60.0f};

// Sleep state names
static const char *SLEEP_STATE_NAMES[] = {"Active", "May Sleep", "Sleeping",
                                          "Awakening"};

bool sleep_system_init(SleepSystem *sleep_sys, uint32_t max_bodies) {
  if (!sleep_sys || max_bodies == 0)
    return false;

  memset(sleep_sys, 0, sizeof(SleepSystem));

  // Allocate bodies array
  sleep_sys->body_capacity = max_bodies;
  sleep_sys->bodies =
      (SleepBody *)core_alloc(sleep_sys->body_capacity * sizeof(SleepBody));
  if (!sleep_sys->bodies)
    return false;
  memset(sleep_sys->bodies, 0, sleep_sys->body_capacity * sizeof(SleepBody));

  // Set default configuration
  sleep_sys->default_thresholds = DEFAULT_THRESHOLDS;
  sleep_sys->default_timers = DEFAULT_TIMERS;
  sleep_sys->enable_sleeping = true;
  sleep_sys->allow_auto_wake = true;
  sleep_sys->global_energy_threshold = 0.01f;

  LOG_INFO("Sleep system initialized with capacity for %u bodies", max_bodies);
  return true;
}

void sleep_system_cleanup(SleepSystem *sleep_sys) {
  if (!sleep_sys)
    return;

  if (sleep_sys->bodies) {
    core_free(sleep_sys->bodies);
    sleep_sys->bodies = NULL;
  }

  memset(sleep_sys, 0, sizeof(SleepSystem));
  LOG_INFO("Sleep system cleaned up");
}

void sleep_system_update(SleepSystem *sleep_sys, PhysicsWorld *world,
                         float time_step) {
  if (!sleep_sys || !world || !sleep_sys->enable_sleeping)
    return;

  float start_time = get_time();

  sleep_sys->stats.active_bodies = 0;
  sleep_sys->stats.sleeping_bodies = 0;

  for (uint32_t i = 0; i < sleep_sys->body_count; i++) {
    SleepBody *body = &sleep_sys->bodies[i];

    if (!body->can_sleep || body->force_awake) {
      body->state = SLEEP_STATE_ACTIVE;
      sleep_sys->stats.active_bodies++;
      continue;
    }

    // Get physics body data (simplified - would need actual physics world
    // access)
    Vec3 current_position = {0}; // Would get from physics body
    Quat current_rotation = quat_identity();
    Vec3 current_linear_velocity = {0};
    Vec3 current_angular_velocity = {0};
    float mass = 1.0f;
    float inertia = 1.0f;

    // Update energy calculations
    body->kinetic_energy = sleep_calculate_kinetic_energy(
        current_linear_velocity, current_angular_velocity, mass, inertia);
    body->potential_energy = sleep_calculate_potential_energy(
        current_position, (Vec3){0, -9.81f, 0}, mass);
    body->total_energy = body->kinetic_energy + body->potential_energy;

    // Update motion tracking
    sleep_update_motion_tracking(body, current_position, current_rotation,
                                 current_linear_velocity,
                                 current_angular_velocity);

    // Update sleep state machine
    sleep_update_state_machine(body, time_step);

    // Update statistics
    if (body->state == SLEEP_STATE_ACTIVE) {
      sleep_sys->stats.active_bodies++;
      body->stats.total_active_time += time_step;
    } else if (body->state == SLEEP_STATE_SLEEPING) {
      sleep_sys->stats.sleeping_bodies++;
      body->total_sleep_time += time_step;
      body->stats.total_sleep_time += time_step;
    }

    // Apply sleep state to physics body
    if (body->state == SLEEP_STATE_SLEEPING) {
      // physics_body_set_sleeping(body->entity_id, true);
    } else {
      // physics_body_set_sleeping(body->entity_id, false);
    }
  }

  sleep_sys->stats.sleep_check_time = get_time() - start_time;

  // Calculate sleep efficiency
  if (sleep_sys->body_count > 0) {
    sleep_sys->stats.sleep_efficiency =
        (float)sleep_sys->stats.sleeping_bodies / sleep_sys->body_count *
        100.0f;
  }
}

uint32_t sleep_system_add_body(SleepSystem *sleep_sys, EntityID entity_id,
                               const SleepThresholds *thresholds,
                               const SleepTimers *timers) {
  if (!sleep_sys || entity_id == 0 ||
      sleep_sys->body_count >= sleep_sys->body_capacity)
    return UINT32_MAX;

  SleepBody *body = &sleep_sys->bodies[sleep_sys->body_count];
  memset(body, 0, sizeof(SleepBody));

  body->entity_id = entity_id;
  body->state = SLEEP_STATE_ACTIVE;
  body->thresholds = thresholds ? *thresholds : sleep_sys->default_thresholds;
  body->timers = timers ? *timers : sleep_sys->default_timers;

  body->can_sleep = true;
  body->auto_sleep = true;
  body->force_awake = false;
  body->persistent_awake = false;

  body->max_motion_samples = 10;
  body->check_timer = body->timers.check_interval;

  // Initialize last values to current values
  // body->last_position = physics_body_get_position(entity_id);
  // body->last_rotation = physics_body_get_rotation(entity_id);
  // body->last_linear_velocity = physics_body_get_linear_velocity(entity_id);
  // body->last_angular_velocity = physics_body_get_angular_velocity(entity_id);

  return sleep_sys->body_count++;
}

bool sleep_system_put_to_sleep(SleepSystem *sleep_sys, EntityID entity_id) {
  if (!sleep_sys || entity_id == 0)
    return false;

  SleepBody *body = sleep_system_get_body(sleep_sys, entity_id);
  if (!body || !body->can_sleep)
    return false;

  SleepState old_state = body->state;
  sleep_transition_to_state(body, SLEEP_STATE_SLEEPING);

  // Trigger callbacks
  if (old_state != SLEEP_STATE_SLEEPING && sleep_sys->on_body_sleep) {
    sleep_sys->on_body_sleep(entity_id);
  }

  if (sleep_sys->on_sleep_state_change) {
    sleep_sys->on_sleep_state_change(entity_id, old_state,
                                     SLEEP_STATE_SLEEPING);
  }

  sleep_sys->stats.bodies_put_to_sleep++;
  LOG_DEBUG("Put entity %d to sleep", entity_id);
  return true;
}

bool sleep_system_wake_up(SleepSystem *sleep_sys, EntityID entity_id) {
  if (!sleep_sys || entity_id == 0)
    return false;

  SleepBody *body = sleep_system_get_body(sleep_sys, entity_id);
  if (!body)
    return false;

  SleepState old_state = body->state;
  sleep_transition_to_state(body, SLEEP_STATE_ACTIVE);

  // Reset sleep timers
  body->sleep_timer = 0.0f;
  body->total_sleep_time = 0.0f;

  // Reset motion tracking
  sleep_reset_motion_tracking(body);

  // Trigger callbacks
  if (old_state == SLEEP_STATE_SLEEPING && sleep_sys->on_body_awake) {
    sleep_sys->on_body_awake(entity_id);
  }

  if (sleep_sys->on_sleep_state_change) {
    sleep_sys->on_sleep_state_change(entity_id, old_state, SLEEP_STATE_ACTIVE);
  }

  sleep_sys->stats.bodies_awakened++;
  LOG_DEBUG("Woke up entity %d", entity_id);
  return true;
}

void sleep_update_state_machine(SleepBody *body, float time_step) {
  if (!body)
    return;

  body->check_timer -= time_step;

  if (body->check_timer <= 0.0f) {
    body->check_timer = body->timers.check_interval;

    switch (body->state) {
    case SLEEP_STATE_ACTIVE:
      if (body->auto_sleep && sleep_should_transition_to_sleep(body)) {
        sleep_transition_to_state(body, SLEEP_STATE_MAY_SLEEP);
      }
      break;

    case SLEEP_STATE_MAY_SLEEP:
      if (sleep_should_transition_to_sleep(body)) {
        sleep_transition_to_state(body, SLEEP_STATE_SLEEPING);
      } else if (sleep_should_transition_to_active(body)) {
        sleep_transition_to_state(body, SLEEP_STATE_ACTIVE);
      }
      break;

    case SLEEP_STATE_SLEEPING:
      if (sleep_should_transition_to_active(body)) {
        sleep_transition_to_state(body, SLEEP_STATE_AWAKING);
      }
      // Check max sleep time
      if (body->total_sleep_time > body->timers.max_sleep_time) {
        sleep_transition_to_state(body, SLEEP_STATE_AWAKING);
      }
      break;

    case SLEEP_STATE_AWAKING:
      body->awake_timer -= time_step;
      if (body->awake_timer <= 0.0f) {
        sleep_transition_to_state(body, SLEEP_STATE_ACTIVE);
      }
      break;
    }
  }
}

void sleep_transition_to_state(SleepBody *body, SleepState new_state) {
  if (!body || body->state == new_state)
    return;

  SleepState old_state = body->state;
  body->state = new_state;

  // Reset timers for state transitions
  switch (new_state) {
  case SLEEP_STATE_MAY_SLEEP:
    body->sleep_timer = 0.0f;
    break;
  case SLEEP_STATE_SLEEPING:
    body->sleep_timer = 0.0f;
    body->stats.last_sleep_time = get_time();
    body->stats.sleep_count++;
    break;
  case SLEEP_STATE_AWAKING:
    body->awake_timer = body->timers.awake_delay;
    body->stats.last_awake_time = get_time();
    body->stats.awake_count++;
    break;
  case SLEEP_STATE_ACTIVE:
    body->awake_timer = 0.0f;
    break;
  }

  LOG_DEBUG("Entity %d transitioned from %s to %s", body->entity_id,
            sleep_state_get_name(old_state), sleep_state_get_name(new_state));
}

bool sleep_should_transition_to_sleep(const SleepBody *body) {
  if (!body || !body->can_sleep || body->force_awake)
    return false;

  // Check energy threshold
  if (body->total_energy > body->thresholds.energy_threshold)
    return false;

  // Check velocity thresholds
  if (vec3_length_sq(body->last_linear_velocity) >
      body->thresholds.linear_velocity_threshold *
          body->thresholds.linear_velocity_threshold) {
    return false;
  }

  if (vec3_length_sq(body->last_angular_velocity) >
      body->thresholds.angular_velocity_threshold *
          body->thresholds.angular_velocity_threshold) {
    return false;
  }

  // Check motion accumulation
  if (body->linear_motion_accumulator > body->thresholds.position_threshold)
    return false;
  if (body->angular_motion_accumulator > body->thresholds.rotation_threshold)
    return false;

  // Check sleep delay
  if (body->sleep_timer < body->timers.sleep_delay)
    return false;

  return true;
}

bool sleep_should_transition_to_active(const SleepBody *body) {
  if (!body)
    return false;

  // Check if body was forced awake
  if (body->force_awake)
    return true;

  // Check energy threshold
  if (body->total_energy > body->thresholds.energy_threshold * 2.0f)
    return true;

  // Check velocity thresholds
  if (vec3_length_sq(body->last_linear_velocity) >
      body->thresholds.linear_velocity_threshold * 4.0f)
    return true;
  if (vec3_length_sq(body->last_angular_velocity) >
      body->thresholds.angular_velocity_threshold * 4.0f)
    return true;

  return false;
}

void sleep_update_motion_tracking(SleepBody *body, Vec3 position, Quat rotation,
                                  Vec3 linear_velocity, Vec3 angular_velocity) {
  if (!body)
    return;

  // Update last values
  body->last_position = position;
  body->last_rotation = rotation;
  body->last_linear_velocity = linear_velocity;
  body->last_angular_velocity = angular_velocity;

  // Accumulate motion
  if (body->motion_samples == 0) {
    body->linear_motion_accumulator = 0.0f;
    body->angular_motion_accumulator = 0.0f;
  }

  // Add current motion to accumulators
  body->linear_motion_accumulator +=
      vec3_length(linear_velocity) * 0.1f; // Check interval
  body->angular_motion_accumulator += vec3_length(angular_velocity) * 0.1f;

  body->motion_samples++;

  // Reset accumulators if we've reached max samples
  if (body->motion_samples >= body->max_motion_samples) {
    body->motion_samples = 0;
  }
}

void sleep_reset_motion_tracking(SleepBody *body) {
  if (!body)
    return;

  body->linear_motion_accumulator = 0.0f;
  body->angular_motion_accumulator = 0.0f;
  body->motion_samples = 0;
}

float sleep_calculate_kinetic_energy(Vec3 linear_velocity,
                                     Vec3 angular_velocity, float mass,
                                     float inertia) {
  float linear_energy = 0.5f * mass * vec3_length_sq(linear_velocity);
  float angular_energy = 0.5f * inertia * vec3_length_sq(angular_velocity);
  return linear_energy + angular_energy;
}

float sleep_calculate_potential_energy(Vec3 position, Vec3 gravity,
                                       float mass) {
  return -mass * vec3_dot(gravity, position);
}

float sleep_calculate_total_energy(Vec3 position, Vec3 linear_velocity,
                                   Vec3 angular_velocity, Vec3 gravity,
                                   float mass, float inertia) {
  float kinetic = sleep_calculate_kinetic_energy(
      linear_velocity, angular_velocity, mass, inertia);
  float potential = sleep_calculate_potential_energy(position, gravity, mass);
  return kinetic + potential;
}

void sleep_on_collision(SleepSystem *sleep_sys, EntityID entity_id,
                        EntityID other_entity_id, float impulse) {
  if (!sleep_sys || entity_id == 0)
    return;

  SleepBody *body = sleep_system_get_body(sleep_sys, entity_id);
  if (!body || body->state != SLEEP_STATE_SLEEPING)
    return;

  // Wake up if impulse is significant
  float impulse_threshold = 1.0f;
  if (impulse > impulse_threshold) {
    sleep_system_wake_up(sleep_sys, entity_id);
    LOG_DEBUG("Entity %d woke up due to collision (impulse: %.3f)", entity_id,
              impulse);
  }
}

void sleep_on_force_applied(SleepSystem *sleep_sys, EntityID entity_id,
                            Vec3 force, Vec3 point) {
  if (!sleep_sys || entity_id == 0)
    return;

  SleepBody *body = sleep_system_get_body(sleep_sys, entity_id);
  if (!body || body->state != SLEEP_STATE_SLEEPING)
    return;

  // Wake up if force is significant
  float force_threshold = 5.0f;
  float force_magnitude = vec3_length(force);

  if (force_magnitude > force_threshold) {
    sleep_system_wake_up(sleep_sys, entity_id);
    LOG_DEBUG("Entity %d woke up due to applied force (magnitude: %.3f)",
              entity_id, force_magnitude);
  }
}

SleepBody *sleep_system_get_body(SleepSystem *sleep_sys, EntityID entity_id) {
  if (!sleep_sys || entity_id == 0)
    return NULL;

  for (uint32_t i = 0; i < sleep_sys->body_count; i++) {
    if (sleep_sys->bodies[i].entity_id == entity_id) {
      return &sleep_sys->bodies[i];
    }
  }

  return NULL;
}

SleepState sleep_system_get_state(const SleepSystem *sleep_sys,
                                  EntityID entity_id) {
  const SleepBody *body =
      sleep_system_get_body((SleepSystem *)sleep_sys, entity_id);
  return body ? body->state : SLEEP_STATE_ACTIVE;
}

bool sleep_system_is_sleeping(const SleepSystem *sleep_sys,
                              EntityID entity_id) {
  return sleep_system_get_state(sleep_sys, entity_id) == SLEEP_STATE_SLEEPING;
}

bool sleep_system_is_active(const SleepSystem *sleep_sys, EntityID entity_id) {
  return sleep_system_get_state(sleep_sys, entity_id) == SLEEP_STATE_ACTIVE;
}

void sleep_wake_up_nearby_bodies(SleepSystem *sleep_sys, EntityID entity_id,
                                 float radius) {
  if (!sleep_sys || entity_id == 0 || radius <= 0.0f)
    return;

  SleepBody *source_body = sleep_system_get_body(sleep_sys, entity_id);
  if (!source_body)
    return;

  Vec3 source_position = source_body->last_position;

  for (uint32_t i = 0; i < sleep_sys->body_count; i++) {
    SleepBody *body = &sleep_sys->bodies[i];

    if (body->entity_id == entity_id || body->state != SLEEP_STATE_SLEEPING)
      continue;

    float distance = vec3_distance(source_position, body->last_position);
    if (distance <= radius) {
      sleep_system_wake_up(sleep_sys, body->entity_id);
      LOG_DEBUG("Woke up nearby entity %d (distance: %.3f)", body->entity_id,
                distance);
    }
  }
}

void sleep_get_statistics(const SleepSystem *sleep_sys, uint32_t *active_count,
                          uint32_t *sleeping_count, float *efficiency) {
  if (!sleep_sys)
    return;

  if (active_count)
    *active_count = sleep_sys->stats.active_bodies;
  if (sleeping_count)
    *sleeping_count = sleep_sys->stats.sleeping_bodies;
  if (efficiency)
    *efficiency = sleep_sys->stats.sleep_efficiency;
}

void sleep_reset_statistics(SleepSystem *sleep_sys) {
  if (!sleep_sys)
    return;

  memset(&sleep_sys->stats, 0, sizeof(sleep_sys->stats));

  // Reset body statistics
  for (uint32_t i = 0; i < sleep_sys->body_count; i++) {
    memset(&sleep_sys->bodies[i].stats, 0, sizeof(sleep_sys->bodies[i].stats));
  }
}

// Utility functions
const char *sleep_state_get_name(SleepState state) {
  if (state >= sizeof(SLEEP_STATE_NAMES) / sizeof(SLEEP_STATE_NAMES[0]))
    return "Unknown";
  return SLEEP_STATE_NAMES[state];
}

SleepThresholds sleep_create_default_thresholds(void) {
  return DEFAULT_THRESHOLDS;
}

SleepTimers sleep_create_default_timers(void) { return DEFAULT_TIMERS; }

// Debug functions
void sleep_debug_print_system_stats(const SleepSystem *sleep_sys) {
  if (!sleep_sys)
    return;

  LOG_INFO("=== Sleep System Statistics ===");
  LOG_INFO("Total Bodies: %u", sleep_sys->body_count);
  LOG_INFO("Active Bodies: %u", sleep_sys->stats.active_bodies);
  LOG_INFO("Sleeping Bodies: %u", sleep_sys->stats.sleeping_bodies);
  LOG_INFO("Sleep Efficiency: %.1f%%", sleep_sys->stats.sleep_efficiency);
  LOG_INFO("Bodies Put to Sleep: %u", sleep_sys->stats.bodies_put_to_sleep);
  LOG_INFO("Bodies Awakened: %u", sleep_sys->stats.bodies_awakened);
  LOG_INFO("Sleep Check Time: %.3f ms",
           sleep_sys->stats.sleep_check_time * 1000.0f);
}

// Global accessor functions
SleepSystem *get_sleep_system(void) { return &g_sleep_system; }

bool init_sleep_system(uint32_t max_bodies) {
  return sleep_system_init(&g_sleep_system, max_bodies);
}

void cleanup_sleep_system(void) { sleep_system_cleanup(&g_sleep_system); }

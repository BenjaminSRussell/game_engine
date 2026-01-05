#include "physics/performance_profiler.h"
#include "core/logger.h"
#include "core/memory.h"
#include "math/math.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Global physics profiler instance
static PhysicsProfiler g_physics_profiler = {0};

// Category names
static const char *CATEGORY_NAMES[PROFILER_CATEGORY_COUNT] = {
    "Physics",
    "Collision Detection",
    "Broad Phase",
    "Narrow Phase",
    "Constraint Solving",
    "Integration",
    "Sleep System",
    "Island Solving",
    "Soft Body",
    "Continuous Collision",
    "Spatial Hash",
    "Contact Cache",
    "Time Step",
    "Debug Visualization"};

bool physics_profiler_init(PhysicsProfiler *profiler, uint32_t max_samples,
                           uint32_t max_alerts) {
  if (!profiler || max_samples == 0 || max_alerts == 0)
    return false;

  memset(profiler, 0, sizeof(PhysicsProfiler));

  // Initialize metrics
  for (int i = 0; i < PROFILER_CATEGORY_COUNT; i++) {
    profiler->metrics[i].min_time = FLT_MAX;
    profiler->metrics[i].enabled = true;
  }

  // Allocate samples
  profiler->sample_capacity = max_samples;
  profiler->samples = (PerformanceSample *)core_alloc(
      profiler->sample_capacity * sizeof(PerformanceSample));
  if (!profiler->samples)
    return false;
  memset(profiler->samples, 0,
         profiler->sample_capacity * sizeof(PerformanceSample));

  // Allocate alerts
  profiler->alert_capacity = max_alerts;
  profiler->alerts = (PerformanceAlert *)core_alloc(profiler->alert_capacity *
                                                    sizeof(PerformanceAlert));
  if (!profiler->alerts) {
    core_free(profiler->samples);
    return false;
  }
  memset(profiler->alerts, 0,
         profiler->alert_capacity * sizeof(PerformanceAlert));

  // Set default configuration
  profiler->enabled = true;
  profiler->real_time_mode = false;
  profiler->sampling_interval = 0.1f; // 10 Hz sampling
  profiler->last_sample_time = 0.0f;
  profiler->frame_counter = 0;

  // Set default thresholds
  for (int i = 0; i < PROFILER_CATEGORY_COUNT; i++) {
    profiler->thresholds[i].warning_threshold =
        16.67f * 0.5f; // 50% of frame budget
    profiler->thresholds[i].critical_threshold =
        16.67f * 0.8f; // 80% of frame budget
    profiler->thresholds[i].max_samples = 1000;
    profiler->thresholds[i].enable_alerts = true;
  }

  LOG_INFO("Physics profiler initialized: %u samples, %u alerts", max_samples,
           max_alerts);
  return true;
}

void physics_profiler_cleanup(PhysicsProfiler *profiler) {
  if (!profiler)
    return;

  if (profiler->samples)
    core_free(profiler->samples);
  if (profiler->alerts)
    core_free(profiler->alerts);

  memset(profiler, 0, sizeof(PhysicsProfiler));
  LOG_INFO("Physics profiler cleaned up");
}

void physics_profiler_begin_sample(PhysicsProfiler *profiler,
                                   ProfilerCategory category) {
  if (!profiler || !profiler->enabled || category >= PROFILER_CATEGORY_COUNT)
    return;

  PerformanceMetric *metric = &profiler->metrics[category];
  if (!metric->enabled)
    return;

  // In a real implementation, this would record the start time
  // metric->start_time = get_high_precision_time();
}

void physics_profiler_end_sample(PhysicsProfiler *profiler,
                                 ProfilerCategory category) {
  if (!profiler || !profiler->enabled || category >= PROFILER_CATEGORY_COUNT)
    return;

  PerformanceMetric *metric = &profiler->metrics[category];
  if (!metric->enabled)
    return;

  // Calculate duration
  float duration = 0.001f; // Placeholder - would calculate actual duration

  // Update metric statistics
  metric->total_time += duration;
  metric->last_time = duration;
  metric->call_count++;

  if (duration < metric->min_time)
    metric->min_time = duration;
  if (duration > metric->max_time)
    metric->max_time = duration;

  if (metric->call_count > 0) {
    metric->average_time = metric->total_time / metric->call_count;
  }

  // Add sample
  physics_profiler_add_sample(profiler, category, duration, 1, 0, 0, 0);

  // Check thresholds
  physics_profiler_check_thresholds(profiler, category, duration);

  // Call sample callback
  if (profiler->on_performance_sample) {
    profiler->on_performance_sample(category, duration);
  }
}

void physics_profiler_add_sample(PhysicsProfiler *profiler,
                                 ProfilerCategory category, float duration,
                                 uint32_t call_count, uint32_t object_count,
                                 uint32_t collision_count, float memory_usage) {
  if (!profiler || category >= PROFILER_CATEGORY_COUNT)
    return;

  if (profiler->sample_count >= profiler->sample_capacity) {
    // Remove oldest sample (circular buffer)
    profiler->current_sample_index =
        (profiler->current_sample_index + 1) % profiler->sample_capacity;
    profiler->sample_count = profiler->sample_capacity;
  } else {
    profiler->sample_count++;
  }

  uint32_t index =
      (profiler->current_sample_index + profiler->sample_count - 1) %
      profiler->sample_capacity;
  PerformanceSample *sample = &profiler->samples[index];

  sample->timestamp = get_time();
  sample->duration = duration;
  sample->call_count = call_count;
  sample->object_count = object_count;
  sample->collision_count = collision_count;
  sample->memory_usage = memory_usage;
}

void physics_profiler_record_frame_metrics(PhysicsProfiler *profiler,
                                           float frame_time,
                                           uint32_t object_count,
                                           uint32_t collision_count,
                                           float memory_usage) {
  if (!profiler)
    return;

  profiler->global_stats.frame_time = frame_time;
  profiler->global_stats.total_objects = object_count;
  profiler->global_stats.total_collisions = collision_count;
  profiler->global_stats.memory_usage = memory_usage;
  profiler->global_stats.frames_analyzed++;

  // Calculate physics frame ratio
  float total_physics_time = 0.0f;
  for (int i = 0; i < PROFILER_CATEGORY_COUNT; i++) {
    total_physics_time += profiler->metrics[i].last_time;
  }
  profiler->global_stats.total_physics_time = total_physics_time;

  if (frame_time > 0.0f) {
    profiler->global_stats.physics_frame_ratio =
        total_physics_time / frame_time;
  }

  profiler->frame_counter++;
}

void physics_profiler_analyze_performance(PhysicsProfiler *profiler) {
  if (!profiler)
    return;

  // Detect bottlenecks
  physics_profiler_detect_bottlenecks(profiler);

  // Generate optimization suggestions
  physics_profiler_generate_optimization_suggestions(profiler);
}

void physics_profiler_detect_bottlenecks(PhysicsProfiler *profiler) {
  if (!profiler)
    return;

  float total_time = 0.0f;
  ProfilerCategory bottleneck_category = PROFILER_CATEGORY_PHYSICS;
  float max_percentage = 0.0f;

  // Calculate total time and find bottleneck
  for (int i = 0; i < PROFILER_CATEGORY_COUNT; i++) {
    PerformanceMetric *metric = &profiler->metrics[i];
    if (!metric->enabled)
      continue;

    total_time += metric->last_time;

    float percentage =
        (total_time > 0.0f) ? (metric->last_time / total_time) * 100.0f : 0.0f;
    if (percentage > max_percentage) {
      max_percentage = percentage;
      bottleneck_category = (ProfilerCategory)i;
    }
  }

  // Check if we have a significant bottleneck
  if (max_percentage > 50.0f) { // More than 50% of time in one category
    profiler->bottleneck.consecutive_frames++;

    if (profiler->bottleneck.consecutive_frames > 10) { // 10 consecutive frames
      profiler->bottleneck.bottleneck_detected = true;
      profiler->bottleneck.bottleneck_category = bottleneck_category;
      profiler->bottleneck.bottleneck_percentage = max_percentage;

      // Trigger bottleneck callback
      if (profiler->on_bottleneck_detected) {
        profiler->on_bottleneck_detected(bottleneck_category, max_percentage);
      }

      // Add alert
      char message[256];
      snprintf(message, sizeof(message),
               "Bottleneck detected: %s using %.1f%% of physics time",
               physics_profiler_category_get_name(bottleneck_category),
               max_percentage);
      physics_profiler_add_alert(profiler, bottleneck_category, 1,
                                 max_percentage, 50.0f, message);
    }
  } else {
    profiler->bottleneck.consecutive_frames = 0;
    profiler->bottleneck.bottleneck_detected = false;
  }
}

bool physics_profiler_check_thresholds(PhysicsProfiler *profiler,
                                       ProfilerCategory category, float value) {
  if (!profiler || category >= PROFILER_CATEGORY_COUNT)
    return false;

  PerformanceThreshold *threshold = &profiler->thresholds[category];
  if (!threshold->enable_alerts)
    return false;

  bool alert_triggered = false;

  if (value >= threshold->critical_threshold) {
    // Critical alert
    char message[256];
    snprintf(message, sizeof(message),
             "Critical performance: %s took %.3fms (threshold: %.3fms)",
             physics_profiler_category_get_name(category), value,
             threshold->critical_threshold);
    physics_profiler_add_alert(profiler, category, 1, value,
                               threshold->critical_threshold, message);
    alert_triggered = true;
  } else if (value >= threshold->warning_threshold) {
    // Warning alert
    char message[256];
    snprintf(message, sizeof(message),
             "Performance warning: %s took %.3fms (threshold: %.3fms)",
             physics_profiler_category_get_name(category), value,
             threshold->warning_threshold);
    physics_profiler_add_alert(profiler, category, 0, value,
                               threshold->warning_threshold, message);
    alert_triggered = true;
  }

  return alert_triggered;
}

void physics_profiler_add_alert(PhysicsProfiler *profiler,
                                ProfilerCategory category, uint32_t alert_type,
                                float current_value, float threshold_value,
                                const char *message) {
  if (!profiler || category >= PROFILER_CATEGORY_COUNT || !message)
    return;

  if (profiler->alert_count >= profiler->alert_capacity) {
    // Remove oldest alert
    for (uint32_t i = 0; i < profiler->alert_count - 1; i++) {
      profiler->alerts[i] = profiler->alerts[i + 1];
    }
    profiler->alert_count--;
  }

  PerformanceAlert *alert = &profiler->alerts[profiler->alert_count++];
  alert->category = category;
  alert->current_value = current_value;
  alert->threshold_value = threshold_value;
  alert->alert_type = alert_type;
  alert->timestamp = (uint64_t)(get_time() * 1000.0); // Convert to milliseconds
  strncpy(alert->message, message, sizeof(alert->message) - 1);
  alert->message[sizeof(alert->message) - 1] = '\0';

  // Trigger alert callback
  if (profiler->on_performance_alert) {
    profiler->on_performance_alert(alert);
  }
}

float physics_profiler_get_category_time(const PhysicsProfiler *profiler,
                                         ProfilerCategory category) {
  if (!profiler || category >= PROFILER_CATEGORY_COUNT)
    return 0.0f;
  return profiler->metrics[category].last_time;
}

float physics_profiler_get_category_percentage(const PhysicsProfiler *profiler,
                                               ProfilerCategory category) {
  if (!profiler || category >= PROFILER_CATEGORY_COUNT)
    return 0.0f;

  float total_time = 0.0f;
  for (int i = 0; i < PROFILER_CATEGORY_COUNT; i++) {
    total_time += profiler->metrics[i].last_time;
  }

  return (total_time > 0.0f)
             ? (profiler->metrics[category].last_time / total_time) * 100.0f
             : 0.0f;
}

void physics_profiler_get_statistics(const PhysicsProfiler *profiler,
                                     uint32_t *total_objects,
                                     uint32_t *active_objects,
                                     uint32_t *total_collisions,
                                     float *physics_time, float *frame_ratio) {
  if (!profiler)
    return;

  if (total_objects)
    *total_objects = profiler->global_stats.total_objects;
  if (active_objects)
    *active_objects = profiler->global_stats.active_objects;
  if (total_collisions)
    *total_collisions = profiler->global_stats.total_collisions;
  if (physics_time)
    *physics_time = profiler->global_stats.total_physics_time;
  if (frame_ratio)
    *frame_ratio = profiler->global_stats.physics_frame_ratio;
}

void physics_profiler_get_category_statistics(
    const PhysicsProfiler *profiler, ProfilerCategory category,
    float *total_time, float *average_time, float *min_time, float *max_time) {
  if (!profiler || category >= PROFILER_CATEGORY_COUNT)
    return;

  const PerformanceMetric *metric = &profiler->metrics[category];

  if (total_time)
    *total_time = metric->total_time;
  if (average_time)
    *average_time = metric->average_time;
  if (min_time)
    *min_time = metric->min_time;
  if (max_time)
    *max_time = metric->max_time;
}

void physics_profiler_track_allocation(PhysicsProfiler *profiler, size_t size) {
  if (!profiler)
    return;

  profiler->memory_stats.total_allocated += size;
  profiler->memory_stats.allocation_count++;

  if (profiler->memory_stats.total_allocated >
      profiler->memory_stats.peak_usage) {
    profiler->memory_stats.peak_usage = profiler->memory_stats.total_allocated;
  }
}

void physics_profiler_track_deallocation(PhysicsProfiler *profiler,
                                         size_t size) {
  if (!profiler)
    return;

  profiler->memory_stats.total_allocated -= size;
  profiler->memory_stats.deallocation_count++;
}

void physics_profiler_get_memory_statistics(const PhysicsProfiler *profiler,
                                            size_t *total_allocated,
                                            size_t *peak_usage,
                                            uint32_t *allocation_count) {
  if (!profiler)
    return;

  if (total_allocated)
    *total_allocated = profiler->memory_stats.total_allocated;
  if (peak_usage)
    *peak_usage = profiler->memory_stats.peak_usage;
  if (allocation_count)
    *allocation_count = profiler->memory_stats.allocation_count;
}

void physics_profiler_generate_optimization_suggestions(
    PhysicsProfiler *profiler) {
  if (!profiler)
    return;

  // This would analyze performance data and generate optimization suggestions
  // For now, we'll provide placeholder suggestions

  // Example: If broad phase is taking too long, suggest spatial optimization
  float broad_phase_time = physics_profiler_get_category_time(
      profiler, PROFILER_CATEGORY_BROAD_PHASE);
  float broad_phase_percentage = physics_profiler_get_category_percentage(
      profiler, PROFILER_CATEGORY_BROAD_PHASE);

  if (broad_phase_percentage > 30.0f) {
    // Suggest spatial optimization
    LOG_INFO("Optimization suggestion: Consider implementing spatial "
             "partitioning for broad phase collision detection");
  }

  // Example: If constraint solving is slow, suggest reducing iterations
  float constraint_time = physics_profiler_get_category_time(
      profiler, PROFILER_CATEGORY_CONSTRAINT_SOLVING);
  float constraint_percentage = physics_profiler_get_category_percentage(
      profiler, PROFILER_CATEGORY_CONSTRAINT_SOLVING);

  if (constraint_percentage > 25.0f) {
    LOG_INFO("Optimization suggestion: Consider reducing constraint solver "
             "iterations or implementing island solving");
  }
}

const char *physics_profiler_category_get_name(ProfilerCategory category) {
  if (category >= PROFILER_CATEGORY_COUNT)
    return "Unknown";
  return CATEGORY_NAMES[category];
}

float physics_profiler_calculate_fps(float frame_time) {
  return (frame_time > 0.0f) ? (1.0f / frame_time) : 0.0f;
}

bool physics_profiler_is_performance_acceptable(
    const PhysicsProfiler *profiler) {
  if (!profiler)
    return false;

  // Check if physics time is within acceptable limits
  float physics_ratio = profiler->global_stats.physics_frame_ratio;
  return physics_ratio <
         0.5f; // Physics should take less than 50% of frame time
}

// RAII-style profiling helper
ProfileScope physics_profiler_create_scope(PhysicsProfiler *profiler,
                                           ProfilerCategory category) {
  ProfileScope scope = {0};
  scope.profiler = profiler;
  scope.category = category;
  scope.active = true;

  if (profiler && category < PROFILER_CATEGORY_COUNT) {
    physics_profiler_begin_sample(profiler, category);
  }

  return scope;
}

void physics_profiler_destroy_scope(ProfileScope *scope) {
  if (!scope || !scope->active)
    return;

  if (scope->profiler && scope->category < PROFILER_CATEGORY_COUNT) {
    physics_profiler_end_sample(scope->profiler, scope->category);
  }

  scope->active = false;
}

// Global accessor functions
PhysicsProfiler *get_physics_profiler(void) { return &g_physics_profiler; }

bool init_physics_profiler(uint32_t max_samples, uint32_t max_alerts) {
  return physics_profiler_init(&g_physics_profiler, max_samples, max_alerts);
}

void cleanup_physics_profiler(void) {
  physics_profiler_cleanup(&g_physics_profiler);
}

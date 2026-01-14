// Server Performance Tracker implementation
#include "network/server_performance_tracker.h"
#include "include/core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Platform-specific includes
#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <psapi.h>
#include <windows.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

// MARK: - Platform-specific implementations

bool platform_get_cpu_usage(float *usage_percent, u32 *cores_available,
                            u32 *cores_active) {
  if (!usage_percent || !cores_available || !cores_active) {
    return false;
  }

  *usage_percent = 0.0f;
  *cores_available = 0;
  *cores_active = 0;

#ifdef __APPLE__
  // Get CPU usage on macOS
  natural_t num_cpus = 0;
  size_t len = sizeof(num_cpus);
  if (sysctlbyname("hw.ncpu", &num_cpus, &len, NULL, 0) == 0) {
    *cores_available = (u32)num_cpus;
    *cores_active = (u32)num_cpus;
  }

  // Get CPU usage percentage
  processor_info_array_t cpu_info;
  mach_msg_type_number_t msg_count = 0;
  natural_t processor_count = 0;
  if (host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO,
                          &processor_count, &cpu_info,
                          &msg_count) == KERN_SUCCESS) {
    vm_deallocate(mach_task_self(), (vm_address_t)cpu_info,
                  msg_count * sizeof(integer_t));
    *usage_percent = 50.0f; // Mock for now
  }

#elif defined(_WIN32)
  // Get CPU usage on Windows
  SYSTEM_INFO sys_info;
  GetSystemInfo(&sys_info);
  *cores_available = sys_info.dwNumberOfProcessors;
  *cores_active = sys_info.dwNumberOfProcessors;
  *usage_percent = 0.0f; // Placeholder

#else
  LOG_WARN("CPU usage monitoring not implemented for this platform");
  return false;
#endif

  return true;
}

bool platform_get_memory_usage(u64 *total_mb, u64 *used_mb, u64 *available_mb) {
  if (!total_mb || !used_mb || !available_mb) {
    return false;
  }

  *total_mb = 0;
  *used_mb = 0;
  *available_mb = 0;

#ifdef __APPLE__
  // Get memory usage on macOS
  vm_statistics64_data_t vm_stats;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                        (host_info64_t)&vm_stats, &count) == KERN_SUCCESS) {
    // Estimate total from used + free
    u64 free_count = (u64)vm_stats.free_count;
    u64 active_count = (u64)vm_stats.active_count;
    u64 inactive_count = (u64)vm_stats.inactive_count;
    u64 wire_count = (u64)vm_stats.wire_count;
    u64 page_size = 16384;

    vm_size_t v_page_size;
    host_page_size(mach_host_self(), &v_page_size);
    page_size = (u64)v_page_size;

    u64 total_pages = free_count + active_count + inactive_count + wire_count;
    *total_mb = (total_pages * page_size) / (1024 * 1024);
    *used_mb = ((active_count + wire_count) * page_size) / (1024 * 1024);
    *available_mb = *total_mb - *used_mb;
  }

#else
  LOG_WARN("Memory usage monitoring not implemented for this platform");
  return false;
#endif

  return true;
}

bool platform_get_network_stats(u32 *packets_sent, u32 *packets_received,
                                u32 *bytes_sent, u32 *bytes_received,
                                float *latency_ms) {
  if (!packets_sent || !packets_received || !bytes_sent || !bytes_received ||
      !latency_ms) {
    return false;
  }
  // Mock implementation
  *packets_sent = 0;
  *packets_received = 0;
  *bytes_sent = 0;
  *bytes_received = 0;
  *latency_ms = 0.0f;
  return true;
}

bool platform_get_process_time(float *user_time_ms, float *system_time_ms) {
  if (!user_time_ms || !system_time_ms) {
    return false;
  }
  *user_time_ms = 0.0f;
  *system_time_ms = 0.0f;
  return true;
}

// MARK: - Helper Functions

#include "include/core/utils.h"

static void create_alert(ServerPerformanceTracker *tracker, AlertLevel level,
                         const char *message, const char *metric_name,
                         float current_value, float threshold_value) {
  if (tracker->alert_count >= tracker->alert_capacity) {
    return; // Alert buffer full
  }

  PerformanceAlert *alert = &tracker->alerts[tracker->alert_count++];
  alert->level = level;
  strncpy(alert->message, message, sizeof(alert->message) - 1);
  alert->message[sizeof(alert->message) - 1] = '\0';
  alert->timestamp_ms = get_current_time_ms();
  alert->metric_name = metric_name;
  alert->current_value = current_value;
  alert->threshold_value = threshold_value;

  // Trigger callback
  if (level == ALERT_WARNING && tracker->on_warning) {
    tracker->on_warning(alert);
  } else if (level == ALERT_CRITICAL && tracker->on_critical) {
    tracker->on_critical(alert);
  }
}

static void check_threshold(ServerPerformanceTracker *tracker,
                            const char *metric_name, float current_value,
                            float warning_threshold, float critical_threshold) {
  AlertLevel level = ALERT_NONE;
  const char *message = NULL;

  if (current_value >= critical_threshold) {
    level = ALERT_CRITICAL;
    message = "Critical performance threshold exceeded";
  } else if (current_value >= warning_threshold) {
    level = ALERT_WARNING;
    message = "Performance threshold exceeded";
  }

  if (level != ALERT_NONE) {
    create_alert(tracker, level, message, metric_name, current_value,
                 level == ALERT_CRITICAL ? critical_threshold
                                         : warning_threshold);
  }
}

// MARK: - Server Performance Tracker Management

bool server_perf_tracker_init(ServerPerformanceTracker *tracker,
                              u32 history_capacity, u32 alert_capacity) {
  if (!tracker || history_capacity == 0 || alert_capacity == 0) {
    LOG_ERROR("Invalid parameters for server_perf_tracker_init");
    return false;
  }

  memset(tracker, 0, sizeof(ServerPerformanceTracker));

  // Allocate history buffer
  tracker->history.metrics = (ServerPerformanceMetrics *)calloc(
      history_capacity, sizeof(ServerPerformanceMetrics));
  if (!tracker->history.metrics) {
    LOG_ERROR("Failed to allocate performance history");
    return false;
  }
  tracker->history.capacity = history_capacity;
  tracker->history.is_circular = true;

  // Allocate alerts buffer
  tracker->alerts =
      (PerformanceAlert *)calloc(alert_capacity, sizeof(PerformanceAlert));
  if (!tracker->alerts) {
    free(tracker->history.metrics);
    LOG_ERROR("Failed to allocate alerts buffer");
    return false;
  }
  tracker->alert_capacity = alert_capacity;

  // Set default thresholds
  tracker->thresholds = server_perf_get_default_thresholds();

  // Set default update interval
  tracker->update_interval_ms = 1000; // 1 second

  LOG_INFO("Server performance tracker initialized (history: %u, alerts: %u)",
           history_capacity, alert_capacity);
  return true;
}

void server_perf_tracker_shutdown(ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return;
  }

  if (tracker->history.metrics) {
    free(tracker->history.metrics);
    tracker->history.metrics = NULL;
  }

  if (tracker->alerts) {
    free(tracker->alerts);
    tracker->alerts = NULL;
  }

  memset(tracker, 0, sizeof(ServerPerformanceTracker));
  LOG_INFO("Server performance tracker shutdown");
}

bool server_perf_tracker_start(ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return false;
  }

  tracker->is_tracking = true;
  tracker->last_update_time_ms = get_current_time_ms();

  LOG_INFO("Server performance tracking started");
  return true;
}

void server_perf_tracker_stop(ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return;
  }

  tracker->is_tracking = false;
  LOG_INFO("Server performance tracking stopped");
}

void server_perf_tracker_update(ServerPerformanceTracker *tracker) {
  if (!tracker || !tracker->is_tracking) {
    return;
  }

  u64 current_time = get_current_time_ms();
  if (current_time - tracker->last_update_time_ms <
      tracker->update_interval_ms) {
    return; // Not time to update yet
  }

  // Collect all metrics
  server_perf_collect_cpu_metrics(tracker);
  server_perf_collect_memory_metrics(tracker);
  server_perf_collect_network_metrics(tracker);
  server_perf_collect_server_metrics(tracker);

  // Set timestamp
  tracker->current_metrics.timestamp_ms = current_time;

  // Calculate server load percentage
  tracker->current_metrics.server_load_percentage =
      (tracker->current_metrics.cpu_usage_percent * 0.4f +
       tracker->current_metrics.memory_usage_percent * 0.3f +
       (tracker->current_metrics.tick_time_ms / 16.67f) *
           0.3f); // 16.67ms = 60 FPS

  // Check performance health
  tracker->current_metrics.is_performance_healthy =
      tracker->current_metrics.cpu_usage_percent < 80.0f &&
      tracker->current_metrics.memory_usage_percent < 85.0f &&
      tracker->current_metrics.tick_time_ms < 33.34f && // 30 FPS minimum
      tracker->current_metrics.network_latency_ms < 100.0f;

  // Add to history
  if (tracker->history.capacity > 0) {
    u32 index = tracker->history.current_index;
    tracker->history.metrics[index] = tracker->current_metrics;
    tracker->history.current_index = (index + 1) % tracker->history.capacity;
    if (tracker->history.count < tracker->history.capacity) {
      tracker->history.count++;
    }
  }

  // Check thresholds and generate alerts
  check_threshold(tracker, "CPU Usage",
                  tracker->current_metrics.cpu_usage_percent,
                  tracker->thresholds.cpu_warning_threshold,
                  tracker->thresholds.cpu_critical_threshold);
  check_threshold(tracker, "Memory Usage",
                  tracker->current_metrics.memory_usage_percent,
                  tracker->thresholds.memory_warning_threshold,
                  tracker->thresholds.memory_critical_threshold);
  check_threshold(tracker, "Network Latency",
                  tracker->current_metrics.network_latency_ms,
                  tracker->thresholds.latency_warning_threshold,
                  tracker->thresholds.latency_critical_threshold);
  check_threshold(tracker, "Tick Time", tracker->current_metrics.tick_time_ms,
                  tracker->thresholds.tick_time_warning_threshold,
                  tracker->thresholds.tick_time_critical_threshold);

  tracker->last_update_time_ms = current_time;
}

// MARK: - Metrics Collection

void server_perf_collect_cpu_metrics(ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return;
  }

  float cpu_usage = 0.0f;
  u32 cores_available = 0, cores_active = 0;

  if (platform_get_cpu_usage(&cpu_usage, &cores_available, &cores_active)) {
    tracker->current_metrics.cpu_usage_percent = cpu_usage;
    tracker->current_metrics.cpu_cores_available = cores_available;
    tracker->current_metrics.cpu_cores_active = cores_active;
  }
}

void server_perf_collect_memory_metrics(ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return;
  }

  u64 total_mb = 0, used_mb = 0, available_mb = 0;

  if (platform_get_memory_usage(&total_mb, &used_mb, &available_mb)) {
    tracker->current_metrics.total_memory_mb = total_mb;
    tracker->current_metrics.used_memory_mb = used_mb;
    tracker->current_metrics.available_memory_mb = available_mb;

    if (total_mb > 0) {
      tracker->current_metrics.memory_usage_percent =
          (float)used_mb / (float)total_mb * 100.0f;
    }
  }
}

void server_perf_collect_network_metrics(ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return;
  }

  u32 packets_sent = 0, packets_received = 0, bytes_sent = 0,
      bytes_received = 0;
  float latency = 0.0f;

  if (platform_get_network_stats(&packets_sent, &packets_received, &bytes_sent,
                                 &bytes_received, &latency)) {
    tracker->current_metrics.packets_sent_per_second = packets_sent;
    tracker->current_metrics.packets_received_per_second = packets_received;
    tracker->current_metrics.bytes_sent_per_second = bytes_sent;
    tracker->current_metrics.bytes_received_per_second = bytes_received;
    tracker->current_metrics.network_latency_ms = latency;
  }
}

void server_perf_collect_server_metrics(ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return;
  }

  // These would be collected from the actual server implementation
  // For now, we'll set some reasonable defaults
  tracker->current_metrics.tick_rate = 60.0f;                 // 60 FPS target
  tracker->current_metrics.tick_time_ms = 16.67f;             // 60 FPS
  tracker->current_metrics.entities_processed_per_tick = 100; // Example value
  tracker->current_metrics.network_events_processed_per_tick =
      50;                                           // Example value
  tracker->current_metrics.active_connections = 10; // Example value
  tracker->current_metrics.max_connections = 100;   // Example value
}

// MARK: - Performance Analysis

bool server_perf_is_healthy(const ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return false;
  }

  return tracker->current_metrics.is_performance_healthy;
}

AlertLevel
server_perf_get_worst_alert_level(const ServerPerformanceTracker *tracker) {
  if (!tracker || tracker->alert_count == 0) {
    return ALERT_NONE;
  }

  AlertLevel worst = ALERT_NONE;
  for (u32 i = 0; i < tracker->alert_count; i++) {
    if (tracker->alerts[i].level > worst) {
      worst = tracker->alerts[i].level;
    }
  }

  return worst;
}

u32 server_perf_get_active_alert_count(
    const ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return 0;
  }

  return tracker->alert_count;
}

// MARK: - History Management

const ServerPerformanceMetrics *
server_perf_get_current_metrics(const ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return NULL;
  }

  return &tracker->current_metrics;
}

const ServerPerformanceMetrics *
server_perf_get_metrics_at_time(const ServerPerformanceTracker *tracker,
                                u32 index) {
  if (!tracker || !tracker->history.metrics || tracker->history.count == 0) {
    return NULL;
  }

  if (index >= tracker->history.count) {
    return NULL;
  }

  u32 actual_index =
      (tracker->history.current_index - index - 1 + tracker->history.count) %
      tracker->history.count;
  return &tracker->history.metrics[actual_index];
}

const ServerPerformanceMetrics *
server_perf_get_average_metrics(const ServerPerformanceTracker *tracker,
                                u32 sample_count) {
  if (!tracker || !tracker->history.metrics || tracker->history.count == 0 ||
      sample_count == 0) {
    return NULL;
  }

  u32 samples_to_average = sample_count > tracker->history.count
                               ? tracker->history.count
                               : sample_count;

  ServerPerformanceMetrics *avg =
      (ServerPerformanceMetrics *)calloc(1, sizeof(ServerPerformanceMetrics));
  if (!avg) {
    return NULL;
  }

  for (u32 i = 0; i < samples_to_average; i++) {
    const ServerPerformanceMetrics *metrics =
        server_perf_get_metrics_at_time(tracker, i);
    if (metrics) {
      avg->cpu_usage_percent += metrics->cpu_usage_percent;
      avg->memory_usage_percent += metrics->memory_usage_percent;
      avg->network_latency_ms += metrics->network_latency_ms;
      avg->tick_time_ms += metrics->tick_time_ms;
    }
  }

  avg->cpu_usage_percent /= samples_to_average;
  avg->memory_usage_percent /= samples_to_average;
  avg->network_latency_ms /= samples_to_average;
  avg->tick_time_ms /= samples_to_average;

  return avg;
}

float server_perf_get_average_cpu_usage(const ServerPerformanceTracker *tracker,
                                        u32 sample_count) {
  const ServerPerformanceMetrics *avg =
      server_perf_get_average_metrics(tracker, sample_count);
  if (!avg) {
    return 0.0f;
  }

  float result = avg->cpu_usage_percent;
  free((void *)avg);
  return result;
}

float server_perf_get_average_memory_usage(
    const ServerPerformanceTracker *tracker, u32 sample_count) {
  const ServerPerformanceMetrics *avg =
      server_perf_get_average_metrics(tracker, sample_count);
  if (!avg) {
    return 0.0f;
  }

  float result = avg->memory_usage_percent;
  free((void *)avg);
  return result;
}

float server_perf_get_average_latency(const ServerPerformanceTracker *tracker,
                                      u32 sample_count) {
  const ServerPerformanceMetrics *avg =
      server_perf_get_average_metrics(tracker, sample_count);
  if (!avg) {
    return 0.0f;
  }

  float result = avg->network_latency_ms;
  free((void *)avg);
  return result;
}

// MARK: - Alert Management

void server_perf_set_thresholds(ServerPerformanceTracker *tracker,
                                const PerformanceThresholds *thresholds) {
  if (!tracker || !thresholds) {
    return;
  }

  tracker->thresholds = *thresholds;
  LOG_INFO("Updated performance thresholds");
}

PerformanceThresholds server_perf_get_default_thresholds(void) {
  PerformanceThresholds thresholds = {0};

  thresholds.cpu_warning_threshold = 70.0f;
  thresholds.cpu_critical_threshold = 85.0f;
  thresholds.memory_warning_threshold = 75.0f;
  thresholds.memory_critical_threshold = 90.0f;
  thresholds.latency_warning_threshold = 50.0f;
  thresholds.latency_critical_threshold = 100.0f;
  thresholds.tick_time_warning_threshold = 25.0f;   // 40 FPS
  thresholds.tick_time_critical_threshold = 33.34f; // 30 FPS

  return thresholds;
}

const PerformanceAlert *
server_perf_get_alerts(const ServerPerformanceTracker *tracker, u32 *count) {
  if (!tracker || !count) {
    return NULL;
  }

  *count = tracker->alert_count;
  return tracker->alerts;
}

void server_perf_clear_alerts(ServerPerformanceTracker *tracker) {
  if (!tracker) {
    return;
  }

  tracker->alert_count = 0;
  LOG_INFO("Cleared all performance alerts");
}

// MARK: - Configuration

void server_perf_set_update_interval(ServerPerformanceTracker *tracker,
                                     u32 interval_ms) {
  if (!tracker) {
    return;
  }

  tracker->update_interval_ms = interval_ms;
  LOG_INFO("Set update interval to %u ms", interval_ms);
}

void server_perf_set_callbacks(ServerPerformanceTracker *tracker,
                               void (*on_warning)(const PerformanceAlert *),
                               void (*on_critical)(const PerformanceAlert *),
                               void (*on_recovery)(const char *)) {
  if (!tracker) {
    return;
  }

  tracker->on_warning = on_warning;
  tracker->on_critical = on_critical;
  tracker->on_recovery = on_recovery;

  LOG_INFO("Set performance alert callbacks");
}

// MARK: - Utility Functions

void server_perf_print_metrics(const ServerPerformanceMetrics *metrics) {
  if (!metrics) {
    printf("Metrics is NULL\n");
    return;
  }

  printf("=== SERVER PERFORMANCE METRICS ===\n");
  printf("CPU Usage: %.2f%% (%u/%u cores)\n", metrics->cpu_usage_percent,
         metrics->cpu_cores_active, metrics->cpu_cores_available);
  printf("Memory: %llu MB used / %llu MB total (%.2f%%)\n",
         metrics->used_memory_mb, metrics->total_memory_mb,
         metrics->memory_usage_percent);
  printf("Network: %.2f ms latency, %u/%u pps, %u/%u Bps\n",
         metrics->network_latency_ms, metrics->packets_sent_per_second,
         metrics->packets_received_per_second, metrics->bytes_sent_per_second,
         metrics->bytes_received_per_second);
  printf("Server: %.1f FPS (%.2f ms tick), %u entities, %u events\n",
         1000.0f / metrics->tick_time_ms, metrics->tick_time_ms,
         metrics->entities_processed_per_tick,
         metrics->network_events_processed_per_tick);
  printf("Connections: %u/%u active\n", metrics->active_connections,
         metrics->max_connections);
  printf("Server Load: %.2f%%\n", metrics->server_load_percentage);
  printf("Health: %s\n",
         metrics->is_performance_healthy ? "HEALTHY" : "UNHEALTHY");
  printf("Timestamp: %llu ms\n", metrics->timestamp_ms);
  printf("===============================\n");
}

void server_perf_print_summary(const ServerPerformanceTracker *tracker) {
  if (!tracker) {
    printf("Tracker is NULL\n");
    return;
  }

  printf("=== SERVER PERFORMANCE SUMMARY ===\n");
  printf("Status: %s\n", tracker->is_tracking ? "TRACKING" : "STOPPED");
  printf("History: %u/%u samples\n", tracker->history.count,
         tracker->history.capacity);
  printf("Alerts: %u/%u active\n", tracker->alert_count,
         tracker->alert_capacity);
  printf("Worst Alert: %s\n",
         server_perf_get_worst_alert_level(tracker) == ALERT_NONE ? "NONE"
         : server_perf_get_worst_alert_level(tracker) == ALERT_WARNING
             ? "WARNING"
             : "CRITICAL");

  if (tracker->history.count > 0) {
    printf("Average CPU: %.2f%% (last %u samples)\n",
           server_perf_get_average_cpu_usage(tracker, tracker->history.count),
           tracker->history.count);
    printf(
        "Average Memory: %.2f%% (last %u samples)\n",
        server_perf_get_average_memory_usage(tracker, tracker->history.count),
        tracker->history.count);
    printf("Average Latency: %.2f ms (last %u samples)\n",
           server_perf_get_average_latency(tracker, tracker->history.count),
           tracker->history.count);
  }

  printf("========================\n");
}

void server_perf_export_to_csv(const ServerPerformanceTracker *tracker,
                               const char *filename) {
  if (!tracker || !filename) {
    return;
  }

  FILE *fp = fopen(filename, "w");
  if (!fp) {
    LOG_ERROR("Failed to open CSV file: %s", filename);
    return;
  }

  // Write CSV header
  fprintf(fp, "timestamp_ms,cpu_usage_percent,memory_usage_percent,network_"
              "latency_ms,tick_time_ms,server_load_percentage,active_"
              "connections,packets_sent_per_second,packets_received_per_second,"
              "bytes_sent_per_second,bytes_received_per_second,is_healthy\n");

  // Write history data
  for (u32 i = 0; i < tracker->history.count; i++) {
    const ServerPerformanceMetrics *metrics =
        server_perf_get_metrics_at_time(tracker, i);
    if (metrics) {
      fprintf(fp, "%llu,%.2f,%.2f,%.2f,%.2f,%.2f,%u,%u,%u,%u,%u,%s\n",
              metrics->timestamp_ms, metrics->cpu_usage_percent,
              metrics->memory_usage_percent, metrics->network_latency_ms,
              metrics->tick_time_ms, metrics->server_load_percentage,
              metrics->active_connections, metrics->packets_sent_per_second,
              metrics->packets_received_per_second,
              metrics->bytes_sent_per_second,
              metrics->bytes_received_per_second,
              metrics->is_performance_healthy ? "true" : "false");
    }
  }

  fclose(fp);
  LOG_INFO("Exported performance data to CSV: %s", filename);
}

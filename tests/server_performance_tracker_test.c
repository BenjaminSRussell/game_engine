/*
 * server_performance_tracker_test.c
 * Load percentage, threshold health, ring history, alerts, and recovery.
 * Compiles server_performance_tracker.c directly and does not link Engine.
 */

#include "core/logging/unified_logger.h"
#include "network/server_performance_tracker.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u64 g_now_ms = 1000;
static int g_warnings = 0;
static int g_criticals = 0;
static int g_recoveries = 0;
static const char *g_last_recovery = NULL;

u64 get_current_time_ms(void) { return g_now_ms; }

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static void on_warning(const PerformanceAlert *alert) {
  (void)alert;
  g_warnings++;
}

static void on_critical(const PerformanceAlert *alert) {
  (void)alert;
  g_criticals++;
}

static void on_recovery(const char *metric_name) {
  g_recoveries++;
  g_last_recovery = metric_name;
}

static void reset_callbacks(void) {
  g_warnings = 0;
  g_criticals = 0;
  g_recoveries = 0;
  g_last_recovery = NULL;
}

static ServerPerformanceMetrics sample_with(float cpu, float memory,
                                            float latency, float tick_ms) {
  ServerPerformanceMetrics sample;
  memset(&sample, 0, sizeof(sample));
  sample.cpu_usage_percent = cpu;
  sample.memory_usage_percent = memory;
  sample.network_latency_ms = latency;
  sample.tick_time_ms = tick_ms;
  sample.tick_rate = tick_ms > 0.0f ? 1000.0f / tick_ms : 0.0f;
  sample.active_connections = 4;
  sample.max_connections = 32;
  return sample;
}

static void expect_float(float got, float expected, const char *msg) {
  if (fabsf(got - expected) > 0.02f) {
    fprintf(stderr, "FAIL: %s (got %f expected %f)\n", msg, got, expected);
    exit(1);
  }
}

static void test_init_and_defaults(void) {
  ServerPerformanceTracker tracker;
  PerformanceThresholds defaults;

  if (server_perf_tracker_init(NULL, 4, 4)) {
    fail("null init succeeded");
  }
  if (server_perf_tracker_init(&tracker, 0, 4)) {
    fail("zero history capacity succeeded");
  }
  if (!server_perf_tracker_init(&tracker, 4, 4)) {
    fail("init");
  }
  if (tracker.is_tracking) {
    fail("tracking before start");
  }

  defaults = server_perf_get_default_thresholds();
  expect_float(defaults.cpu_warning_threshold, 70.0f, "cpu warning");
  expect_float(defaults.cpu_critical_threshold, 85.0f, "cpu critical");
  expect_float(defaults.memory_warning_threshold, 75.0f, "memory warning");
  expect_float(defaults.latency_warning_threshold, 50.0f, "latency warning");
  expect_float(defaults.tick_time_warning_threshold, 25.0f, "tick warning");
  expect_float(tracker.thresholds.tick_time_critical_threshold, 33.34f,
               "tick critical copied onto tracker");

  server_perf_tracker_shutdown(&tracker);
  server_perf_tracker_shutdown(&tracker);
  server_perf_tracker_shutdown(NULL);
}

static void test_submitted_sample_load_alerts_and_recovery(void) {
  ServerPerformanceTracker tracker;
  ServerPerformanceMetrics sample;
  const ServerPerformanceMetrics *current;
  const ServerPerformanceMetrics *newest;
  const ServerPerformanceMetrics *older;
  const ServerPerformanceMetrics *average;
  const PerformanceAlert *alerts = NULL;
  u32 alert_count = 0;
  float expected_load;
  PerformanceThresholds tight;

  reset_callbacks();
  g_now_ms = 5000;
  if (!server_perf_tracker_init(&tracker, 3, 8)) {
    fail("init");
  }
  server_perf_set_callbacks(&tracker, on_warning, on_critical, on_recovery);
  server_perf_set_update_interval(&tracker, 0);
  if (!server_perf_tracker_start(&tracker)) {
    fail("start");
  }

  sample = sample_with(40.0f, 50.0f, 10.0f, 16.67f);
  server_perf_submit_metrics(&tracker, &sample);
  server_perf_tracker_update(&tracker);

  current = server_perf_get_current_metrics(&tracker);
  if (!current || !server_perf_is_healthy(&tracker)) {
    fail("healthy sample");
  }
  if (current->performance_warnings != 0 ||
      server_perf_get_active_alert_count(&tracker) != 0) {
    fail("healthy sample raised alerts");
  }
  if (current->active_connections != 4 || current->max_connections != 32) {
    fail("submitted connection counts were dropped");
  }
  expected_load = 40.0f * 0.4f + 50.0f * 0.3f + 100.0f * 0.3f;
  expect_float(current->server_load_percentage, expected_load, "load");
  if (tracker.history.count != 1) {
    fail("history did not record the first sample");
  }

  server_perf_tracker_stop(&tracker);
  sample = sample_with(95.0f, 10.0f, 1.0f, 10.0f);
  server_perf_submit_metrics(&tracker, &sample);
  server_perf_tracker_update(&tracker);
  if (tracker.history.count != 1) {
    fail("update while stopped recorded a sample");
  }
  if (!server_perf_tracker_start(&tracker)) {
    fail("restart");
  }

  sample = sample_with(72.0f, 10.0f, 1.0f, 10.0f);
  server_perf_submit_metrics(&tracker, &sample);
  g_now_ms += 10;
  server_perf_tracker_update(&tracker);
  if (server_perf_is_healthy(&tracker)) {
    fail("cpu warning still healthy");
  }
  if (g_warnings != 1 || g_criticals != 0) {
    fail("cpu warning callback");
  }
  if (current->performance_warnings != 1) {
    fail("performance_warnings");
  }
  if (server_perf_get_worst_alert_level(&tracker) != ALERT_WARNING) {
    fail("worst alert after warning");
  }

  sample = sample_with(90.0f, 96.0f, 120.0f, 40.0f);
  server_perf_submit_metrics(&tracker, &sample);
  g_now_ms += 10;
  server_perf_tracker_update(&tracker);
  if (g_criticals != 4 || server_perf_get_active_alert_count(&tracker) != 5) {
    fail("critical alerts");
  }
  if (server_perf_get_worst_alert_level(&tracker) != ALERT_CRITICAL) {
    fail("worst alert");
  }
  if (current->performance_warnings != 4) {
    fail("four metrics over threshold");
  }

  alerts = server_perf_get_alerts(&tracker, &alert_count);
  if (!alerts || alert_count != 5 || alerts[1].level != ALERT_CRITICAL) {
    fail("alert buffer");
  }
  if (strcmp(alerts[1].metric_name, "CPU Usage") != 0) {
    fail("alert metric name");
  }

  sample = sample_with(10.0f, 10.0f, 1.0f, 10.0f);
  server_perf_submit_metrics(&tracker, &sample);
  g_now_ms += 10;
  server_perf_tracker_update(&tracker);
  if (!server_perf_is_healthy(&tracker) || g_recoveries != 4) {
    fail("recovery");
  }
  if (!g_last_recovery || strcmp(g_last_recovery, "Tick Time") != 0) {
    fail("recovery order");
  }
  g_recoveries = 0;
  g_now_ms += 10;
  server_perf_tracker_update(&tracker);
  if (g_recoveries != 0) {
    fail("recovery fired twice");
  }

  if (tracker.history.count != 3) {
    fail("history did not cap at 3");
  }
  newest = server_perf_get_metrics_at_time(&tracker, 0);
  older = server_perf_get_metrics_at_time(&tracker, 2);
  if (!newest || !older || newest->cpu_usage_percent != 10.0f ||
      older->cpu_usage_percent != 90.0f) {
    fail("ring buffer order");
  }
  if (server_perf_get_metrics_at_time(&tracker, 3) != NULL) {
    fail("out of range history");
  }

  expect_float(server_perf_get_average_cpu_usage(&tracker, 2), 10.0f,
               "average cpu");
  expect_float(server_perf_get_average_memory_usage(&tracker, 2), 10.0f,
               "average memory");
  expect_float(server_perf_get_average_latency(&tracker, 2), 1.0f,
               "average latency");
  average = server_perf_get_average_metrics(&tracker, 0);
  if (average) {
    free((void *)average);
    fail("average of zero samples");
  }

  server_perf_clear_alerts(&tracker);
  if (server_perf_get_active_alert_count(&tracker) != 0 ||
      server_perf_get_worst_alert_level(&tracker) != ALERT_NONE) {
    fail("clear alerts");
  }

  tight = server_perf_get_default_thresholds();
  tight.cpu_warning_threshold = 1.0f;
  tight.cpu_critical_threshold = 2.0f;
  server_perf_set_thresholds(&tracker, &tight);
  g_now_ms += 10;
  server_perf_tracker_update(&tracker);
  if (server_perf_is_healthy(&tracker) || g_criticals < 5) {
    fail("updated thresholds were ignored");
  }

  server_perf_tracker_shutdown(&tracker);
}

static void test_platform_sample_uses_thresholds_and_load(void) {
  ServerPerformanceTracker tracker;
  PerformanceThresholds thresholds;
  const ServerPerformanceMetrics *current;
  float tick_percent;
  float expected_load;

  g_now_ms = 1000;
  if (!server_perf_tracker_init(&tracker, 4, 4)) {
    fail("init");
  }
  server_perf_set_update_interval(&tracker, 1000);
  if (!server_perf_tracker_start(&tracker)) {
    fail("start");
  }
  server_perf_tracker_update(&tracker);
  if (tracker.history.count != 0) {
    fail("first update ignored the interval");
  }
  g_now_ms = 2000;
  server_perf_tracker_update(&tracker);

  current = server_perf_get_current_metrics(&tracker);
  if (!current || current->tick_time_ms < 1.0f) {
    fail("platform update did not record tick time");
  }
  tick_percent = (current->tick_time_ms / 16.67f) * 100.0f;
  expected_load = current->cpu_usage_percent * 0.4f +
                  current->memory_usage_percent * 0.3f + tick_percent * 0.3f;
  expect_float(current->server_load_percentage, expected_load,
               "platform load");

  thresholds = server_perf_get_default_thresholds();
  thresholds.cpu_warning_threshold = 1000.0f;
  thresholds.cpu_critical_threshold = 2000.0f;
  thresholds.memory_warning_threshold = 1000.0f;
  thresholds.memory_critical_threshold = 2000.0f;
  thresholds.latency_warning_threshold = 1000.0f;
  thresholds.latency_critical_threshold = 2000.0f;
  thresholds.tick_time_warning_threshold = 1.0f;
  thresholds.tick_time_critical_threshold = 2.0f;
  server_perf_set_thresholds(&tracker, &thresholds);
  g_now_ms = 4000;
  server_perf_tracker_update(&tracker);
  if (server_perf_is_healthy(&tracker)) {
    fail("tick warning threshold did not mark the server unhealthy");
  }

  thresholds.tick_time_warning_threshold = 1000.0f;
  thresholds.tick_time_critical_threshold = 2000.0f;
  server_perf_set_thresholds(&tracker, &thresholds);
  g_now_ms = 6000;
  server_perf_tracker_update(&tracker);
  if (!server_perf_is_healthy(&tracker)) {
    fail("high thresholds still reported unhealthy");
  }

  server_perf_submit_metrics(&tracker, NULL);
  server_perf_tracker_shutdown(&tracker);
}

static void test_history_not_power_of_two_and_csv(void) {
  ServerPerformanceTracker tracker;
  ServerPerformanceMetrics sample;
  const ServerPerformanceMetrics *at;
  FILE *fp;
  char line[256];
  int rows = 0;
  const char *path = "/tmp/server_perf_381_test.csv";

  g_now_ms = 10000;
  if (!server_perf_tracker_init(&tracker, 5, 2)) {
    fail("init");
  }
  server_perf_set_update_interval(&tracker, 0);
  server_perf_set_callbacks(&tracker, on_warning, on_critical, NULL);
  if (!server_perf_tracker_start(&tracker)) {
    fail("start");
  }

  sample = sample_with(95.0f, 96.0f, 1.0f, 10.0f);
  server_perf_submit_metrics(&tracker, &sample);
  server_perf_tracker_update(&tracker);
  g_now_ms += 5;
  server_perf_tracker_update(&tracker);
  if (server_perf_get_active_alert_count(&tracker) != 2) {
    fail("alert buffer did not stop at capacity");
  }

  server_perf_clear_alerts(&tracker);
  for (int i = 1; i <= 8; i++) {
    sample = sample_with((float)i, 5.0f, 1.0f, 10.0f);
    server_perf_submit_metrics(&tracker, &sample);
    g_now_ms += 5;
    server_perf_tracker_update(&tracker);
  }

  if (tracker.history.count != 5) {
    fail("capacity 5 history count");
  }
  at = server_perf_get_metrics_at_time(&tracker, 0);
  if (!at || at->cpu_usage_percent != 8.0f) {
    fail("newest sample");
  }
  at = server_perf_get_metrics_at_time(&tracker, 4);
  if (!at || at->cpu_usage_percent != 4.0f) {
    fail("oldest sample after wrap");
  }
  expect_float(server_perf_get_average_cpu_usage(&tracker, 5), 6.0f,
               "wrapped average");

  remove(path);
  server_perf_export_to_csv(&tracker, path);
  fp = fopen(path, "r");
  if (!fp) {
    fail("csv open");
  }
  if (!fgets(line, sizeof(line), fp) || strstr(line, "cpu_usage_percent") == NULL) {
    fclose(fp);
    fail("csv header");
  }
  if (!fgets(line, sizeof(line), fp) || strncmp(line, "100", 3) != 0) {
    fclose(fp);
    fail("csv newest row");
  }
  while (fgets(line, sizeof(line), fp)) {
    rows++;
  }
  fclose(fp);
  remove(path);
  if (rows != 4) {
    fail("csv row count");
  }

  server_perf_tracker_shutdown(&tracker);
}

int main(void) {
  test_init_and_defaults();
  test_submitted_sample_load_alerts_and_recovery();
  test_platform_sample_uses_thresholds_and_load();
  test_history_not_power_of_two_and_csv();
  printf("server performance tracker tests passed\n");
  return 0;
}

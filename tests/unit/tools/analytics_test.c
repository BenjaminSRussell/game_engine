/* Unit tests for src/engine/tools/analytics.c. Issue #407.
 *
 * Compiled directly with the unit under test (tests/registry/test_analytics.cmake)
 * because Engine does not build on origin/main. The logger entry point that
 * LOG_INFO expands to is stubbed here.
 *
 * Covered: init and shutdown, the enabled and initialised gates, the bounded event
 * buffer, FPS/memory/GPU tracking with their event thresholds, engagement counters,
 * session start/end, offline upload gating, and the A/B event helpers.
 *
 * Not covered: the online upload path, offline cache processing and
 * analytics_ab_get_variant. They are TODO stubs (issue #198), so pinning their current
 * behaviour would fight that work. Heatmap contents have no read API, so the heatmap
 * test only checks that the caps hold without out-of-bounds writes.
 */

#include "core/logging/unified_logger.h"
#include "tools/analytics.h"

#include <stdio.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

static int g_failed = 0;

#define EXPECT(cond)                                                           \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

/* Shut down and re-initialise so each test starts from a clean state. */
static void fresh_analytics(void) {
  analytics_shutdown();
  analytics_init(NULL);
}

static void test_init_defaults(void) {
  fresh_analytics();

  PerformanceMetrics perf = analytics_get_performance_metrics();
  EXPECT(perf.current_fps == 60.0);
  EXPECT(perf.min_fps == 60.0);
  EXPECT(perf.max_fps == 60.0);
  EXPECT(perf.memory_usage == 0);
  EXPECT(perf.peak_memory == 0);
  EXPECT(perf.gpu_utilization == 0.0);

  EngagementMetrics eng = analytics_get_engagement_metrics();
  EXPECT(eng.quests_completed == 0);
  EXPECT(eng.items_used == 0);
  EXPECT(eng.deaths == 0);
  EXPECT(eng.current_level == 1);
  EXPECT(eng.session_duration == 0);
  EXPECT(analytics_get_event_count() == 0);
}

static void test_calls_ignored_when_not_initialised(void) {
  fresh_analytics();
  analytics_shutdown();
  u32 events_before = analytics_get_event_count();

  analytics_track_event("ignored", NULL);
  analytics_track_quest_completion("q1");
  analytics_track_session_start();
  analytics_update_fps(10.0);
  analytics_update_memory_usage(5);

  EXPECT(analytics_get_event_count() == events_before);
  EXPECT(analytics_get_performance_metrics().current_fps == 60.0);
  EXPECT(analytics_get_performance_metrics().memory_usage == 0);
  EXPECT(analytics_get_engagement_metrics().quests_completed == 0);
  EXPECT(analytics_get_engagement_metrics().session_start_time == 0);
}

static void test_disabled_system_drops_events_and_counters(void) {
  fresh_analytics();
  analytics_set_enabled(false);

  analytics_track_event("dropped", NULL);
  analytics_track_quest_completion("q1");
  analytics_track_item_usage("potion", 2);
  analytics_track_death("fall", 0.0f, 0.0f, 0.0f);
  analytics_track_level_up(4);
  analytics_track_session_start();
  analytics_ab_test_init("hud", "blue");

  EXPECT(analytics_get_event_count() == 0);
  EngagementMetrics eng = analytics_get_engagement_metrics();
  EXPECT(eng.quests_completed == 0);
  EXPECT(eng.items_used == 0);
  EXPECT(eng.deaths == 0);
  EXPECT(eng.current_level == 1);
  EXPECT(eng.session_start_time == 0);

  analytics_set_enabled(true);
  analytics_track_event("kept", NULL);
  EXPECT(analytics_get_event_count() == 1);
}

static void test_track_event_counts_each_call(void) {
  fresh_analytics();
  analytics_track_event("generic", "a=1");
  EXPECT(analytics_get_event_count() == 1);
  analytics_track_event("no_params", NULL);
  EXPECT(analytics_get_event_count() == 2);
}

static void test_typed_helpers_each_record_one_event(void) {
  fresh_analytics();
  analytics_track_player_action("jump", NULL);
  EXPECT(analytics_get_event_count() == 1);
  analytics_track_performance_metric("frame_ms", 16.5);
  EXPECT(analytics_get_event_count() == 2);
  analytics_track_error("load_failed", NULL);
  EXPECT(analytics_get_event_count() == 3);
  analytics_ab_test_init("hud", "blue");
  EXPECT(analytics_get_event_count() == 4);
  analytics_ab_track_conversion("hud", "blue");
  EXPECT(analytics_get_event_count() == 5);
}

static void test_event_buffer_drops_oldest_when_full(void) {
  fresh_analytics();
  for (u32 i = 0; i < MAX_ANALYTICS_EVENTS + 25; i++) {
    analytics_track_event("tick", NULL);
  }
  EXPECT(analytics_get_event_count() == MAX_ANALYTICS_EVENTS);
}

static void test_fps_min_max_and_low_fps_event(void) {
  fresh_analytics();

  analytics_update_fps(45.0);
  PerformanceMetrics perf = analytics_get_performance_metrics();
  EXPECT(perf.current_fps == 45.0);
  EXPECT(perf.min_fps == 45.0);
  EXPECT(perf.max_fps == 60.0);
  EXPECT(analytics_get_event_count() == 0);

  analytics_update_fps(75.0);
  perf = analytics_get_performance_metrics();
  EXPECT(perf.current_fps == 75.0);
  EXPECT(perf.min_fps == 45.0);
  EXPECT(perf.max_fps == 75.0);

  /* 30 fps is not below the low-fps threshold. */
  analytics_update_fps(30.0);
  EXPECT(analytics_get_event_count() == 0);

  analytics_update_fps(29.0);
  EXPECT(analytics_get_event_count() == 1);
  EXPECT(analytics_get_performance_metrics().min_fps == 29.0);
}

static void test_memory_current_peak_and_high_memory_event(void) {
  fresh_analytics();
  const u64 one_gib = (u64)1024 * 1024 * 1024;

  analytics_update_memory_usage(100);
  analytics_update_memory_usage(50);
  PerformanceMetrics perf = analytics_get_performance_metrics();
  EXPECT(perf.memory_usage == 50);
  EXPECT(perf.peak_memory == 100);
  EXPECT(analytics_get_event_count() == 0);

  /* Exactly 1 GiB is not above the high-memory threshold. */
  analytics_update_memory_usage(one_gib);
  EXPECT(analytics_get_event_count() == 0);

  analytics_update_memory_usage(one_gib + 1);
  EXPECT(analytics_get_event_count() == 1);
  EXPECT(analytics_get_performance_metrics().peak_memory == one_gib + 1);
}

static void test_gpu_usage_and_high_gpu_event(void) {
  fresh_analytics();

  /* 90 is not above the high-GPU threshold. */
  analytics_update_gpu_usage(90.0);
  EXPECT(analytics_get_event_count() == 0);
  EXPECT(analytics_get_performance_metrics().gpu_utilization == 90.0);

  analytics_update_gpu_usage(95.5);
  EXPECT(analytics_get_event_count() == 1);
  EXPECT(analytics_get_performance_metrics().gpu_utilization == 95.5);
}

static void test_engagement_counters(void) {
  fresh_analytics();
  analytics_track_quest_completion("intro");
  analytics_track_quest_completion("forest");
  analytics_track_item_usage("potion", 3);
  analytics_track_item_usage("potion", 2);
  analytics_track_death("lava", 1.0f, 2.0f, 3.0f);
  analytics_track_level_up(7);

  EngagementMetrics eng = analytics_get_engagement_metrics();
  EXPECT(eng.quests_completed == 2);
  EXPECT(eng.items_used == 5);
  EXPECT(eng.deaths == 1);
  EXPECT(eng.current_level == 7);
  /* Two quests, two item uses, one death, one level up. */
  EXPECT(analytics_get_event_count() == 6);
}

static void test_session_start_and_end(void) {
  fresh_analytics();

  analytics_track_session_start();
  EngagementMetrics eng = analytics_get_engagement_metrics();
  EXPECT(eng.session_start_time != 0);
  EXPECT(analytics_get_event_count() == 1); /* session_start */

  analytics_track_session_end();
  EXPECT(analytics_get_event_count() == 2); /* session_end */
  eng = analytics_get_engagement_metrics();
  EXPECT(eng.session_duration >= 0);

  /* The session is already closed, so a second end records nothing. */
  analytics_track_session_end();
  EXPECT(analytics_get_event_count() == 2);
}

static void test_shutdown_closes_active_session(void) {
  fresh_analytics();
  analytics_track_session_start();

  analytics_shutdown();
  EXPECT(analytics_get_event_count() == 2); /* session_start + session_end */

  analytics_shutdown(); /* second shutdown is a no-op */
  EXPECT(analytics_get_event_count() == 2);
}

static void test_offline_mode_blocks_upload(void) {
  fresh_analytics();
  analytics_track_event("queued", NULL);

  analytics_set_offline_mode(true);
  EXPECT(analytics_upload_events() == false);
  /* Nothing was sent, so the queued event is kept. */
  EXPECT(analytics_get_event_count() == 1);
  analytics_set_offline_mode(false);
}

static void test_upload_refused_when_not_initialised(void) {
  fresh_analytics();
  analytics_shutdown();
  EXPECT(analytics_upload_events() == false);
}

static void test_heatmap_caps_hold_without_events(void) {
  /* Only the caps are checked. The heatmap contents have no read API. */
  fresh_analytics();
  for (u32 i = 0; i < MAX_HEATMAP_POINTS + 5; i++) {
    analytics_add_heatmap_point("death_locations", (f32)i, 0.0f, 0.0f);
  }
  char name[32];
  for (u32 i = 0; i < MAX_HEATMAPS + 4; i++) {
    snprintf(name, sizeof(name), "map_%u", i);
    analytics_add_heatmap_point(name, 1.0f, 2.0f, 3.0f);
  }
  EXPECT(analytics_get_event_count() == 0);
}

typedef void (*TestFn)(void);

static const struct {
  const char *name;
  TestFn run;
} kTests[] = {
    {"init_defaults", test_init_defaults},
    {"calls_ignored_when_not_initialised", test_calls_ignored_when_not_initialised},
    {"disabled_system_drops_events_and_counters",
     test_disabled_system_drops_events_and_counters},
    {"track_event_counts_each_call", test_track_event_counts_each_call},
    {"typed_helpers_each_record_one_event", test_typed_helpers_each_record_one_event},
    {"event_buffer_drops_oldest_when_full", test_event_buffer_drops_oldest_when_full},
    {"fps_min_max_and_low_fps_event", test_fps_min_max_and_low_fps_event},
    {"memory_current_peak_and_high_memory_event",
     test_memory_current_peak_and_high_memory_event},
    {"gpu_usage_and_high_gpu_event", test_gpu_usage_and_high_gpu_event},
    {"engagement_counters", test_engagement_counters},
    {"session_start_and_end", test_session_start_and_end},
    {"shutdown_closes_active_session", test_shutdown_closes_active_session},
    {"offline_mode_blocks_upload", test_offline_mode_blocks_upload},
    {"upload_refused_when_not_initialised", test_upload_refused_when_not_initialised},
    {"heatmap_caps_hold_without_events", test_heatmap_caps_hold_without_events},
};

int main(void) {
  const size_t total = sizeof(kTests) / sizeof(kTests[0]);
  int failed_tests = 0;

  for (size_t i = 0; i < total; i++) {
    int failed_before = g_failed;
    kTests[i].run();
    if (g_failed == failed_before) {
      printf("ok %zu - %s\n", i + 1, kTests[i].name);
    } else {
      printf("not ok %zu - %s\n", i + 1, kTests[i].name);
      failed_tests++;
    }
  }

  analytics_shutdown();
  printf("1..%zu\n", total);

  if (failed_tests) {
    fprintf(stderr, "%d analytics test(s) failed\n", failed_tests);
    return 1;
  }
  printf("analytics tests passed\n");
  return 0;
}

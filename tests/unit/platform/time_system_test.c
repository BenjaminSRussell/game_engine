/* Unit test for src/engine/platform/time_system.c (issue #387).
 *
 * Compiled against time_system.c directly, not linked to Engine, which does
 * not build on main. time_system_update() reads the real clock, so each check
 * is either exact whatever the timing (a frame 10 s stale hits the 0.25 s
 * clamp) or bounded so scheduler jitter cannot flip it.
 */

#include "core/time_system.h"

#include <stdio.h>
#include <string.h>

static int g_checks = 0;
static int g_failed = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    g_checks++;                                                                \
    if (!(cond)) {                                                             \
      fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
      g_failed++;                                                              \
    }                                                                          \
  } while (0)

/* Tolerance check that prints both values on failure. */
#define CHECK_NEAR(actual, expected, tol)                                      \
  do {                                                                         \
    double got_ = (double)(actual);                                            \
    double want_ = (double)(expected);                                         \
    double diff_ = got_ > want_ ? got_ - want_ : want_ - got_;                 \
    g_checks++;                                                                \
    if (!(diff_ <= (tol))) {                                                   \
      fprintf(stderr, "  FAIL %s:%d: %s = %.9g, expected %.9g\n", __FILE__,   \
              __LINE__, #actual, got_, want_);                                 \
      g_failed++;                                                              \
    }                                                                          \
  } while (0)

/* Makes the next update see a frame that started `seconds` ago. */
static void age_last_frame(TimeSystem *ts, f64 seconds) {
  ts->last_frame_time = time_get_high_res_time() - seconds;
}

static void test_init_sets_defaults(void) {
  TimeSystem ts;
  memset(&ts, 0xAB, sizeof ts); /* init must overwrite every field */
  time_system_init(&ts, 1.0f / 60.0f);

  CHECK(time_system_get_fixed_delta_time(&ts) == 1.0f / 60.0f);
  CHECK(ts.fixed_step == 1.0f / 60.0f);
  CHECK(ts.time_scale == 1.0f);
  CHECK(!ts.paused);
  CHECK(!ts.slow_motion_active);
  CHECK(ts.accumulator == 0.0f);
  CHECK(ts.interpolation_factor == 0.0f);
  CHECK(ts.frame_count == 0);
  CHECK(ts.fps == 60.0f);
  CHECK(ts.min_frame_time_ms == 1000.0f);
  CHECK(ts.max_frame_time_ms == 0.0f);
  /* Nothing has advanced the clock yet, so total time is exactly zero. */
  CHECK(ts.start_time == ts.current_time);
  CHECK(ts.current_time == ts.last_frame_time);
  CHECK(time_system_get_total_time(&ts) == 0.0);
}

static void test_null_arguments_are_safe(void) {
  /* None of these may dereference NULL. */
  time_system_init(NULL, 0.1f);
  time_system_update(NULL);
  time_system_shutdown(NULL);
  time_system_pause(NULL);
  time_system_resume(NULL);
  time_system_set_time_scale(NULL, 2.0f);
  time_system_set_slow_motion(NULL, 0.5f, true);

  CHECK(time_system_get_delta_time(NULL) == 0.0f);
  CHECK(time_system_get_fixed_delta_time(NULL) == 0.0f);
  CHECK(time_system_get_time(NULL) == 0.0);
  CHECK(time_system_get_total_time(NULL) == 0.0);
  CHECK(time_system_get_fps(NULL) == 0.0f);
  CHECK(time_system_get_interpolation_factor(NULL) == 0.0f);
  CHECK(!time_system_is_paused(NULL));
}

static void test_update_clamps_long_frames(void) {
  TimeSystem ts;
  time_system_init(&ts, 1.0f); /* 1 s fixed step */
  age_last_frame(&ts, 10.0);
  time_system_update(&ts);

  /* A 10 s stall is clamped to the 0.25 s maximum, which is exact in float. */
  CHECK(ts.frame_count == 1);
  CHECK(time_system_get_delta_time(&ts) == 0.25f);
  CHECK(ts.frame_time_ms == 250.0f);
  CHECK(ts.accumulator == 0.25f);
}

static void test_update_uses_elapsed_time(void) {
  TimeSystem ts;
  time_system_init(&ts, 1.0f);
  age_last_frame(&ts, 0.05);
  time_system_update(&ts);

  /* The clock only moves forward, so the frame is at least 50 ms. The upper
   * bound leaves 200 ms of slack so scheduler noise cannot flip it. */
  f32 dt = time_system_get_delta_time(&ts);
  CHECK(dt >= 0.049f && dt < 0.25f);
  CHECK(ts.accumulator == dt);
}

static void test_time_scale_clamps_and_scales_delta(void) {
  TimeSystem ts;
  time_system_init(&ts, 1.0f);

  time_system_set_time_scale(&ts, -3.0f);
  CHECK(ts.time_scale == 0.0f);
  time_system_set_time_scale(&ts, 25.0f);
  CHECK(ts.time_scale == 10.0f);
  time_system_set_time_scale(&ts, 2.5f);
  CHECK(ts.time_scale == 2.5f);

  /* Scale 0 freezes time: delta is exactly zero and the accumulator stays put.
   * FPS is not checked here: it divides by delta, so it becomes inf. */
  time_system_set_time_scale(&ts, 0.0f);
  age_last_frame(&ts, 10.0);
  time_system_update(&ts);
  CHECK(time_system_get_delta_time(&ts) == 0.0f);
  CHECK(ts.accumulator == 0.0f);

  /* Scale 2 doubles a real 50 ms frame to about 100 ms. */
  time_system_set_time_scale(&ts, 2.0f);
  age_last_frame(&ts, 0.05);
  time_system_update(&ts);
  f32 dt = time_system_get_delta_time(&ts);
  CHECK(dt >= 0.099f && dt < 0.5f);
}

static void test_slow_motion_scales_delta_only_when_active(void) {
  TimeSystem ts;
  time_system_init(&ts, 1.0f);

  time_system_set_slow_motion(&ts, 0.5f, true);
  age_last_frame(&ts, 10.0);
  time_system_update(&ts);
  CHECK(time_system_get_delta_time(&ts) == 0.125f); /* 0.25 s clamp * 0.5 */

  time_system_set_slow_motion(&ts, 0.5f, false);
  age_last_frame(&ts, 10.0);
  time_system_update(&ts);
  CHECK(time_system_get_delta_time(&ts) == 0.25f);
}

static void test_pause_freezes_time(void) {
  TimeSystem ts;
  time_system_init(&ts, 1.0f);

  time_system_pause(&ts);
  CHECK(time_system_is_paused(&ts));
  age_last_frame(&ts, 10.0);
  time_system_update(&ts);
  CHECK(ts.frame_count == 0);
  CHECK(ts.accumulator == 0.0f);
  CHECK(time_system_get_delta_time(&ts) == 0.0f);

  time_system_resume(&ts);
  CHECK(!time_system_is_paused(&ts));
  age_last_frame(&ts, 10.0);
  time_system_update(&ts);
  CHECK(ts.frame_count == 1);
  CHECK(time_system_get_delta_time(&ts) == 0.25f);
}

static void test_interpolation_factor_saturates_at_one(void) {
  TimeSystem ts;
  /* With a 1 s fixed step, each clamped 0.25 s frame adds exactly 0.25. */
  time_system_init(&ts, 1.0f);

  const f32 expected[] = {0.25f, 0.5f, 0.75f, 1.0f, 1.0f};
  for (size_t i = 0; i < sizeof expected / sizeof expected[0]; i++) {
    age_last_frame(&ts, 10.0);
    time_system_update(&ts);
    CHECK(time_system_get_interpolation_factor(&ts) == expected[i]);
  }
}

static void test_frame_statistics_and_fps(void) {
  TimeSystem ts;
  time_system_init(&ts, 1.0f / 60.0f);

  age_last_frame(&ts, 10.0);
  time_system_update(&ts);
  /* Instant fps is 1 / 0.25 = 4; smoothing is 0.9 * 60 + 0.1 * 4. */
  CHECK_NEAR(time_system_get_fps(&ts), 54.4, 1e-3);
  CHECK(ts.frame_time_ms == 250.0f);
  CHECK(ts.min_frame_time_ms == 250.0f);
  CHECK(ts.max_frame_time_ms == 250.0f);
  CHECK(ts.frame_count == 1);

  /* A shorter real frame lowers the minimum and leaves the maximum alone. */
  age_last_frame(&ts, 0.05);
  time_system_update(&ts);
  CHECK(ts.frame_count == 2);
  CHECK(ts.min_frame_time_ms >= 49.0f && ts.min_frame_time_ms < 250.0f);
  CHECK(ts.max_frame_time_ms == 250.0f);
}

static void test_time_getters_track_clock(void) {
  TimeSystem ts;
  time_system_init(&ts, 1.0f);
  age_last_frame(&ts, 0.05);
  time_system_update(&ts);

  CHECK(time_system_get_time(&ts) == ts.current_time);
  CHECK(time_system_get_total_time(&ts) == ts.current_time - ts.start_time);
  CHECK(time_system_get_total_time(&ts) >= 0.0);
}

static void test_clock_helpers_are_monotonic(void) {
  f64 t0 = time_get_high_res_time();
  f64 t1 = time_get_high_res_time();
  CHECK(t1 >= t0);
  CHECK(time_get_elapsed_seconds(t0) >= 0.0);

  u64 n0 = get_time_nanos();
  u64 n1 = get_time_nanos();
  CHECK(n1 >= n0);

  u64 m0 = time_get_current_ms();
  u64 m1 = time_get_current_ms();
  CHECK(m1 >= m0);

  CHECK(nanos_to_ms(0) == 0.0f);
  CHECK(nanos_to_ms(1500000ULL) == 1.5f);
  CHECK(nanos_to_ms(2000000000ULL) == 2000.0f);
}

static void test_timestamp_formatting(void) {
  DateTime stamp = {2026, 10, 5, 9, 7, 3, 42};
  char buf[64];
  char small[8];

  CHECK(time_format_timestamp(&stamp, buf, sizeof buf) == buf);
  CHECK(strcmp(buf, "2026-10-05 09:07:03.042") == 0);

  /* snprintf truncates to the buffer and still terminates it. */
  CHECK(time_format_timestamp(&stamp, small, sizeof small) == small);
  CHECK(strcmp(small, "2026-10") == 0);

  CHECK(time_format_timestamp(NULL, buf, sizeof buf) == NULL);
  CHECK(time_format_timestamp(&stamp, NULL, sizeof buf) == NULL);
  CHECK(time_format_timestamp(&stamp, buf, 0) == NULL);
}

static void test_current_timestamp_is_plausible(void) {
  DateTime now = time_get_timestamp();
  CHECK(now.year >= 2000);
  CHECK(now.month >= 1 && now.month <= 12);
  CHECK(now.day >= 1 && now.day <= 31);
  CHECK(now.hour >= 0 && now.hour <= 23);
  CHECK(now.minute >= 0 && now.minute <= 59);
  CHECK(now.second >= 0 && now.second <= 60);
  CHECK(now.millisecond >= 0 && now.millisecond <= 999);
}

#define RUN(test)                                                              \
  do {                                                                         \
    int before_ = g_failed;                                                    \
    test();                                                                    \
    printf("%-40s %s\n", #test, g_failed == before_ ? "ok" : "FAILED");        \
  } while (0)

int main(void) {
  RUN(test_init_sets_defaults);
  RUN(test_null_arguments_are_safe);
  RUN(test_update_clamps_long_frames);
  RUN(test_update_uses_elapsed_time);
  RUN(test_time_scale_clamps_and_scales_delta);
  RUN(test_slow_motion_scales_delta_only_when_active);
  RUN(test_pause_freezes_time);
  RUN(test_interpolation_factor_saturates_at_one);
  RUN(test_frame_statistics_and_fps);
  RUN(test_time_getters_track_clock);
  RUN(test_clock_helpers_are_monotonic);
  RUN(test_timestamp_formatting);
  RUN(test_current_timestamp_is_plausible);

  printf("%d checks, %d failed\n", g_checks, g_failed);
  return g_failed == 0 ? 0 : 1;
}

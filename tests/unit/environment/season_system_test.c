/* Issue #250: unit test for src/engine/environment/season_system.c.
 *
 * Compiled directly against the unit, because the Engine target does not
 * build on main (see tests/registry/test_season_system.cmake). The unit keeps
 * its season and elapsed time in file-scope statics with no reset function,
 * so this file includes the .c to reset them between cases. Checks are
 * explicit rather than assert(), so they still run when NDEBUG is defined.
 */
#include <stdio.h>

#include "environment/season_system.c"

static int g_failures = 0;

static void expect(const char *what, int ok) {
  if (!ok) {
    fprintf(stderr, "FAIL: %s\n", what);
    g_failures++;
  }
}

static void expect_season(const char *what, Season want) {
  Season got = get_current_season();
  if (got != want) {
    fprintf(stderr, "FAIL: %s: got season %d, want %d\n", what, (int)got,
            (int)want);
    g_failures++;
  }
}

/* Restores the state the unit has at program start. */
static void reset_season_state(void) {
  current_season = SEASON_SPRING;
  time_in_season = 0.0f;
}

static void test_init_starts_in_spring_without_transition(void) {
  reset_season_state();
  season_system_init();
  expect_season("init leaves the season in spring", SEASON_SPRING);
  expect("init leaves elapsed time at zero", time_in_season == 0.0f);
}

static void test_no_transition_before_duration(void) {
  reset_season_state();
  season_system_update(0.0f);
  season_system_update(150.0f);
  season_system_update(149.0f); /* total 299, below the 300 duration */
  expect_season("no transition before the duration elapses", SEASON_SPRING);
  expect("elapsed time accumulates across updates", time_in_season == 299.0f);
}

static void test_exact_duration_does_not_transition(void) {
  reset_season_state();
  season_system_update(150.0f);
  season_system_update(150.0f); /* total 300: the comparison is strict */
  expect_season("exactly the duration does not transition", SEASON_SPRING);
  expect("elapsed time at the duration is kept", time_in_season == 300.0f);
  season_system_update(0.5f); /* total 300.5, now past the duration */
  expect_season("first update past the duration transitions", SEASON_SUMMER);
}

static void test_transitions_cycle_through_all_seasons(void) {
  reset_season_state();
  expect_season("starts in spring", SEASON_SPRING);
  season_system_update(season_duration + 1.0f);
  expect_season("spring to summer", SEASON_SUMMER);
  season_system_update(season_duration + 1.0f);
  expect_season("summer to autumn", SEASON_AUTUMN);
  season_system_update(season_duration + 1.0f);
  expect_season("autumn to winter", SEASON_WINTER);
  season_system_update(season_duration + 1.0f);
  expect_season("winter wraps back to spring", SEASON_SPRING);
}

static void test_transition_resets_elapsed_time(void) {
  reset_season_state();
  season_system_update(season_duration + 10.0f); /* 310: transitions */
  expect_season("overshooting update transitions", SEASON_SUMMER);
  expect("elapsed time resets to zero on transition",
         time_in_season == 0.0f);
  season_system_update(299.0f);
  expect_season("no second transition right after the reset", SEASON_SUMMER);
  season_system_update(2.0f); /* 301: the next transition counts from reset */
  expect_season("next transition counts from the reset", SEASON_AUTUMN);
}

static void test_get_current_season_reports_state(void) {
  reset_season_state();
  expect("getter reports spring", get_current_season() == SEASON_SPRING);
  current_season = SEASON_WINTER;
  expect("getter reflects the current season",
         get_current_season() == SEASON_WINTER);
}

int main(void) {
  test_init_starts_in_spring_without_transition();
  test_no_transition_before_duration();
  test_exact_duration_does_not_transition();
  test_transitions_cycle_through_all_seasons();
  test_transition_resets_elapsed_time();
  test_get_current_season_reports_state();

  if (g_failures != 0) {
    fprintf(stderr, "test_season_system: %d check(s) failed\n", g_failures);
    return 1;
  }
  printf("test_season_system: all checks passed\n");
  return 0;
}

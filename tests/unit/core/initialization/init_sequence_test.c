/* Proves the ordering contract of engine_init, engine_shutdown and
 * engine_is_running in src/engine/core/initialization/private/init_sequence.c.
 * Issue #253. Compiles init_sequence.c directly with stubs for the platform,
 * subsystem registry and error recovery calls it makes, because add_engine_test
 * links Engine, which does not build on origin/main.
 */

#include "core/initialization/public/engine_init.h"
#include "core/initialization/public/subsystem_registry.h"
#include "core/logging/unified_logger.h"

#include <stdio.h>
#include <string.h>

#define MAX_CALLS 16
#define ARRAY_COUNT(array) ((int)(sizeof(array) / sizeof((array)[0])))

/* Stubs record every call in order, so tests can assert the sequence. */
static const char *g_calls[MAX_CALLS];
static int g_call_count = 0;
static bool g_platform_init_ok = true;
static bool g_subsystem_init_ok = true;

static void record_call(const char *name) {
  if (g_call_count < MAX_CALLS) {
    g_calls[g_call_count] = name;
  }
  g_call_count++;
}

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

void error_recovery_init(void) { record_call("error_recovery_init"); }

bool platform_init(const EngineConfig *config) {
  (void)config;
  record_call("platform_init");
  return g_platform_init_ok;
}

void platform_shutdown(void) { record_call("platform_shutdown"); }

bool subsystem_registry_initialize_all(const EngineConfig *config) {
  (void)config;
  record_call("subsystem_registry_initialize_all");
  return g_subsystem_init_ok;
}

void subsystem_registry_shutdown_all(void) {
  record_call("subsystem_registry_shutdown_all");
}

static int g_failed = 0;

static bool calls_start_with(const char *const expected[], int count) {
  if (count > MAX_CALLS || g_call_count < count) {
    return false;
  }
  for (int i = 0; i < count; i++) {
    if (strcmp(g_calls[i], expected[i]) != 0) {
      return false;
    }
  }
  return true;
}

static bool calls_equal(const char *const expected[], int count) {
  return g_call_count == count && calls_start_with(expected, count);
}

static bool last_call_is(const char *name) {
  return g_call_count > 0 && g_call_count <= MAX_CALLS &&
         strcmp(g_calls[g_call_count - 1], name) == 0;
}

static void print_calls(void) {
  fprintf(stderr, "  recorded %d call(s):", g_call_count);
  for (int i = 0; i < g_call_count && i < MAX_CALLS; i++) {
    fprintf(stderr, " %s", g_calls[i]);
  }
  fprintf(stderr, "\n");
}

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);          \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define EXPECT_CALLS(expected, msg)                                            \
  do {                                                                         \
    if (!calls_equal((expected), ARRAY_COUNT(expected))) {                     \
      print_calls();                                                           \
      fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);          \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

static const char *const kInitOrder[] = {"error_recovery_init", "platform_init",
                                         "subsystem_registry_initialize_all"};
static const char *const kPlatformFailure[] = {"error_recovery_init",
                                               "platform_init"};
static const char *const kShutdownOrder[] = {"subsystem_registry_shutdown_all",
                                             "platform_shutdown"};

/* Clears the running flag left by an earlier test, then the stub state. */
static void reset_stubs(void) {
  engine_shutdown();
  g_call_count = 0;
  g_platform_init_ok = true;
  g_subsystem_init_ok = true;
}

static void test_null_config_fails_without_side_effects(void) {
  EXPECT(engine_init(NULL) == ENGINE_INIT_FAILURE_UNKNOWN,
         "NULL config should return ENGINE_INIT_FAILURE_UNKNOWN");
  EXPECT(g_call_count == 0, "NULL config must not run any init step");
  EXPECT(!engine_is_running(), "engine must not be running after NULL config");
}

static void test_init_runs_error_recovery_then_platform_then_subsystems(void) {
  EngineConfig config = {0};
  EXPECT(engine_init(&config) == ENGINE_INIT_SUCCESS,
         "init should succeed when every step succeeds");
  EXPECT_CALLS(kInitOrder,
               "init must run error recovery, platform, then subsystems");
  EXPECT(engine_is_running(), "engine should be running after successful init");
}

static void test_platform_failure_skips_subsystems(void) {
  EngineConfig config = {0};
  g_platform_init_ok = false;
  EXPECT(engine_init(&config) == ENGINE_INIT_FAILURE_WINDOW,
         "platform failure should return ENGINE_INIT_FAILURE_WINDOW");
  EXPECT_CALLS(kPlatformFailure,
               "platform failure must stop before subsystems start");
  EXPECT(!engine_is_running(), "engine must not be running after platform failure");
}

static void test_subsystem_failure_shuts_down_platform_last(void) {
  EngineConfig config = {0};
  g_subsystem_init_ok = false;
  EXPECT(engine_init(&config) == ENGINE_INIT_FAILURE_SUBSYSTEM,
         "subsystem failure should return ENGINE_INIT_FAILURE_SUBSYSTEM");
  EXPECT(calls_start_with(kInitOrder, ARRAY_COUNT(kInitOrder)),
         "subsystem failure must follow error recovery, platform, then subsystems");
  /* Subsystems that initialized before the failure are not torn down on this
   * path. The test does not assert that either way, so it does not lock in
   * that gap. */
  EXPECT(last_call_is("platform_shutdown"),
         "subsystem failure must shut the platform down as the final step");
  EXPECT(!engine_is_running(),
         "engine must not be running after subsystem failure");
}

static void test_shutdown_stops_subsystems_before_platform(void) {
  EngineConfig config = {0};
  EXPECT(engine_init(&config) == ENGINE_INIT_SUCCESS,
         "setup: init should succeed");
  g_call_count = 0;
  engine_shutdown();
  EXPECT_CALLS(kShutdownOrder,
               "shutdown must stop subsystems before the platform");
  EXPECT(!engine_is_running(), "engine must not be running after shutdown");
}

static void test_shutdown_is_noop_unless_running(void) {
  engine_shutdown();
  EXPECT(g_call_count == 0,
         "shutdown before init must not touch subsystems or platform");

  EngineConfig config = {0};
  g_platform_init_ok = false;
  EXPECT(engine_init(&config) == ENGINE_INIT_FAILURE_WINDOW,
         "setup: platform failure should fail init");
  g_call_count = 0;
  engine_shutdown();
  EXPECT(g_call_count == 0,
         "shutdown after a failed init must not tear anything down");

  g_platform_init_ok = true;
  EXPECT(engine_init(&config) == ENGINE_INIT_SUCCESS,
         "setup: init should succeed");
  engine_shutdown();
  g_call_count = 0;
  engine_shutdown();
  EXPECT(g_call_count == 0, "a second shutdown must not repeat teardown");
}

static void test_init_succeeds_again_after_shutdown(void) {
  EngineConfig config = {0};
  EXPECT(engine_init(&config) == ENGINE_INIT_SUCCESS,
         "first init should succeed");
  engine_shutdown();
  g_call_count = 0;
  EXPECT(engine_init(&config) == ENGINE_INIT_SUCCESS,
         "init should succeed again after shutdown");
  EXPECT_CALLS(kInitOrder, "re-init must run the same ordered steps");
  EXPECT(engine_is_running(), "engine should be running after re-init");
}

#define RUN(test)                                                              \
  do {                                                                         \
    int failed_before = g_failed;                                              \
    reset_stubs();                                                             \
    test();                                                                    \
    printf("%s %s\n", g_failed == failed_before ? "PASS" : "FAIL", #test);     \
  } while (0)

int main(void) {
  RUN(test_null_config_fails_without_side_effects);
  RUN(test_init_runs_error_recovery_then_platform_then_subsystems);
  RUN(test_platform_failure_skips_subsystems);
  RUN(test_subsystem_failure_shuts_down_platform_last);
  RUN(test_shutdown_stops_subsystems_before_platform);
  RUN(test_shutdown_is_noop_unless_running);
  RUN(test_init_succeeds_again_after_shutdown);

  if (g_failed) {
    printf("init_sequence_test: %d check(s) failed\n", g_failed);
    return 1;
  }
  printf("init_sequence_test: all tests passed\n");
  return 0;
}

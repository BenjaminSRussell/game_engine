/* Unit tests for the developer console command parser and dispatcher.
 * Issue #231. Compiles src/engine/scripting/console.c directly, because
 * add_engine_test links Engine, which does not build on origin/main.
 *
 * console.c keeps its log history private, so this file stubs the logger and
 * captures every line that console_print / console_print_error emit.
 */

#include "core/logging/unified_logger.h"
#include "scripting/console.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CAPTURED_LOG_CAPACITY 512
#define CAPTURED_TEXT_SIZE 512
#define MAX_SEEN_ARGS 32
/* Must match MAX_COMMANDS in console.c. */
#define COMMAND_CAPACITY 256

/* ---- Logger capture ----------------------------------------------------- */

typedef struct {
  LogLevel level;
  char text[CAPTURED_TEXT_SIZE];
} CapturedLog;

static CapturedLog g_logs[CAPTURED_LOG_CAPACITY];
static size_t g_log_count = 0;

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  if (g_log_count >= CAPTURED_LOG_CAPACITY) {
    return;
  }
  va_list args;
  va_start(args, format);
  vsnprintf(g_logs[g_log_count].text, sizeof g_logs[g_log_count].text, format,
            args);
  va_end(args);
  g_logs[g_log_count].level = level;
  g_log_count++;
}

static bool was_logged(LogLevel level, const char *text) {
  for (size_t i = 0; i < g_log_count; i++) {
    if (g_logs[i].level == level && strcmp(g_logs[i].text, text) == 0) {
      return true;
    }
  }
  return false;
}

/* ---- Command recorders -------------------------------------------------- */

static int g_greet_calls = 0;
static int g_greet_argc = -1;
static char g_greet_argv[MAX_SEEN_ARGS][64];
static int g_other_calls = 0;

static void greet_command(int argc, char **argv) {
  g_greet_calls++;
  g_greet_argc = argc;
  for (int i = 0; i < argc && i < MAX_SEEN_ARGS; i++) {
    snprintf(g_greet_argv[i], sizeof g_greet_argv[i], "%s", argv[i]);
  }
}

static void other_command(int argc, char **argv) {
  (void)argc;
  (void)argv;
  g_other_calls++;
}

static void fresh_console(void) {
  g_log_count = 0;
  g_greet_calls = 0;
  g_greet_argc = -1;
  memset(g_greet_argv, 0, sizeof g_greet_argv);
  g_other_calls = 0;
  console_init();
  g_log_count = 0; /* Drop the "Developer Console initialized" line. */
}

/* ---- Harness ------------------------------------------------------------ */

static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

/* ---- Tests -------------------------------------------------------------- */

static void test_dispatch_passes_argc_and_argv(void) {
  fresh_console();
  console_register_command("greet", greet_command, "Say hello");

  console_execute("greet alpha beta");

  EXPECT(g_greet_calls == 1, "greet should run exactly once");
  EXPECT(g_greet_argc == 3, "argc should count the name and both arguments");
  EXPECT(strcmp(g_greet_argv[0], "greet") == 0,
         "argv[0] should be the command name");
  EXPECT(strcmp(g_greet_argv[1], "alpha") == 0,
         "argv[1] should be the first argument");
  EXPECT(strcmp(g_greet_argv[2], "beta") == 0,
         "argv[2] should be the second argument");
}

static void test_dispatch_selects_named_command(void) {
  fresh_console();
  console_register_command("greet", greet_command, "Say hello");
  console_register_command("other", other_command, "Do something else");

  console_execute("other x");

  EXPECT(g_other_calls == 1, "the named command should run");
  EXPECT(g_greet_calls == 0, "commands with other names must not run");
}

static void test_command_line_is_echoed(void) {
  fresh_console();
  console_register_command("greet", greet_command, "Say hello");

  console_execute("greet alpha beta");

  EXPECT(was_logged(LOG_LEVEL_INFO, "[Console] greet alpha beta"),
         "the raw command line should be echoed before dispatch");
}

static void test_repeated_spaces_are_skipped(void) {
  fresh_console();
  console_register_command("greet", greet_command, "Say hello");

  console_execute("  greet   alpha  beta  ");

  EXPECT(g_greet_calls == 1, "a padded command line should still dispatch");
  EXPECT(g_greet_argc == 3,
         "runs of spaces should not produce empty arguments");
  EXPECT(strcmp(g_greet_argv[1], "alpha") == 0, "argv[1] should be alpha");
  EXPECT(strcmp(g_greet_argv[2], "beta") == 0, "argv[2] should be beta");
}

static void test_unknown_command_reports_error(void) {
  fresh_console();
  console_register_command("greet", greet_command, "Say hello");

  console_execute("nosuch alpha");

  EXPECT(g_greet_calls == 0,
         "an unknown name must not dispatch to other commands");
  EXPECT(was_logged(LOG_LEVEL_ERROR, "[Console] Unknown command"),
         "an unknown command should log an error");
}

static void test_empty_and_null_input_is_ignored(void) {
  fresh_console();
  console_register_command("greet", greet_command, "Say hello");

  console_execute(NULL);
  console_execute("");

  EXPECT(g_greet_calls == 0, "empty input must not dispatch");
  EXPECT(g_log_count == 0,
         "empty input must not echo or report an error");
}

static void test_command_names_are_case_sensitive(void) {
  fresh_console();
  console_register_command("greet", greet_command, "Say hello");

  console_execute("Greet alpha");

  EXPECT(g_greet_calls == 0, "a differently cased name must not dispatch");
  EXPECT(was_logged(LOG_LEVEL_ERROR, "[Console] Unknown command"),
         "a differently cased name should be reported as unknown");
}

static void test_argument_count_is_capped_at_32(void) {
  char line[512] = "greet";
  fresh_console();
  console_register_command("greet", greet_command, "Say hello");
  for (int i = 1; i < 40; i++) {
    char token[16];
    snprintf(token, sizeof token, " t%d", i);
    strncat(line, token, sizeof line - strlen(line) - 1);
  }

  console_execute(line);

  EXPECT(g_greet_calls == 1, "an over-long command should still dispatch");
  EXPECT(g_greet_argc == 32, "argc should stop at 32 tokens");
  EXPECT(strcmp(g_greet_argv[31], "t31") == 0,
         "the 32nd token should be the last argument");
}

static void test_help_lists_builtin_and_registered_commands(void) {
  fresh_console();
  console_register_command("greet", greet_command, "Say hello");

  console_execute("help");

  EXPECT(was_logged(LOG_LEVEL_INFO, "[Console] Available commands:"),
         "help should print a header");
  EXPECT(was_logged(LOG_LEVEL_INFO, "[Console]   help - List all commands"),
         "help should list itself");
  EXPECT(was_logged(LOG_LEVEL_INFO, "[Console]   clear - Clear console output"),
         "help should list the built-in clear command");
  EXPECT(was_logged(LOG_LEVEL_INFO, "[Console]   greet - Say hello"),
         "help should list registered commands");
}

static void test_registration_stops_at_command_capacity(void) {
  char name[32];
  fresh_console();
  /* console_init has already registered help and clear. */
  for (int i = 0; i < COMMAND_CAPACITY - 2; i++) {
    snprintf(name, sizeof name, "cmd_%d", i);
    console_register_command(name, other_command, NULL);
  }
  console_register_command("overflow", other_command, "Too late");

  EXPECT(was_logged(LOG_LEVEL_ERROR, "Console command limit reached"),
         "registering past capacity should log an error");

  console_execute("overflow");
  EXPECT(g_other_calls == 0, "a rejected command must not dispatch");
  EXPECT(was_logged(LOG_LEVEL_ERROR, "[Console] Unknown command"),
         "a rejected command should be reported as unknown");

  snprintf(name, sizeof name, "cmd_%d", COMMAND_CAPACITY - 3);
  console_execute(name);
  EXPECT(g_other_calls == 1,
         "the last accepted command should still dispatch");
}

int main(void) {
  test_dispatch_passes_argc_and_argv();
  test_dispatch_selects_named_command();
  test_command_line_is_echoed();
  test_repeated_spaces_are_skipped();
  test_unknown_command_reports_error();
  test_empty_and_null_input_is_ignored();
  test_command_names_are_case_sensitive();
  test_argument_count_is_capped_at_32();
  test_help_lists_builtin_and_registered_commands();
  test_registration_stops_at_command_capacity();

  if (g_failed) {
    fprintf(stderr, "%d console test check(s) failed\n", g_failed);
    return 1;
  }
  printf("console command tests passed\n");
  return 0;
}

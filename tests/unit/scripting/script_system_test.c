/* Proves the script system's commands and variables do what their registered
 * descriptions say. Issue #147: src/engine/scripting/script_system.c was
 * excluded from Engine with a "syntax error" note. That error (a stray ')' in
 * ScriptSystem_Init) was already fixed, so the unit is compiled directly here,
 * because Engine does not build on main. Only the logger entry point is stubbed.
 */

#define _POSIX_C_SOURCE 200809L

#include "core/logging/unified_logger.h"
#include "scripting/script_system.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef void (*ScriptCommandFn)(const char *args);
bool ScriptSystem_RegisterCommand(const char *name, ScriptCommandFn func,
                                  const char *desc);
float ScriptSystem_GetVar(const char *name, float default_val);
void ScriptSystem_SetVar(const char *name, float value);

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

static int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failures++;                                                            \
    }                                                                          \
  } while (0)

/* Runs one script line and copies whatever it printed to stdout into buf. */
static bool run_captured(ScriptSystem *sys, const char *line, char *buf,
                         size_t buf_size) {
  buf[0] = '\0';
  fflush(stdout);
  FILE *tmp = tmpfile();
  if (!tmp)
    return false;
  int saved = dup(fileno(stdout));
  dup2(fileno(tmp), fileno(stdout));
  bool ok = ScriptSystem_RunString(sys, line);
  fflush(stdout);
  dup2(saved, fileno(stdout));
  close(saved);
  rewind(tmp);
  size_t n = fread(buf, 1, buf_size - 1, tmp);
  buf[n] = '\0';
  fclose(tmp);
  return ok;
}

static int g_ping_calls = 0;
static char g_ping_args[64];

static void ping_command(const char *args) {
  g_ping_calls++;
  snprintf(g_ping_args, sizeof g_ping_args, "%s", args);
}

int main(void) {
  ScriptSystem sys = {0};
  char out[2048];

  CHECK(!ScriptSystem_Init(NULL));
  CHECK(ScriptSystem_Init(&sys));

  /* Blank input is accepted; a null script or a null system is rejected. */
  CHECK(ScriptSystem_RunString(&sys, ""));
  CHECK(ScriptSystem_RunString(&sys, "  \t "));
  CHECK(!ScriptSystem_RunString(&sys, NULL));
  CHECK(!ScriptSystem_RunString(NULL, "help"));
  CHECK(!ScriptSystem_RunString(&sys, "nosuchcommand 1 2"));

  /* print echoes its argument text. */
  CHECK(run_captured(&sys, "print hello world", out, sizeof out));
  CHECK(strcmp(out, "hello world\n") == 0);

  /* set stores a variable; get prints it; GetVar reads the same store. */
  CHECK(ScriptSystem_RunString(&sys, "set speed 2.5"));
  CHECK(ScriptSystem_GetVar("speed", -1.0f) == 2.5f);
  CHECK(run_captured(&sys, "get speed", out, sizeof out));
  CHECK(strcmp(out, "speed = 2.5\n") == 0);
  CHECK(ScriptSystem_RunString(&sys, "set speed 4"));
  CHECK(ScriptSystem_GetVar("speed", -1.0f) == 4.0f);
  CHECK(run_captured(&sys, "get missing_var", out, sizeof out));
  CHECK(strcmp(out, "missing_var is not set\n") == 0);
  CHECK(run_captured(&sys, "get", out, sizeof out));
  CHECK(strcmp(out, "usage: get <name>\n") == 0);
  ScriptSystem_SetVar("gravity", 9.8f);
  CHECK(ScriptSystem_GetVar("gravity", 0.0f) == 9.8f);
  CHECK(ScriptSystem_GetVar("unset_name", 7.0f) == 7.0f);

  /* help lists the builtins with their descriptions. */
  CHECK(run_captured(&sys, "help", out, sizeof out));
  CHECK(strstr(out, "  help - Show available commands\n") != NULL);
  CHECK(strstr(out, "  print - Print a message\n") != NULL);
  CHECK(strstr(out, "  set - Set a variable: set <name> <value>\n") != NULL);

  /* A registered command receives the text that follows its name. */
  CHECK(ScriptSystem_RegisterCommand("ping", ping_command, "Ping: ping [text]"));
  CHECK(ScriptSystem_RunString(&sys, "ping 42 units"));
  CHECK(g_ping_calls == 1);
  CHECK(strcmp(g_ping_args, "42 units") == 0);

  /* Shutdown clears commands and variables; a second Init restores builtins. */
  ScriptSystem_Shutdown(&sys);
  CHECK(!ScriptSystem_RunString(&sys, "help"));
  CHECK(ScriptSystem_GetVar("speed", -1.0f) == -1.0f);
  CHECK(ScriptSystem_Init(&sys));
  CHECK(run_captured(&sys, "print back", out, sizeof out));
  CHECK(strcmp(out, "back\n") == 0);

  if (g_failures != 0) {
    fprintf(stderr, "script_system: %d check(s) failed\n", g_failures);
    return 1;
  }
  printf("script_system: all checks passed\n");
  return 0;
}

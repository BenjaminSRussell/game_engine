// Source/Runtime/Core/Engine/Private/crash_handler.c
#include "crash_handler.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void show_error_dialog(const char *title, const char *message) {
  fprintf(stderr, "\n=== %s ===\n", title);
  fprintf(stderr, "%s\n", message);
  fprintf(stderr, "========================\n\n");
}

const char *engine_get_error_string(EngineError error) {
  switch (error) {
  case ENGINE_ERROR_NONE:
    return "No Error";
  case ENGINE_ERROR_INIT_FAILED:
    return "Initialization Failed";
  case ENGINE_ERROR_OUT_OF_MEMORY:
    return "Out of Memory";
  // ...
  default:
    return "Unknown Error";
  }
}

static CrashReport *generate_crash_report(EngineError error,
                                          const char *additional_info) {
  CrashReport *report = malloc(sizeof(CrashReport));
  if (!report)
    return NULL;
  memset(report, 0, sizeof(CrashReport));
  report->timestamp = time(NULL);
  report->engine_error = error;
  strncpy(report->error_message, engine_get_error_string(error),
          sizeof(report->error_message) - 1);
  // ...
  return report;
}

static bool save_crash_report(const CrashReport *report) {
  // ... impl from main.c
  return true;
}

static void crash_signal_handler(int sig) {
  const char *signal_name = "Unknown";
  // ... switch sig
  LOG_ERROR("Recieved signal %d", sig);

  // show dialog, save report
  exit(1);
}

void crash_handler_init(void) {
  signal(SIGSEGV, crash_signal_handler);
  signal(SIGABRT, crash_signal_handler);
  signal(SIGFPE, crash_signal_handler);
  signal(SIGILL, crash_signal_handler);
}

void handle_engine_error(EngineError error) {
  const char *error_string = engine_get_error_string(error);
  LOG_ERROR("Engine Error: %s", error_string);
  // show dialog etc
  exit(EXIT_FAILURE);
}

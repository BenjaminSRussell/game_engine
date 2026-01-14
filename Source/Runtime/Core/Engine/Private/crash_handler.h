#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <core/logger.h>
#include <time.h>

#include <core/engine.h>

typedef struct {
  time_t timestamp;
  char error_message[512];
  char stack_trace[4096];
  EngineError engine_error;
  unsigned int frame_count;
  double total_time;
} CrashReport;

void crash_handler_init(void);
void handle_engine_error(EngineError error);
const char *engine_get_error_string(EngineError error);

#endif // CRASH_HANDLER_H

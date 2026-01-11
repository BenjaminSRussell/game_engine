// Platform/Crash Reporting
#ifndef CRASH_REPORTER_H
#define CRASH_REPORTER_H

#include "include/common.h"

typedef struct {
  char app_version[32];
  char os_version[32];
  char gpu_info[128];
  void *stack_trace[32];
  u32 stack_depth;
  char error_message[256];
} CrashReport;

#ifdef __cplusplus
extern "C" {
#endif

void crash_reporter_init(void);
void crash_reporter_capture(const char *message);
void crash_reporter_submit(CrashReport *report);

#ifdef __cplusplus
}
#endif

#endif

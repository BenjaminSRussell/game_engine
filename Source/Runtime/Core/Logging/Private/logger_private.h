#ifndef LOGGER_PRIVATE_H
#define LOGGER_PRIVATE_H

#include "../Public/unified_logger.h"
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

// Internal constants
#define MAX_LOG_MESSAGE_SIZE 4096
#define MAX_LOG_FILES 10
#define DEFAULT_MAX_FILE_SIZE_MB 50
#define DEFAULT_MAX_BACKUP_FILES 5
#define THROTTLE_MAP_SIZE 256
#define LOG_BREADCRUMB_MAX 128 // Assuming this value based on usage context

// Forward declaration of internal types if not in public header
// (Assuming these were internal to .c file or opaque in public header)

typedef struct {
  char key[64];
  u32 count;
  u64 last_reset_time;
} ThrottleEntry;

typedef struct {
  LogEntry entries[LOG_BREADCRUMB_MAX];
  u32 head;
  u32 count;
  bool enabled;
} LogBreadcrumbTrail;

typedef struct {
  // Config
  LogConfig config;

  // File state
  FILE *log_file;
  size_t current_file_size;

  // Buffering
  char *log_buffer;
  size_t buffer_pos;
  u64 last_flush_time;

  // Breadcrumbs
  LogBreadcrumbTrail breadcrumbs;

  // Stats
  u64 total_messages_logged;
  u64 start_time_ms;
} Logger;

typedef struct {
  Logger *instance;
  ThrottleEntry throttle_map[THROTTLE_MAP_SIZE];
  char session_id[32];
  char engine_version[64];
  char build_info[128];
  bool initialized;
} LoggingState;

// Global state
extern LoggingState g_logging_state;
extern Logger *g_logger; // Using ptr as per original code

// Utility functions shared internally
u64 get_time_ms(void);
u32 hash_string(const char *str);
void format_timestamp(u64 timestamp_ms, char *buffer, size_t buffer_size);

// Platform specific (prototypes)
void logger_platform_init(void);
void logger_platform_shutdown(void);
void logger_platform_output(const char *message);

// Internal helper for rotation
void logger_rotate_file(void);
void logger_flush_buffer(void);
void logger_breadcrumb_add(const LogEntry *entry);

#endif // LOGGER_PRIVATE_H

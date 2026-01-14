#include "logger_private.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Forward decls
const char *get_level_string(LogLevel level);
const char *get_category_string(LogCategory category);
void output_to_console(LogLevel level, LogCategory category,
                       const LogEntry *entry);
void output_to_file(LogLevel level, LogCategory category,
                    const LogEntry *entry);
void output_to_debugger(LogLevel level, const LogEntry *entry);

// Global State
LoggingState g_logging_state = {0};
Logger *g_logger = NULL;

// ============================================================================
// PLATFORM API (Stubbed here, or move to logger_platform.c if large)
// ============================================================================

void logger_platform_init(void) {
#ifdef _WIN32
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  GetConsoleMode(hConsole, &mode);
  SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void logger_platform_shutdown(void) {
  // Platform cleanup
}

// ============================================================================
// CORE LOGGING API
// ============================================================================

bool logger_init(const LogConfig *config) {
  if (g_logging_state.initialized) {
    return true;
  }

  // Allocate logger instance
  g_logger = malloc(sizeof(Logger));
  if (!g_logger) {
    return false;
  }

  memset(g_logger, 0, sizeof(Logger));

  // Copy configuration or set defaults
  if (config) {
    g_logger->config = *config;
  } else {
    memset(&g_logger->config, 0, sizeof(LogConfig));
    g_logger->config.global_level = LOG_LEVEL_INFO;
    g_logger->config.active_channels = LOG_CHANNEL_CONSOLE | LOG_CHANNEL_FILE;
    g_logger->config.format = LOG_FORMAT_DEVELOPMENT;
    g_logger->config.use_colors = true;
    g_logger->config.show_timestamp = true;
    g_logger->config.show_level = true;
    g_logger->config.show_category = true;
    g_logger->config.flush_on_write = false;
    g_logger->config.enable_breadcrumbs = true;
    g_logger->config.thread_safe = true;
    g_logger->config.rotation.max_file_size_mb = DEFAULT_MAX_FILE_SIZE_MB;
    g_logger->config.rotation.max_backup_files = DEFAULT_MAX_BACKUP_FILES;
    strcpy(g_logger->config.log_filename, "voxelforge.log");

    // Enable all categories
    for (int i = 0; i < LOG_CAT_COUNT; i++) {
      g_logger->config.category_filters[i].min_level = LOG_LEVEL_TRACE;
      g_logger->config.category_filters[i].enabled = true;
      g_logger->config.category_filters[i].channels =
          g_logger->config.active_channels;
    }
  }

  logger_platform_init();

  // Open log file
  if (g_logger->config.active_channels & LOG_CHANNEL_FILE) {
    g_logger->log_file = fopen(g_logger->config.log_filename, "a");
    if (g_logger->log_file) {
      fseek(g_logger->log_file, 0, SEEK_END);
      g_logger->current_file_size = ftell(g_logger->log_file);
    }
  }

  // Init buffering
  if (g_logger->config.enable_buffering) {
    u32 buffer_size = g_logger->config.buffer_size > 0
                          ? g_logger->config.buffer_size
                          : 64 * 1024;
    g_logger->log_buffer = malloc(buffer_size);
    g_logger->buffer_pos = 0;
  }

  // Init breadcrumbs
  if (g_logger->config.enable_breadcrumbs) {
    g_logger->breadcrumbs.enabled = true;
  }

  g_logger->start_time_ms = get_time_ms();

  // Init session info
  snprintf(g_logging_state.session_id, sizeof(g_logging_state.session_id),
           "%llx", (unsigned long long)g_logger->start_time_ms);

  g_logging_state.instance = g_logger;
  g_logging_state.initialized = true;

  return true;
}

void logger_shutdown(void) {
  if (!g_logging_state.initialized || !g_logger)
    return;

  if (g_logger->log_buffer) {
    logger_flush_buffer();
    free(g_logger->log_buffer);
  }

  if (g_logger->log_file) {
    fclose(g_logger->log_file);
    g_logger->log_file = NULL;
  }

  logger_platform_shutdown();

  free(g_logger);
  g_logger = NULL;
  g_logging_state.instance = NULL;
  g_logging_state.initialized = false;
}

bool logger_is_initialized(void) { return g_logging_state.initialized; }

// Breadcrumbs
void logger_breadcrumb_add(const LogEntry *entry) {
  if (!g_logger || !g_logger->breadcrumbs.enabled)
    return;
  LogBreadcrumbTrail *trail = &g_logger->breadcrumbs;
  trail->entries[trail->head] = *entry;
  trail->head = (trail->head + 1) % LOG_BREADCRUMB_MAX;
  if (trail->count < LOG_BREADCRUMB_MAX)
    trail->count++;
}

void logger_log(LogLevel level, LogCategory category, const char *file,
                int line, const char *function, const char *format, ...) {
  if (!g_logger || level < g_logger->config.global_level)
    return;

  if (category < LOG_CAT_COUNT) {
    const LogCategoryFilter *filter =
        &g_logger->config.category_filters[category];
    if (!filter->enabled || level < filter->min_level)
      return;
  }

  LogEntry entry = {0};
  entry.level = level;
  entry.category = category;
  entry.timestamp_ms = get_time_ms();
  // entry.thread_id = get_thread_id(); // Need to implement get_thread_id if we
  // want it
  entry.file = file;
  entry.line = line;
  entry.function = function;

  va_list args;
  va_start(args, format);
  vsnprintf(entry.message, sizeof(entry.message), format, args);
  va_end(args);

  if (g_logger->config.enable_breadcrumbs) {
    logger_breadcrumb_add(&entry);
  }

  output_to_console(level, category, &entry);
  output_to_file(level, category, &entry);
  output_to_debugger(level, &entry);

  g_logger->total_messages_logged++;
}

// File Rotation Helper
void logger_rotate_file(void) {
  if (!g_logger || !g_logger->log_file)
    return;
  fclose(g_logger->log_file);

  char backup_filename[512];
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);

  snprintf(backup_filename, sizeof(backup_filename),
           "%s.%04d%02d%02d_%02d%02d%02d", g_logger->config.log_filename,
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

  rename(g_logger->config.log_filename, backup_filename);

  g_logger->log_file = fopen(g_logger->config.log_filename, "w");
  if (g_logger->log_file) {
    g_logger->current_file_size = 0;
  }
}

void logger_flush_buffer(void) {
  if (!g_logger || !g_logger->log_buffer || g_logger->buffer_pos == 0)
    return;
  if (g_logger->log_file) {
    fwrite(g_logger->log_buffer, 1, g_logger->buffer_pos, g_logger->log_file);
    fflush(g_logger->log_file);
  }
  g_logger->buffer_pos = 0;
  g_logger->last_flush_time = get_time_ms();
}

// ============================================================================
// EXTRAS: Hex Dump & Config
// ============================================================================

void logger_log_hex(LogLevel level, LogCategory category, const void *data,
                    size_t length, const char *label) {
  if (!g_logger || !data || length == 0)
    return;
  const u8 *bytes = (const u8 *)data;
  char hex_line[128];

  logger_log(level, category, "hex", 0, "hex", "Hex dump: %s (%zu bytes)",
             label ? label : "Data", length);

  for (size_t i = 0; i < length; i += 16) {
    size_t line_length = (length - i < 16) ? (length - i) : 16;
    int offset = 0;
    offset +=
        snprintf(hex_line + offset, sizeof(hex_line) - offset, "%04zx: ", i);
    for (size_t j = 0; j < line_length; j++) {
      offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "%02x ",
                         bytes[i + j]);
    }
    for (size_t j = line_length; j < 16; j++) {
      offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "   ");
    }
    offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, " ");
    for (size_t j = 0; j < line_length; j++) {
      char c = bytes[i + j];
      offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "%c",
                         (c >= 32 && c <= 126) ? c : '.');
    }
    logger_log(level, category, "hex", 0, "hex", "%s", hex_line);
  }
}

void logger_set_level(LogLevel level) {
  if (g_logger)
    g_logger->config.global_level = level;
}

void logger_get_config(LogConfig *out_config) {
  if (g_logger && out_config)
    *out_config = g_logger->config;
}

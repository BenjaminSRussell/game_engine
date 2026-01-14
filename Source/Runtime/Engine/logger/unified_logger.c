#include "core/logger/unified_logger.h"
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

// ============================================================================
// UNIFIED LOGGER IMPLEMENTATION - CONSOLIDATING ALL BACKENDS
// ============================================================================

// Global logger state
static struct {
  LoggerConfig config;
  LoggerStats stats;
  FILE *log_file;
  pthread_mutex_t mutex;
  LogEntry *buffer;
  u32 buffer_size;
  u32 buffer_pos;
  bool initialized;
} g_unified_logger = {0};

// Internal helper functions
static const char *level_strings[] = {"TRACE", "DEBUG", "INFO",
                                      "WARN",  "ERROR", "FATAL"};

static const char *level_colors[] = {
    "\x1b[90m", // Gray for TRACE
    "\x1b[36m", // Cyan for DEBUG
    "\x1b[32m", // Green for INFO
    "\x1b[33m", // Yellow for WARN
    "\x1b[31m", // Red for ERROR
    "\x1b[35m"  // Magenta for FATAL
};

static const char *category_strings[] = {
    "GENERAL", "GAME",  "PHYSICS",   "GRAPHICS", "RENDERER", "AI",
    "NETWORK", "AUDIO", "MEMORY",    "IO",       "SCRIPT",   "PLATFORM",
    "EDITOR",  "ASSET", "ANIMATION", "INPUT",    "UI"};

static const char *reset_color = "\x1b[0m";

// ============================================================================
// CORE LOGGER FUNCTIONS
// ============================================================================

bool unified_logger_init(const LoggerConfig *config) {
  if (g_unified_logger.initialized) {
    return true; // Already initialized
  }

  if (!config) {
    return false;
  }

  // Initialize mutex
  if (pthread_mutex_init(&g_unified_logger.mutex, NULL) != 0) {
    return false;
  }

  // Copy configuration
  g_unified_logger.config = *config;

  // Initialize statistics
  memset(&g_unified_logger.stats, 0, sizeof(LoggerStats));
  g_unified_logger.stats.start_time = time(NULL);

  // Open log file if file channel is enabled
  if (config->enabled_channels & LOG_CHANNEL_FILE) {
    g_unified_logger.log_file = fopen(config->log_file_path, "a");
    if (!g_unified_logger.log_file) {
      pthread_mutex_destroy(&g_unified_logger.mutex);
      return false;
    }
  }

  // Initialize buffer if buffering is enabled
  if (config->use_buffering) {
    g_unified_logger.buffer_size = 1024; // Default buffer size
    g_unified_logger.buffer =
        malloc(g_unified_logger.buffer_size * sizeof(LogEntry));
    if (!g_unified_logger.buffer) {
      if (g_unified_logger.log_file) {
        fclose(g_unified_logger.log_file);
      }
      pthread_mutex_destroy(&g_unified_logger.mutex);
      return false;
    }
    g_unified_logger.buffer_pos = 0;
  }

  g_unified_logger.initialized = true;
  return true;
}

void unified_logger_shutdown(void) {
  if (!g_unified_logger.initialized) {
    return;
  }

  pthread_mutex_lock(&g_unified_logger.mutex);

  // Flush any remaining buffer entries
  if (g_unified_logger.config.use_buffering &&
      g_unified_logger.buffer_pos > 0) {
    unified_logger_flush();
  }

  // Close log file
  if (g_unified_logger.log_file) {
    fclose(g_unified_logger.log_file);
    g_unified_logger.log_file = NULL;
  }

  // Free buffer
  if (g_unified_logger.buffer) {
    free(g_unified_logger.buffer);
    g_unified_logger.buffer = NULL;
  }

  g_unified_logger.initialized = false;
  pthread_mutex_unlock(&g_unified_logger.mutex);
  pthread_mutex_destroy(&g_unified_logger.mutex);
}

void unified_logger_flush(void) {
  if (!g_unified_logger.initialized) {
    return;
  }

  pthread_mutex_lock(&g_unified_logger.mutex);

  // Write buffered entries to file
  if (g_unified_logger.buffer && g_unified_logger.log_file) {
    for (u32 i = 0; i < g_unified_logger.buffer_pos; i++) {
      const LogEntry *entry = &g_unified_logger.buffer[i];

      // Format timestamp
      struct tm *tm_info = localtime(&entry->timestamp);
      char timestamp_str[64];
      strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S",
               tm_info);

      // Write formatted log entry
      if (g_unified_logger.config.use_colors) {
        fprintf(g_unified_logger.log_file,
                "%s[%s] [%s] [%s] %s:%d %s(): %s%s\n",
                level_colors[entry->level], timestamp_str,
                level_strings[entry->level], category_strings[entry->category],
                entry->file, entry->line, entry->function, entry->message,
                reset_color);
      } else {
        fprintf(g_unified_logger.log_file, "[%s] [%s] [%s] %s:%d %s(): %s\n",
                timestamp_str, level_strings[entry->level],
                category_strings[entry->category], entry->file, entry->line,
                entry->function, entry->message);
      }
    }
    fflush(g_unified_logger.log_file);
    g_unified_logger.buffer_pos = 0;
  }

  pthread_mutex_unlock(&g_unified_logger.mutex);
}

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

void unified_logger_set_level(LogLevel level) {
  if (!g_unified_logger.initialized)
    return;

  pthread_mutex_lock(&g_unified_logger.mutex);
  g_unified_logger.config.min_level = level;
  pthread_mutex_unlock(&g_unified_logger.mutex);
}

void unified_logger_set_channels(LogChannel channels) {
  if (!g_unified_logger.initialized)
    return;

  pthread_mutex_lock(&g_unified_logger.mutex);
  g_unified_logger.config.enabled_channels = channels;
  pthread_mutex_unlock(&g_unified_logger.mutex);
}

void unified_logger_enable_category(LogCategory category, bool enabled) {
  if (!g_unified_logger.initialized || category >= LOG_CAT_COUNT)
    return;

  pthread_mutex_lock(&g_unified_logger.mutex);
  g_unified_logger.config.enabled_categories[category] = enabled;
  pthread_mutex_unlock(&g_unified_logger.mutex);
}

// ============================================================================
// CORE LOGGING FUNCTIONS
// ============================================================================

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  if (!g_unified_logger.initialized)
    return;

  // Check if level is enabled
  if (level < g_unified_logger.config.min_level)
    return;

  // Check if category is enabled
  if (!g_unified_logger.config.enabled_categories[category])
    return;

  pthread_mutex_lock(&g_unified_logger.mutex);

  // Create log entry
  LogEntry entry = {0};
  entry.level = level;
  entry.category = category;
  entry.channels = g_unified_logger.config.enabled_channels;
  entry.timestamp = time(NULL);
  entry.file = file;
  entry.line = line;
  entry.function = function;

  // Format message
  va_list args;
  va_start(args, format);
  vsnprintf(entry.message, sizeof(entry.message), format, args);
  va_end(args);

  // Update statistics
  g_unified_logger.stats.total_entries++;
  g_unified_logger.stats.entries_per_level[level]++;
  g_unified_logger.stats.entries_per_category[category]++;
  g_unified_logger.stats.last_entry_time = entry.timestamp;

  // Output to console if enabled
  if (g_unified_logger.config.enabled_channels & LOG_CHANNEL_CONSOLE) {
    struct tm *tm_info = localtime(&entry.timestamp);
    char timestamp_str[64];
    strftime(timestamp_str, sizeof(timestamp_str), "%H:%M:%S", tm_info);

    if (g_unified_logger.config.use_colors) {
      printf("%s[%s] [%s] %s %s%s\n", level_colors[level], timestamp_str,
             level_strings[level], category_strings[category], entry.message,
             reset_color);
    } else {
      printf("[%s] [%s] [%s] %s\n", timestamp_str, level_strings[level],
             category_strings[category], entry.message);
    }
  }

  // Output to file if enabled
  if (g_unified_logger.config.enabled_channels & LOG_CHANNEL_FILE) {
    if (g_unified_logger.config.use_buffering) {
      // Add to buffer
      if (g_unified_logger.buffer_pos < g_unified_logger.buffer_size) {
        g_unified_logger.buffer[g_unified_logger.buffer_pos++] = entry;

        // Flush buffer if it's full
        if (g_unified_logger.buffer_pos >= g_unified_logger.buffer_size) {
          unified_logger_flush();
        }
      }
    } else {
      // Write directly to file
      if (g_unified_logger.log_file) {
        struct tm *tm_info = localtime(&entry.timestamp);
        char timestamp_str[64];
        strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S",
                 tm_info);

        fprintf(g_unified_logger.log_file, "[%s] [%s] %s:%d %s(): %s\n",
                timestamp_str, category_strings[category], entry.file,
                entry.line, entry.function, entry.message);
        fflush(g_unified_logger.log_file);
        g_unified_logger.stats.bytes_written +=
            strlen(entry.message) + 100; // Approximate
      }
    }
  }

  // Send to remote if enabled
  if (g_unified_logger.config.enable_remote &&
      (g_unified_logger.config.enabled_channels & LOG_CHANNEL_NETWORK)) {
    unified_logger_send_to_remote(&entry);
  }

  pthread_mutex_unlock(&g_unified_logger.mutex);

  // Handle fatal errors
  if (level == LOG_LEVEL_FATAL) {
    // Print stack trace
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    printf("Stack trace:\n");
    for (i = 0; i < size; i++) {
      printf("%s\n", strings[i]);
    }
    free(strings);

    exit(EXIT_FAILURE);
  }
}

void unified_logger_log_structured(LogLevel level, LogCategory category,
                                   const char *file, int line,
                                   const char *function, const char *message,
                                   const char *context_json) {
  if (!g_unified_logger.initialized)
    return;

  // Check if level is enabled
  if (level < g_unified_logger.config.min_level)
    return;

  // Check if category is enabled
  if (!g_unified_logger.config.enabled_categories[category])
    return;

  pthread_mutex_lock(&g_unified_logger.mutex);

  // Create log entry with structured context
  LogEntry entry = {0};
  entry.level = level;
  entry.category = category;
  entry.channels = g_unified_logger.config.enabled_channels;
  entry.timestamp = time(NULL);
  entry.file = file;
  entry.line = line;
  entry.function = function;

  // Format message with JSON context
  if (context_json && strlen(context_json) > 0) {
    snprintf(entry.message, sizeof(entry.message), "%s | Context: %s", message,
             context_json);
    strncpy(entry.context_json, context_json, sizeof(entry.context_json) - 1);
  } else {
    strncpy(entry.message, message, sizeof(entry.message) - 1);
  }

  // Update statistics
  g_unified_logger.stats.total_entries++;
  g_unified_logger.stats.entries_per_level[level]++;
  g_unified_logger.stats.entries_per_category[category]++;

  // Output to console with JSON formatting
  if (g_unified_logger.config.enabled_channels & LOG_CHANNEL_CONSOLE) {
    struct tm *tm_info = localtime(&entry.timestamp);
    char timestamp_str[64];
    strftime(timestamp_str, sizeof(timestamp_str), "%H:%M:%S", tm_info);

    if (g_unified_logger.config.use_colors) {
      printf("%s[%s] [%s] %s %s%s\n", level_colors[level], timestamp_str,
             level_strings[level], category_strings[category], entry.message,
             reset_color);
    } else {
      printf("[%s] [%s] [%s] %s\n", timestamp_str, level_strings[level],
             category_strings[category], entry.message);
    }
  }

  pthread_mutex_unlock(&g_unified_logger.mutex);
}

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

void unified_logger_rotate(void) {
  if (!g_unified_logger.initialized || !g_unified_logger.log_file)
    return;

  pthread_mutex_lock(&g_unified_logger.mutex);

  // Close current log file
  fclose(g_unified_logger.log_file);

  // Create backup filename
  char backup_path[512];
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  char timestamp_str[64];
  strftime(timestamp_str, sizeof(timestamp_str), ".%Y%m%d_%H%M%S", tm_info);
  snprintf(backup_path, sizeof(backup_path), "%s%s",
           g_unified_logger.config.log_file_path, timestamp_str);

  // Rename current log file
  rename(g_unified_logger.config.log_file_path, backup_path);

  // Open new log file
  g_unified_logger.log_file = fopen(g_unified_logger.config.log_file_path, "w");

  // Compress old log file if enabled
  if (g_unified_logger.config.enable_compression) {
    unified_logger_compress_old_logs();
  }

  g_unified_logger.stats.files_rotated++;

  pthread_mutex_unlock(&g_unified_logger.mutex);
}

void unified_logger_compress_old_logs(void) {
  // Implementation for compressing old log files using gzip
  // This would iterate through backup files and compress them
  // For now, just increment the compression counter
  g_unified_logger.stats.compression_operations++;
}

void unified_logger_send_to_remote(const LogEntry *entry) {
  if (!entry || !g_unified_logger.config.enable_remote)
    return;

  // Simple HTTP POST implementation would go here
  // For now, just increment the network counter
  g_unified_logger.stats.network_transmissions++;
}

LoggerStats unified_logger_get_stats(void) { return g_unified_logger.stats; }

void unified_logger_reset_stats(void) {
  pthread_mutex_lock(&g_unified_logger.mutex);
  memset(&g_unified_logger.stats, 0, sizeof(LoggerStats));
  g_unified_logger.stats.start_time = time(NULL);
  pthread_mutex_unlock(&g_unified_logger.mutex);
}

// ============================================================================
// SEARCH AND QUERY FUNCTIONALITY
// ============================================================================

LogQueryResult unified_logger_search_by_level(LogLevel level) {
  // Implementation would search through log entries by level
  // For now, return empty result
  LogQueryResult result = {0};
  return result;
}

LogQueryResult unified_logger_search_by_category(LogCategory category) {
  // Implementation would search through log entries by category
  // For now, return empty result
  LogQueryResult result = {0};
  return result;
}

LogQueryResult unified_logger_search_by_time_range(time_t start, time_t end) {
  // Implementation would search through log entries by time range
  // For now, return empty result
  LogQueryResult result = {0};
  return result;
}

LogQueryResult unified_logger_search_by_text(const char *pattern) {
  // Implementation would search through log entries by text pattern
  // For now, return empty result
  LogQueryResult result = {0};
  return result;
}

void unified_logger_free_query_result(LogQueryResult *result) {
  if (result && result->entries) {
    free(result->entries);
    result->entries = NULL;
    result->count = 0;
    result->capacity = 0;
  }
}

// ============================================================================
// BENCHMARK FUNCTION
// ============================================================================

void unified_logger_benchmark(void) {
  printf("=== Unified Logger Benchmark ===\n");
  printf("Total Entries: %llu\n",
         (unsigned long long)g_unified_logger.stats.total_entries);
  printf("Bytes Written: %llu\n",
         (unsigned long long)g_unified_logger.stats.bytes_written);
  printf("Files Rotated: %llu\n",
         (unsigned long long)g_unified_logger.stats.files_rotated);
  printf("Network Transmissions: %llu\n",
         (unsigned long long)g_unified_logger.stats.network_transmissions);
  printf("Compression Operations: %llu\n",
         (unsigned long long)g_unified_logger.stats.compression_operations);

  if (g_unified_logger.stats.start_time > 0) {
    time_t uptime = time(NULL) - g_unified_logger.stats.start_time;
    printf("Uptime: %ld seconds\n", (long)uptime);
    if (uptime > 0) {
      printf("Average Log Rate: %.2f logs/second\n",
             (double)g_unified_logger.stats.total_entries / uptime);
    }
  }

  printf("=== End Benchmark ===\n");
}

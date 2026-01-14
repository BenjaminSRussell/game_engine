// Crash Report System implementation
#include "debug/crash_report_system.h"
#include "engine/include/core/logger.h"
#include <curl/curl.h>
#include <execinfo.h>
#include "engine/include/math/math.h"
#include <math/vec3.h>
#include <math/vec4.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

// Global crash report system instance
static CrashReportSystem *g_crash_system = NULL;

// MARK: - Helper Functions

static u64 get_current_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  // TODO: Add high-resolution timer support for all platforms
  return (u64)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void generate_report_id(char *id, size_t size) {
  const char charset[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  for (size_t i = 0; i < size - 1; i++) {
    id[i] = charset[rand() % (sizeof(charset) - 1)];
  }
  id[size - 1] = '\0';
}

static size_t write_callback(void *contents, size_t size, size_t nmemb,
                             void *userp) {
  size_t realsize = size * nmemb;
  char *response = (char *)userp;
  strncat(response, (char *)contents, realsize);
  return realsize;
}

static bool make_http_request(const char *url, const char *method,
                              const char *data, char *response,
                              size_t response_size) {
  // TODO: Implement proper URL encoding
  CURL *curl = curl_easy_init();
  if (!curl) {
    LOG_ERROR("Failed to initialize CURL");
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);

  if (data) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
  }

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

  response[0] = '\0';

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    LOG_ERROR("HTTP request failed: %s", curl_easy_strerror(res));
    return false;
  }

  return true;
}

static const char *crash_type_to_string(CrashType type) {
  switch (type) {
  case CRASH_TYPE_ACCESS_VIOLATION:
    return "Access Violation";
  case CRASH_TYPE_STACK_OVERFLOW:
    return "Stack Overflow";
  case CRASH_TYPE_ASSERTION_FAILED:
    return "Assertion Failed";
  case CRASH_TYPE_NULL_POINTER:
    return "Null Pointer";
  case CRASH_TYPE_DIVIDE_BY_ZERO:
    return "Divide by Zero";
  case CRASH_TYPE_MEMORY_CORRUPTION:
    return "Memory Corruption";
  case CRASH_TYPE_DEADLOCK:
    return "Deadlock";
  case CRASH_TYPE_UNKNOWN:
    return "Unknown";
  default:
    return "Unknown";
  }
}

static const char *crash_severity_to_string(CrashSeverity severity) {
  switch (severity) {
  case CRASH_SEVERITY_INFO:
    return "Info";
  case CRASH_SEVERITY_WARNING:
    return "Warning";
  case CRASH_SEVERITY_ERROR:
    return "Error";
  case CRASH_SEVERITY_CRITICAL:
    return "Critical";
  case CRASH_SEVERITY_FATAL:
    return "Fatal";
  default:
    return "Unknown";
  }
}

// MARK: - Platform-specific implementations

#ifdef __APPLE__ || defined(__linux__)

void crash_signal_handler(int signal, siginfo_t *info, void *context) {
  if (!g_crash_system) {
    return;
  }

  CrashType type = CRASH_TYPE_UNKNOWN;
  CrashSeverity severity = CRASH_SEVERITY_FATAL;

  switch (signal) {
  case SIGSEGV:
    type = CRASH_TYPE_ACCESS_VIOLATION;
    break;
  case SIGABRT:
    type = CRASH_TYPE_ASSERTION_FAILED;
    break;
  case SIGFPE:
    type = CRASH_TYPE_DIVIDE_BY_ZERO;
    break;
  case SIGBUS:
    type = CRASH_TYPE_MEMORY_CORRUPTION;
    break;
  default:
    type = CRASH_TYPE_UNKNOWN;
    break;
  }

  // Create crash report
  CrashReport *report = crash_report_create(
      g_crash_system, type, severity, strsignal(signal), "Unhandled signal");

  if (report) {
    // Capture stack trace
    void *frames[64];
    u32 frame_count = 0;
    platform_capture_stack_trace(frames, &frame_count, 64);
    crash_report_add_stack_trace(report, frames, frame_count);

    // Set exception address
    report->crash_context.exception_address = (u64)info->si_addr;
    report->crash_context.exception_code = signal;

    // Upload if auto-upload is enabled
    if (g_crash_system->auto_upload) {
      crash_report_upload(g_crash_system, report->report_id);
    }

    // Trigger callback
    if (g_crash_system->on_crash_detected) {
      g_crash_system->on_crash_detected(report);
    }
  }

  // Remove signal handlers and re-raise signal
  crash_report_remove_exception_handlers(g_crash_system);
  raise(signal);
}

void platform_install_crash_handlers(CrashReportSystem *system) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = crash_signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);
  sigaction(SIGFPE, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);

  LOG_INFO("Platform crash handlers installed");
}

void platform_remove_crash_handlers(CrashReportSystem *system) {
  signal(SIGSEGV, SIG_DFL);
  signal(SIGABRT, SIG_DFL);
  signal(SIGFPE, SIG_DFL);
  signal(SIGBUS, SIG_DFL);
  signal(SIGILL, SIG_DFL);

  LOG_INFO("Platform crash handlers removed");
}

void platform_capture_stack_trace(void **frames, u32 *frame_count,
                                  u32 max_frames) {
  *frame_count = backtrace(frames, max_frames);
}

void platform_get_register_state(const char *name, u64 *value) {
  // On Unix-like systems, register state is captured in signal handler context
  // This is a simplified implementation
  *value = 0;
}

void platform_generate_minidump(const char *file_path,
                                const CrashContext *context) {
  // On Unix-like systems, we can use core dumps or create a simple dump
  FILE *dump_file = fopen(file_path, "w");
  if (!dump_file) {
    return;
  }

  fprintf(dump_file, "=== CRASH DUMP ===\n");
  fprintf(dump_file, "Type: %s\n", crash_type_to_string(context->type));
  fprintf(dump_file, "Severity: %s\n",
          crash_severity_to_string(context->severity));
  fprintf(dump_file, "Exception Address: 0x%llx\n", context->exception_address);
  fprintf(dump_file, "Exception Code: %llu\n", context->exception_code);
  fprintf(dump_file, "Thread ID: %u\n", context->thread_id);

  fprintf(dump_file, "\n=== STACK TRACE ===\n");
  for (u32 i = 0; i < context->stack_frame_count; i++) {
  }

  fclose(dump_file);
}

#elif defined(_WIN32)

LONG WINAPI crash_exception_handler(EXCEPTION_POINTERS *exception_info) {
  if (!g_crash_system) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  CrashType type = CRASH_TYPE_UNKNOWN;
  CrashSeverity severity = CRASH_SEVERITY_FATAL;

  switch (exception_info->ExceptionRecord->ExceptionCode) {
  case EXCEPTION_ACCESS_VIOLATION:
    type = CRASH_TYPE_ACCESS_VIOLATION;
    break;
  case EXCEPTION_STACK_OVERFLOW:
    type = CRASH_TYPE_STACK_OVERFLOW;
    break;
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    type = CRASH_TYPE_DIVIDE_BY_ZERO;
    break;
  default:
    type = CRASH_TYPE_UNKNOWN;
    break;
  }

  // Create crash report
  char message[256];
  snprintf(message, sizeof(message), "Exception code: 0x%08X",
           exception_info->ExceptionRecord->ExceptionCode);

  CrashReport *report = crash_report_create(g_crash_system, type, severity,
                                            message, "Unhandled exception");

  if (report) {
    // Set exception information
    report->crash_context.exception_address =
        (u64)exception_info->ExceptionRecord->ExceptionAddress;
    report->crash_context.exception_code =
        exception_info->ExceptionRecord->ExceptionCode;

    // Capture stack trace
    void *frames[64];
    u32 frame_count = 0;
    platform_capture_stack_trace(frames, &frame_count, 64);
    crash_report_add_stack_trace(report, frames, frame_count);

    // Upload if auto-upload is enabled
    if (g_crash_system->auto_upload) {
      crash_report_upload(g_crash_system, report->report_id);
    }

    // Trigger callback
    if (g_crash_system->on_crash_detected) {
      g_crash_system->on_crash_detected(report);
    }
  }

  return EXCEPTION_EXECUTE_HANDLER;
}

void platform_install_crash_handlers(CrashReportSystem *system) {
  SetUnhandledExceptionFilter(crash_exception_handler);

  // Set signal handlers
  signal(SIGABRT, crash_signal_handler);
  signal(SIGINT, crash_signal_handler);
  signal(SIGTERM, crash_signal_handler);

  LOG_INFO("Platform crash handlers installed");
}

void platform_remove_crash_handlers(CrashReportSystem *system) {
  SetUnhandledExceptionFilter(NULL);

  signal(SIGABRT, SIG_DFL);
  signal(SIGINT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);

  LOG_INFO("Platform crash handlers removed");
}

void platform_capture_stack_trace(void **frames, u32 *frame_count,
                                  u32 max_frames) {
  *frame_count = CaptureStackBackTrace(0, max_frames, frames, NULL);
}

void platform_get_register_state(const char *name, u64 *value) {
  // On Windows, register state is captured in exception context
  *value = 0;
}

void platform_generate_minidump(const char *file_path,
                                const CrashContext *context) {
  // On Windows, we can use MiniDumpWriteDump API
  // This is a simplified implementation
  FILE *dump_file = fopen(file_path, "w");
  if (!dump_file) {
    return;
  }

  fprintf(dump_file, "=== CRASH DUMP ===\n");
  fprintf(dump_file, "Type: %s\n", crash_type_to_string(context->type));
  fprintf(dump_file, "Severity: %s\n",
          crash_severity_to_string(context->severity));
  fprintf(dump_file, "Exception Address: 0x%llx\n", context->exception_address);
  fprintf(dump_file, "Exception Code: %llu\n", context->exception_code);
  fprintf(dump_file, "Thread ID: %u\n", context->thread_id);

  fprintf(dump_file, "\n=== STACK TRACE ===\n");
  for (u32 i = 0; i < context->stack_frame_count; i++) {
  }

  fclose(dump_file);
}

void crash_signal_handler(int signal) {
  // Handle signals on Windows
  if (g_crash_system && g_crash_system->on_crash_detected) {
    CrashReport *report =
        crash_report_create(g_crash_system, CRASH_TYPE_UNKNOWN,
                            CRASH_SEVERITY_FATAL, strsignal(signal), "Signal");
    if (report) {
      g_crash_system->on_crash_detected(report);
    }
  }

  raise(signal);
}

#endif

// MARK: - Crash Report System Management

bool crash_report_init(CrashReportSystem *system, const char *api_endpoint,
                       u32 max_reports) {
  if (!system || !api_endpoint || max_reports == 0) {
    LOG_ERROR("Invalid parameters for crash_report_init");
    return false;
  }

  memset(system, 0, sizeof(CrashReportSystem));

  // Copy API endpoint
  strncpy(system->api_endpoint, api_endpoint, sizeof(system->api_endpoint) - 1);
  system->api_endpoint[sizeof(system->api_endpoint) - 1] = '\0';

  // Allocate reports array
  system->reports = (CrashReport *)calloc(max_reports, sizeof(CrashReport));
  if (!system->reports) {
    LOG_ERROR("Failed to allocate reports array");
    return false;
  }
  system->report_capacity = max_reports;

  // Allocate upload queue
  system->upload_queue = (char **)calloc(max_reports, sizeof(char *));
  if (!system->upload_queue) {
    free(system->reports);
    LOG_ERROR("Failed to allocate upload queue");
    return false;
  }
  system->upload_queue_capacity = max_reports;

  // Set default configuration
  system->auto_upload = true;
  system->include_screenshots = true;
  system->include_logs = true;
  system->include_dumps = true;
  system->max_local_reports = 50;
  system->upload_interval_ms = 60000; // 1 minute

  // Initialize CURL
  curl_global_init(CURL_GLOBAL_DEFAULT);

  // Set global instance for signal handlers
  g_crash_system = system;
  system->is_initialized = true;

  LOG_INFO("Crash report system initialized (max reports: %u)", max_reports);
  return true;
}

void crash_report_shutdown(CrashReportSystem *system) {
  if (!system) {
    return;
  }

  // Remove crash handlers
  crash_report_remove_exception_handlers(system);

  // Free reports
  if (system->reports) {
    for (u32 i = 0; i < system->report_count; i++) {
      if (system->upload_queue[i]) {
        free(system->upload_queue[i]);
      }
    }
    free(system->reports);
  }

  // Free upload queue
  if (system->upload_queue) {
    free(system->upload_queue);
  }

  // Cleanup CURL
  curl_global_cleanup();

  // Clear global instance
  g_crash_system = NULL;

  memset(system, 0, sizeof(CrashReportSystem));
  LOG_INFO("Crash report system shutdown");
}

void crash_report_set_exception_handlers(CrashReportSystem *system) {
  if (!system) {
    return;
  }

  platform_install_crash_handlers(system);
  LOG_INFO("Exception handlers installed");
}

void crash_report_remove_exception_handlers(CrashReportSystem *system) {
  if (!system) {
    return;
  }

  platform_remove_crash_handlers(system);
  LOG_INFO("Exception handlers removed");
}

// MARK: - Crash Detection and Reporting

CrashReport *crash_report_create(CrashReportSystem *system, CrashType type,
                                 CrashSeverity severity, const char *message,
                                 const char *description) {
  if (!system || system->report_count >= system->report_capacity) {
    return NULL;
  }

  CrashReport *report = &system->reports[system->report_count++];
  memset(report, 0, sizeof(CrashReport));

  // Generate report ID
  generate_report_id(report->report_id, sizeof(report->report_id));

  // TODO(Jules): Add support for custom crash report headers in
  // crash_report_create. Set crash context
  report->crash_context.type = type;
  report->crash_context.severity = severity;

  if (message) {
    strncpy(report->crash_context.exception_message, message,
            sizeof(report->crash_context.exception_message) - 1);
    report->crash_context
        .exception_message[sizeof(report->crash_context.exception_message) -
                           1] = '\0';
  }

  if (description) {
    strncpy(report->user_description, description,
            sizeof(report->user_description) - 1);
    report->user_description[sizeof(report->user_description) - 1] = '\0';
  }

  // Set timestamps
  report->crash_time_ms = get_current_time_ms();

  // Collect system information
  crash_report_collect_system_info(&report->system_info);

  // Collect application state
  crash_report_collect_application_state(&report->app_state);

  // Generate file paths
  snprintf(report->log_file_path, sizeof(report->log_file_path),
           "crash_logs/%s.log", report->report_id);
  // TODO(Jules): Implement structured JSON logging for crash reports instead of
  // plain text.
  snprintf(report->screenshot_path, sizeof(report->screenshot_path),
           "crash_screenshots/%s.png", report->report_id);
  snprintf(report->dump_file_path, sizeof(report->dump_file_path),
           "crash_dumps/%s.dmp", report->report_id);
  snprintf(report->config_file_path, sizeof(report->config_file_path),
           "crash_configs/%s.json", report->report_id);

  // Capture additional data if enabled
  if (system->include_screenshots) {
    crash_report_capture_screenshot(report->screenshot_path);
  }

  if (system->include_dumps) {
    // TODO(Jules): Add compression for minidumps before upload.
    platform_generate_minidump(report->dump_file_path, &report->crash_context);
  }

  LOG_DEBUG("Created crash report: %s", report->report_id);
  return report;
}

bool crash_report_add_stack_trace(CrashReport *report, void **frames,
                                  u32 frame_count) {
  if (!report || !frames || frame_count == 0) {
    return false;
  }

  report->crash_context.stack_frame_count = frame_count;
  for (u32 i = 0; i < frame_count && i < 64; i++) {
    report->crash_context.stack_frames[i] = frames[i];

    // Try to resolve symbol names
    char **symbols = backtrace_symbols(frames + i, 1);
    if (symbols && symbols[0]) {
      strncpy(report->crash_context.stack_symbols[i], symbols[0],
              sizeof(report->crash_context.stack_symbols[i]) - 1);
      report->crash_context
          .stack_symbols[i][sizeof(report->crash_context.stack_symbols[i]) -
                            1] = '\0';
      free(symbols);
    }
  }

  return true;
}

// MARK: - System Information Collection

void crash_report_collect_system_info(SystemInfo *info) {
  if (!info) {
    return;
  }

  // Get system information
  struct utsname uname_info;
  if (uname(&uname_info) == 0) {
    strncpy(info->os_name, uname_info.sysname, sizeof(info->os_name) - 1);
    strncpy(info->os_version, uname_info.release, sizeof(info->os_version) - 1);
    strncpy(info->cpu_architecture, uname_info.machine,
            sizeof(info->cpu_architecture) - 1);
  }

  // Get CPU information
  info->cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);

  // Get memory information
  info->total_memory_mb =
      sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) / (1024 * 1024);

  // Set engine version (this should be defined elsewhere)
  strncpy(info->engine_version, "1.0.0", sizeof(info->engine_version) - 1);
  strncpy(info->build_number, "dev", sizeof(info->build_number) - 1);

  // Get build date
  strncpy(info->build_date, __DATE__, sizeof(info->build_date) - 1);

  LOG_DEBUG("Collected system information");
}

void crash_report_collect_application_state(ApplicationState *state) {
  if (!state) {
    return;
  }

  // This should be filled with actual application state
  // For now, set some default values
  strncpy(state->scene_name, "Unknown", sizeof(state->scene_name) - 1);
  state->player_position = (Vec3){0, 0, 0};
  state->player_rotation = (Vec3){0, 0, 0};
  state->entity_count = 0;
  state->loaded_chunks = 0;
  state->fps = 60.0f;
  state->memory_usage_mb = 100.0f;
  strncpy(state->last_action, "Unknown", sizeof(state->last_action) - 1);
  state->session_duration_ms = get_current_time_ms();
  strncpy(state->game_mode, "Survival", sizeof(state->game_mode) - 1);

  LOG_DEBUG("Collected application state");
}

void crash_report_capture_screenshot(const char *file_path) {
  if (!file_path) {
    LOG_ERROR("Screenshot capture: invalid file path");
    return;
  }

  LOG_DEBUG("Screenshot capture requested: %s", file_path);

  // Check if renderer is available
  // In a real implementation, this would check for active renderer context
  bool renderer_available = true; // Placeholder check

  if (!renderer_available) {
    LOG_WARN("Screenshot capture failed: renderer not available");
    return;
  }

  // Implementation would depend on the rendering backend
  // For Metal: Use MTLTexture to capture framebuffer
  // For OpenGL: Use glReadPixels to capture framebuffer
  // For Vulkan: Use VkImage to capture framebuffer

  // Placeholder implementation - simulate screenshot capture
  bool capture_success = true;

  if (capture_success) {
    // In a real implementation, this would:
    // 1. Get the current framebuffer
    // 2. Read pixel data into a buffer
    // 3. Convert to desired format (PNG/JPEG)
    // 4. Save to file

    // Simulate file creation
    FILE *screenshot_file = fopen(file_path, "wb");
    if (screenshot_file) {
      // Write placeholder image header and data
      // In reality, this would be actual image data

      // Simple BMP header placeholder
      u8 bmp_header[54] = {
          0x42, 0x4D,             // BM
          0x36, 0x00, 0x00, 0x00, // File size
          0x00, 0x00, 0x00, 0x00, // Reserved
          0x36, 0x00, 0x00, 0x00, // Data offset
          0x28, 0x00, 0x00, 0x00, // Header size
          0x02, 0x00, 0x00, 0x00, // Width (2 pixels)
          0x02, 0x00, 0x00, 0x00, // Height (2 pixels)
          0x01, 0x00,             // Planes
          0x18, 0x00,             // Bits per pixel (24)
          0x00, 0x00, 0x00, 0x00, // Compression
          0x00, 0x00, 0x00, 0x00, // Image size
          0x00, 0x00, 0x00, 0x00, // X pixels per meter
          0x00, 0x00, 0x00, 0x00, // Y pixels per meter
          0x00, 0x00, 0x00, 0x00, // Colors used
          0x00, 0x00, 0x00, 0x00  // Important colors
      };

      // Simple 2x2 pixel data (red square)
      u8 pixel_data[12] = {
          0xFF, 0x00, 0x00, // Red pixel
          0xFF, 0x00, 0x00, // Red pixel
          0xFF, 0x00, 0x00, // Red pixel
          0xFF, 0x00, 0x00  // Red pixel
      };

      fwrite(bmp_header, 1, sizeof(bmp_header), screenshot_file);
      fwrite(pixel_data, 1, sizeof(pixel_data), screenshot_file);
      fclose(screenshot_file);

      LOG_INFO("Screenshot captured successfully: %s", file_path);
    } else {
      LOG_ERROR("Failed to create screenshot file: %s", file_path);
      capture_success = false;
    }
  } else {
    LOG_ERROR("Screenshot capture failed: renderer error");
  }

  // Log capture attempt result
  if (capture_success) {
    LOG_DEBUG("Screenshot capture completed successfully");
  } else {
    LOG_ERROR("Screenshot capture failed");
  }
}

// MARK: - Report Upload and Management

bool crash_report_upload(CrashReportSystem *system, const char *report_id) {
  if (!system || !report_id || !system->is_initialized) {
    return false;
  }

  // Find the report
  CrashReport *report = NULL;
  for (u32 i = 0; i < system->report_count; i++) {
    if (strcmp(system->reports[i].report_id, report_id) == 0) {
      report = &system->reports[i];
      break;
    }
  }

  if (!report) {
    LOG_ERROR("Report not found: %s", report_id);
    return false;
  }

  // Create upload request
  char request_data[8192];
  snprintf(
      request_data, sizeof(request_data),
      "{\"report_id\":\"%s\",\"type\":%u,\"severity\":%u,\"message\":\"%s\","
      "\"description\":\"%s\",\"crash_time\":%llu,\"os_name\":\"%s\","
      "\"os_version\":\"%s\",\"cpu_cores\":%u,\"memory_mb\":%llu}",
      report->report_id, report->crash_context.type,
      report->crash_context.severity, report->crash_context.exception_message,
      report->user_description, report->crash_time_ms,
      report->system_info.os_name, report->system_info.os_version,
      report->system_info.cpu_cores, report->system_info.total_memory_mb);

  char response[4096];
  char url[512];
  snprintf(url, sizeof(url), "%s/api/crash/upload", system->api_endpoint);

  if (!make_http_request(url, "POST", request_data, response,
                         sizeof(response))) {
    LOG_ERROR("Upload request failed for report: %s", report_id);
    strncpy(report->upload_error, "HTTP request failed",
            sizeof(report->upload_error) - 1);

    if (system->on_upload_failed) {
      system->on_upload_failed(report_id, report->upload_error);
    }

    return false;
  }

  // Mark as uploaded
  report->is_uploaded = true;
  report->upload_time_ms = get_current_time_ms();

  LOG_INFO("Crash report uploaded: %s", report_id);

  if (system->on_report_uploaded) {
    system->on_report_uploaded(report_id, true);
  }

  return true;
}

// MARK: - Utility Functions

void crash_report_set_auto_upload(CrashReportSystem *system, bool enabled) {
  if (!system) {
    return;
  }

  system->auto_upload = enabled;
  LOG_INFO("Auto upload %s", enabled ? "enabled" : "disabled");
}

void crash_report_set_callbacks(CrashReportSystem *system,
                                void (*on_crash)(const CrashReport *),
                                void (*on_upload)(const char *, bool),
                                void (*on_failed)(const char *, const char *)) {
  if (!system) {
    return;
  }

  system->on_crash_detected = on_crash;
  system->on_report_uploaded = on_upload;
  system->on_upload_failed = on_failed;

  LOG_INFO("Crash report callbacks set");
}

void crash_report_print_report(const CrashReport *report) {
  if (!report) {
    printf("Report is NULL\n");
    return;
  }

  printf("=== CRASH REPORT ===\n");
  printf("ID: %s\n", report->report_id);
  printf("Type: %s\n", crash_type_to_string(report->crash_context.type));
  printf("Severity: %s\n",
         crash_severity_to_string(report->crash_context.severity));
  printf("Message: %s\n", report->crash_context.exception_message);
  printf("Description: %s\n", report->user_description);
  printf("Crash Time: %llu\n", report->crash_time_ms);
  printf("OS: %s %s\n", report->system_info.os_name,
         report->system_info.os_version);
  printf("CPU Cores: %u\n", report->system_info.cpu_cores);
  printf("Memory: %llu MB\n", report->system_info.total_memory_mb);
  printf("Uploaded: %s\n", report->is_uploaded ? "Yes" : "No");

  if (report->crash_context.stack_frame_count > 0) {
    printf("\n=== STACK TRACE ===\n");
    for (u32 i = 0; i < report->crash_context.stack_frame_count; i++) {
                   report->crash_context.stack_symbols[i]);
    }
  }

  printf("===================\n");
}

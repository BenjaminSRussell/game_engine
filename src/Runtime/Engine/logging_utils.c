// Logging Utilities Implementation
// Centralized logging utilities to reduce code duplication

#include "logging_utils.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Context names
static const char *context_names[LOG_CONTEXT_COUNT] = {
    "CORE",
    "RENDERING",
    "PHYSICS", 
    "AUDIO",
    "NETWORK",
    "ANIMATION",
    "AI",
    "ASSETS",
    "EDITOR",
    "PLATFORM"
};

// Context settings
static LogLevel context_levels[LOG_CONTEXT_COUNT] = {
    LOG_LEVEL_DEBUG,  // CORE
    LOG_LEVEL_INFO,   // RENDERING
    LOG_LEVEL_INFO,   // PHYSICS
    LOG_LEVEL_INFO,   // AUDIO
    LOG_LEVEL_INFO,   // NETWORK
    LOG_LEVEL_DEBUG,  // ANIMATION
    LOG_LEVEL_DEBUG,  // AI
    LOG_LEVEL_INFO,   // ASSETS
    LOG_LEVEL_DEBUG,  // EDITOR
    LOG_LEVEL_INFO    // PLATFORM
};

static bool context_enabled[LOG_CONTEXT_COUNT] = {
    true, true, true, true, true, true, true, true, true, true
};

// Performance tracking
typedef struct {
    const char *name;
    u64 start_time;
    bool active;
} PerfEntry;

#define MAX_PERF_ENTRIES 64
static PerfEntry perf_entries[MAX_PERF_ENTRIES];
static u32 perf_entry_count = 0;
static u64 frame_start_time = 0;
static u32 frame_count = 0;

// Initialize logging utilities
void logging_utils_init(void) {
    memset(perf_entries, 0, sizeof(perf_entries));
    perf_entry_count = 0;
    frame_start_time = 0;
    frame_count = 0;
    
    LOG_INFO("Logging utilities initialized");
}

// Shutdown logging utilities
void logging_utils_shutdown(void) {
    logging_utils_print_stats();
    LOG_INFO("Logging utilities shutdown");
}

// Set logging level for specific context
void logging_utils_set_context_level(LogContext context, LogLevel level) {
    if (context >= LOG_CONTEXT_COUNT) return;
    
    context_levels[context] = level;
    LOG_DEBUG("Set %s context level to %d", context_names[context], level);
}

// Enable/disable specific context
void logging_utils_set_context_enabled(LogContext context, bool enabled) {
    if (context >= LOG_CONTEXT_COUNT) return;
    
    context_enabled[context] = enabled;
    LOG_DEBUG("Set %s context %s", context_names[context], enabled ? "enabled" : "disabled");
}

// Log with context
void logging_utils_log(LogLevel level, LogContext context, const char *file, int line, const char *format, ...) {
    if (context >= LOG_CONTEXT_COUNT || !context_enabled[context]) return;
    
    // Check if level is enabled for this context
    if (level < context_levels[context]) return;
    
    // Extract filename from path
    const char *filename = strrchr(file, '/');
    if (!filename) filename = file;
    else filename++;
    
    // Format the message
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Log with context prefix
    char formatted_message[1100];
    snprintf(formatted_message, sizeof(formatted_message), "[%s] %s (%s:%d)", 
             context_names[context], message, filename, line);
    
    // Use the existing logger
    switch (level) {
        case LOG_LEVEL_ERROR:
            LOG_ERROR("%s", formatted_message);
            break;
        case LOG_LEVEL_WARN:
            LOG_WARN("%s", formatted_message);
            break;
        case LOG_LEVEL_INFO:
            LOG_INFO("%s", formatted_message);
            break;
        case LOG_LEVEL_DEBUG:
            LOG_DEBUG("%s", formatted_message);
            break;
    }
}

// Get current time (platform-specific)
static u64 get_current_time(void) {
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (u64)(counter.QuadPart * 1000000 / frequency.QuadPart);
#elif defined(__APPLE__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
#endif
}

// Performance logging
void logging_utils_perf_start(const char *name) {
    if (perf_entry_count >= MAX_PERF_ENTRIES) return;
    
    PerfEntry *entry = &perf_entries[perf_entry_count++];
    entry->name = name;
    entry->start_time = get_current_time();
    entry->active = true;
}

void logging_utils_perf_end(const char *name) {
    u64 end_time = get_current_time();
    
    // Find matching entry
    for (u32 i = 0; i < perf_entry_count; i++) {
        PerfEntry *entry = &perf_entries[i];
        if (entry->active && strcmp(entry->name, name) == 0) {
            u64 duration = end_time - entry->start_time;
            LOG_DEBUG("PERF: %s took %.2f ms", name, duration / 1000.0f);
            entry->active = false;
            return;
        }
    }
    
    LOG_WARN("PERF: No matching start entry found for %s", name);
}

void logging_utils_perf_frame_start(void) {
    frame_start_time = get_current_time();
}

void logging_utils_perf_frame_end(void) {
    if (frame_start_time == 0) return;
    
    u64 end_time = get_current_time();
    u64 frame_time = end_time - frame_start_time;
    frame_count++;
    
    // Log frame time every 60 frames
    if (frame_count % 60 == 0) {
        LOG_DEBUG("PERF: Frame time: %.2f ms (%.1f FPS)", 
                 frame_time / 1000.0f, 1000000.0f / frame_time);
    }
    
    frame_start_time = 0;
}

// Get context name as string
const char *logging_utils_get_context_name(LogContext context) {
    if (context >= LOG_CONTEXT_COUNT) return "UNKNOWN";
    return context_names[context];
}

// Print logging statistics
void logging_utils_print_stats(void) {
    LOG_INFO("=== Logging Statistics ===");
    
    // Count active performance entries
    u32 active_perf_entries = 0;
    for (u32 i = 0; i < perf_entry_count; i++) {
        if (perf_entries[i].active) active_perf_entries++;
    }
    
    LOG_INFO("Performance entries: %u total, %u active", perf_entry_count, active_perf_entries);
    LOG_INFO("Frames logged: %u", frame_count);
    
    // Print context settings
    for (int i = 0; i < LOG_CONTEXT_COUNT; i++) {
        LOG_INFO("Context %s: level=%d, enabled=%s", 
                 context_names[i], 
                 context_levels[i],
                 context_enabled[i] ? "yes" : "no");
    }
    
    LOG_INFO("========================");
}

// Enable/disable file logging
void logging_utils_set_file_logging(bool enabled, const char *filename) {
    // This would integrate with the existing logger system
    LOG_INFO("File logging %s: %s", enabled ? "enabled" : "disabled", filename ? filename : "default");
}

// Flush all logs
void logging_utils_flush(void) {
    // This would integrate with the existing logger system
    LOG_DEBUG("Flushing all logs");
}

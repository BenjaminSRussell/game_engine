#ifndef CRASH_REPORT_SYSTEM_H
#define CRASH_REPORT_SYSTEM_H

#include <core/types.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct Vec3 Vec3;
typedef struct Vec4 Vec4;

// Crash severity levels
typedef enum {
    CRASH_SEVERITY_INFO,
    CRASH_SEVERITY_WARNING,
    CRASH_SEVERITY_ERROR,
    CRASH_SEVERITY_CRITICAL,
    CRASH_SEVERITY_FATAL
} CrashSeverity;

// Crash types
typedef enum {
    CRASH_TYPE_ACCESS_VIOLATION,
    CRASH_TYPE_STACK_OVERFLOW,
    CRASH_TYPE_ASSERTION_FAILED,
    CRASH_TYPE_NULL_POINTER,
    CRASH_TYPE_DIVIDE_BY_ZERO,
    CRASH_TYPE_MEMORY_CORRUPTION,
    CRASH_TYPE_DEADLOCK,
    CRASH_TYPE_UNKNOWN
} CrashType;

// System information
typedef struct {
    char os_name[64];
    char os_version[32];
    char cpu_architecture[32];
    char cpu_model[64];
    u32 cpu_cores;
    u64 total_memory_mb;
    char gpu_vendor[64];
    char gpu_model[64];
    char driver_version[32];
    char engine_version[32];
    char build_number[32];
    char build_date[32];
} SystemInfo;

// Crash context
typedef struct {
    CrashType type;
    CrashSeverity severity;
    char exception_message[256];
    u64 exception_address;
    u64 exception_code;
    
    // Stack trace
    void* stack_frames[64];
    u32 stack_frame_count;
    char stack_symbols[64][256];
    
    // Register state
    u64 registers[16];
    char register_names[16][16];
    
    // Thread information
    u32 thread_id;
    char thread_name[64];
    u64 thread_stack_base;
    u64 thread_stack_limit;
} CrashContext;

// Application state
typedef struct {
    char scene_name[128];
    Vec3 player_position;
    Vec3 player_rotation;
    u32 entity_count;
    u32 loaded_chunks;
    float fps;
    float memory_usage_mb;
    char last_action[128];
    u64 session_duration_ms;
    char game_mode[32];
    
    // Editor state if applicable
    bool is_editor_mode;
    char current_tool[64];
    char selected_asset[256];
} ApplicationState;

// Crash report
typedef struct {
    char report_id[64];
    SystemInfo system_info;
    CrashContext crash_context;
    ApplicationState app_state;
    
    // Report metadata
    u64 crash_time_ms;
    char user_description[512];
    char reproduction_steps[1024];
    bool is_user_reported;
    
    // Additional data
    char log_file_path[256];
    char screenshot_path[256];
    char dump_file_path[256];
    char config_file_path[256];
    
    // Upload status
    bool is_uploaded;
    char upload_url[512];
    u64 upload_time_ms;
    char upload_error[256];
} CrashReport;

// Crash report system
typedef struct {
    // Report storage
    CrashReport* reports;
    u32 report_count;
    u32 report_capacity;
    
    // System configuration
    char api_endpoint[256];
    char api_key[128];
    bool auto_upload;
    bool include_screenshots;
    bool include_logs;
    bool include_dumps;
    u32 max_local_reports;
    
    // Upload queue
    char** upload_queue;
    u32 upload_queue_count;
    u32 upload_queue_capacity;
    
    // System state
    bool is_initialized;
    bool is_crashing;
    u64 last_upload_time_ms;
    u32 upload_interval_ms;
    
    // Callbacks
    void (*on_crash_detected)(const CrashReport* report);
    void (*on_report_uploaded)(const char* report_id, bool success);
    void (*on_upload_failed)(const char* report_id, const char* error);
    
    void* user_data;
} CrashReportSystem;

// MARK: - Crash Report System Management

bool crash_report_init(CrashReportSystem* system, const char* api_endpoint, u32 max_reports);
void crash_report_shutdown(CrashReportSystem* system);

// MARK: - Crash Detection and Reporting

void crash_report_set_exception_handlers(CrashReportSystem* system);
void crash_report_remove_exception_handlers(CrashReportSystem* system);

CrashReport* crash_report_create(CrashReportSystem* system, CrashType type, CrashSeverity severity, 
                               const char* message, const char* description);
bool crash_report_add_stack_trace(CrashReport* report, void** frames, u32 frame_count);
bool crash_report_add_register_state(CrashReport* report, const char* name, u64 value);
bool crash_report_set_application_state(CrashReport* report, const ApplicationState* state);

// MARK: - Report Upload and Management

bool crash_report_upload(CrashReportSystem* system, const char* report_id);
bool crash_report_upload_all(CrashReportSystem* system);
bool crash_report_delete_report(CrashReportSystem* system, const char* report_id);
bool crash_report_clear_all_reports(CrashReportSystem* system);

// MARK: - System Information Collection

void crash_report_collect_system_info(SystemInfo* info);
void crash_report_collect_application_state(ApplicationState* state);
void crash_report_capture_screenshot(const char* file_path);
void crash_report_generate_minidump(const char* file_path);

// MARK: - Utility Functions

void crash_report_set_auto_upload(CrashReportSystem* system, bool enabled);
void crash_report_set_upload_interval(CrashReportSystem* system, u32 interval_ms);
void crash_report_set_max_local_reports(CrashReportSystem* system, u32 max_reports);

void crash_report_set_callbacks(CrashReportSystem* system,
                               void (*on_crash)(const CrashReport*),
                               void (*on_upload)(const char*, bool),
                               void (*on_failed)(const char*, const char*));

CrashReport* crash_report_get_report(CrashReportSystem* system, const char* report_id);
CrashReport* crash_report_get_latest_report(CrashReportSystem* system);
u32 crash_report_get_report_count(CrashReportSystem* system);

// MARK: - Report Export and Analysis

bool crash_report_export_to_json(CrashReportSystem* system, const char* report_id, const char* filename);
bool crash_report_export_all_to_json(CrashReportSystem* system, const char* filename);
bool crash_report_generate_summary(CrashReportSystem* system, const char* filename);

void crash_report_print_report(const CrashReport* report);
void crash_report_print_summary(CrashReportSystem* system);

// MARK: - Platform-specific implementations

void platform_install_crash_handlers(CrashReportSystem* system);
void platform_remove_crash_handlers(CrashReportSystem* system);
void platform_capture_stack_trace(void** frames, u32* frame_count, u32 max_frames);
void platform_get_register_state(const char* name, u64* value);
void platform_generate_minidump(const char* file_path, const CrashContext* context);

// MARK: - Crash Signal Handlers (platform-specific)

#ifdef _WIN32
LONG WINAPI crash_exception_handler(EXCEPTION_POINTERS* exception_info);
void crash_signal_handler(int signal);
#elif defined(__APPLE__) || defined(__linux__)
void crash_signal_handler(int signal, siginfo_t* info, void* context);
#endif

#endif // CRASH_REPORT_SYSTEM_H

/**
 * @file deployment_validator.h
 * @brief Comprehensive deployment validation system for Minecraft v2 Engine
 * @author Deployment Validation Team
 * @date 2025
 * 
 * This system provides comprehensive validation for all deployment checklist items
 * including code quality, performance metrics, and compatibility checks.
 */

#ifndef DEPLOYMENT_VALIDATOR_H
#define DEPLOYMENT_VALIDATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Validation result codes
typedef enum {
    VALIDATION_PASS = 0,
    VALIDATION_FAIL = 1,
    VALIDATION_WARNING = 2,
    VALIDATION_SKIP = 3,
    VALIDATION_ERROR = 4
} validation_result_t;

// Validation categories
typedef enum {
    CATEGORY_CODE_QUALITY = 0,
    CATEGORY_PERFORMANCE = 1,
    CATEGORY_COMPATIBILITY = 2,
    CATEGORY_SECURITY = 3,
    CATEGORY_DOCUMENTATION = 4
} validation_category_t;

// Validation severity levels
typedef enum {
    SEVERITY_CRITICAL = 0,
    SEVERITY_HIGH = 1,
    SEVERITY_MEDIUM = 2,
    SEVERITY_LOW = 3,
    SEVERITY_INFO = 4
} validation_severity_t;

// Individual validation check
typedef struct {
    uint32_t id;
    char name[256];
    char description[512];
    validation_category_t category;
    validation_severity_t severity;
    validation_result_t result;
    char error_message[1024];
    double execution_time_ms;
    time_t timestamp;
} validation_check_t;

// Performance metrics
typedef struct {
    double frame_time_ms;
    double frame_time_variance;
    uint64_t frame_spikes_20_percent;
    double memory_usage_mb;
    double memory_leak_rate_mb_per_min;
    double gpu_utilization_percent;
    double cpu_utilization_percent;
    uint64_t total_frames_analyzed;
    uint64_t analysis_duration_ms;
} performance_metrics_t;

// Code quality metrics
typedef struct {
    uint32_t todo_count;
    uint32_t fixme_count;
    uint32_t total_lines_of_code;
    uint32_t tested_error_conditions;
    uint32_t untested_error_conditions;
    uint32_t thread_safety_violations;
    uint32_t memory_safety_issues;
    uint32_t code_coverage_percent;
} code_quality_metrics_t;

// Compatibility check results
typedef struct {
    bool windows_vulkan_supported;
    bool macos_metal_supported;
    bool linux_vulkan_supported;
    char windows_version[64];
    char macos_version[64];
    char linux_distribution[64];
    char gpu_vendor[64];
    char gpu_model[128];
    uint32_t vulkan_version;
    uint32_t metal_version;
} compatibility_results_t;

// Main validation context
typedef struct {
    validation_check_t* checks;
    uint32_t check_count;
    uint32_t max_checks;
    
    performance_metrics_t performance;
    code_quality_metrics_t code_quality;
    compatibility_results_t compatibility;
    
    pthread_mutex_t mutex;
    bool is_running;
    time_t start_time;
    char report_path[512];
} deployment_validator_t;

// Validation function pointer type
typedef validation_result_t (*validation_func_t)(deployment_validator_t* validator);

// Main API functions
int deployment_validator_init(deployment_validator_t* validator, uint32_t max_checks);
void deployment_validator_shutdown(deployment_validator_t* validator);
int deployment_validator_add_check(deployment_validator_t* validator, 
                                  const char* name, 
                                  const char* description,
                                  validation_category_t category,
                                  validation_severity_t severity,
                                  validation_func_t func);

// Core validation functions
validation_result_t validate_no_todo_comments(deployment_validator_t* validator);
validation_result_t validate_error_conditions_tested(deployment_validator_t* validator);
validation_result_t validate_memory_usage_optimized(deployment_validator_t* validator);
validation_result_t validate_thread_safety_verified(deployment_validator_t* validator);
validation_result_t validate_frame_time_stability(deployment_validator_t* validator);
validation_result_t validate_memory_leak_detection(deployment_validator_t* validator);
validation_result_t validate_gpu_utilization(deployment_validator_t* validator);
validation_result_t validate_cpu_utilization(deployment_validator_t* validator);
validation_result_t validate_windows_compatibility(deployment_validator_t* validator);
validation_result_t validate_macos_compatibility(deployment_validator_t* validator);
validation_result_t validate_linux_compatibility(deployment_validator_t* validator);

// Execution functions
int deployment_validator_run_all_checks(deployment_validator_t* validator);
int deployment_validator_run_category(deployment_validator_t* validator, validation_category_t category);
validation_result_t deployment_validator_run_single_check(deployment_validator_t* validator, uint32_t check_id);

// Reporting functions
int deployment_validator_generate_report(deployment_validator_t* validator, const char* format);
int deployment_validator_save_report(deployment_validator_t* validator, const char* filename);
void deployment_validator_print_summary(deployment_validator_t* validator);

// Utility functions
const char* validation_result_to_string(validation_result_t result);
const char* validation_category_to_string(validation_category_t category);
const char* validation_severity_to_string(validation_severity_t severity);
double deployment_validator_get_progress_percentage(deployment_validator_t* validator);

// Performance monitoring functions
int performance_monitor_start(deployment_validator_t* validator);
int performance_monitor_stop(deployment_validator_t* validator);
int performance_monitor_collect_metrics(deployment_validator_t* validator);

// Code analysis functions
int code_analyzer_scan_directory(deployment_validator_t* validator, const char* directory);
int code_analyzer_check_thread_safety(deployment_validator_t* validator);
int code_analyzer_count_test_coverage(deployment_validator_t* validator);

// Compatibility checking functions
int compatibility_check_system_info(deployment_validator_t* validator);
int compatibility_check_graphics_backends(deployment_validator_t* validator);

#ifdef __cplusplus
}
#endif

#endif // DEPLOYMENT_VALIDATOR_H

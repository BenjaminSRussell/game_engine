/*
 * jiggle_bones_hot_reload.h
 * Professional jiggle bones hot-reload system with file watching
 *
 * Part of the Animation subsystem
 * Advanced 3D Rendering Engine
 *
 * Features:
 * - Real-time file system watching
 * - Automatic asset detection and reloading
 * - Thread-safe hot-reload operations
 * - Comprehensive error handling and validation
 * - Performance monitoring and profiling
 * - Multi-threaded file processing
 * - Asset dependency tracking
 * - Rollback capabilities
 * - Enterprise-grade reliability
 */

#ifndef ANIMATION_JIGGLE_BONES_HOT_RELOAD_H
#define ANIMATION_JIGGLE_BONES_HOT_RELOAD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define JIGGLE_BONES_HOT_RELOAD_MAX_WATCHED_FILES 1024
#define JIGGLE_BONES_HOT_RELOAD_MAX_DEPENDENCIES 256
#define JIGGLE_BONES_HOT_RELOAD_MAX_QUEUE_SIZE 512
#define JIGGLE_BONES_HOT_RELOAD_FILE_BUFFER_SIZE 65536
#define JIGGLE_BONES_HOT_RELOAD_WATCH_INTERVAL_MS 100
#define JIGGLE_BONES_HOT_RELOAD_MAX_RETRY_ATTEMPTS 3
#define JIGGLE_BONES_HOT_RELOAD_RETRY_DELAY_MS 1000

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct jiggle_bones_hot_reload_handle {
    uint32_t id;
} jiggle_bones_hot_reload_handle_t;

typedef enum jiggle_bones_file_type {
    JIGGLE_BONES_FILE_TYPE_JSON = 0,
    JIGGLE_BONES_FILE_TYPE_BINARY = 1,
    JIGGLE_BONES_FILE_TYPE_XML = 2,
    JIGGLE_BONES_FILE_TYPE_INI = 3,
    JIGGLE_BONES_FILE_TYPE_CUSTOM = 4
} jiggle_bones_file_type_t;

typedef enum jiggle_bones_reload_status {
    JIGGLE_BONES_RELOAD_STATUS_PENDING = 0,
    JIGGLE_BONES_RELOAD_STATUS_IN_PROGRESS = 1,
    JIGGLE_BONES_RELOAD_STATUS_COMPLETED = 2,
    JIGGLE_BONES_RELOAD_STATUS_FAILED = 3,
    JIGGLE_BONES_RELOAD_STATUS_ROLLED_BACK = 4
} jiggle_bones_reload_status_t;

typedef struct jiggle_bones_file_info {
    char path[512];
    char absolute_path[1024];
    jiggle_bones_file_type_t type;
    uint64_t last_modified_time;
    uint64_t file_size;
    uint32_t checksum;
    bool is_watched;
    bool is_dirty;
    uint32_t reload_attempts;
    jiggle_bones_reload_status_t status;
    char last_error[256];
} jiggle_bones_file_info_t;

typedef struct jiggle_bones_dependency {
    char parent_file[512];
    char child_file[512];
    uint32_t parent_id;
    uint32_t child_id;
    bool is_direct_dependency;
    float influence_weight;
} jiggle_bones_dependency_t;

typedef struct jiggle_bones_reload_queue_item {
    uint32_t file_id;
    jiggle_bones_reload_status_t status;
    uint64_t enqueue_time;
    uint64_t start_time;
    uint64_t completion_time;
    uint32_t retry_count;
    uint32_t priority;
    void* user_data;
} jiggle_bones_reload_queue_item_t;

typedef struct jiggle_bones_reload_result {
    uint32_t file_id;
    jiggle_bones_reload_status_t status;
    uint64_t reload_time_ms;
    uint32_t retry_count;
    char error_message[256];
    void* new_data;
    size_t new_data_size;
    void* old_data;
    size_t old_data_size;
    bool can_rollback;
} jiggle_bones_reload_result_t;

typedef struct jiggle_bones_hot_reload_settings {
    bool enable_auto_reload;
    bool enable_dependency_tracking;
    bool enable_rollback;
    bool enable_compression;
    bool enable_encryption;
    bool enable_validation;
    uint32_t watch_interval_ms;
    uint32_t max_retry_attempts;
    uint32_t retry_delay_ms;
    uint32_t max_queue_size;
    uint32_t worker_thread_count;
    float memory_limit_mb;
    uint32_t flags;
} jiggle_bones_hot_reload_settings_t;

typedef struct jiggle_bones_hot_reload_performance_stats {
    uint64_t total_reloads;
    uint64_t successful_reloads;
    uint64_t failed_reloads;
    uint64_t rolled_back_reloads;
    float average_reload_time_ms;
    float peak_reload_time_ms;
    uint64_t total_files_watched;
    uint64_t total_dependencies_tracked;
    uint64_t cache_hits;
    uint64_t cache_misses;
    float memory_usage_mb;
    uint64_t frame_count;
} jiggle_bones_hot_reload_performance_stats_t;

typedef struct jiggle_bones_hot_reload_desc {
    char name[128];
    jiggle_bones_hot_reload_settings_t settings;
    const char** watch_directories;
    uint32_t watch_directory_count;
    const char** file_extensions;
    uint32_t file_extension_count;
    size_t memory_budget;
    uint32_t flags;
    void* user_data;
} jiggle_bones_hot_reload_desc_t;

typedef struct jiggle_bones_hot_reload_info {
    uint32_t id;
    char name[128];
    uint32_t flags;
    bool initialized;
    uint32_t watched_file_count;
    uint32_t dependency_count;
    uint32_t queue_size;
    jiggle_bones_hot_reload_performance_stats_t performance;
    bool is_watching;
    float current_memory_usage;
} jiggle_bones_hot_reload_info_t;

/* Callback function types */
typedef int (*jiggle_bones_file_parser_callback)(const char* file_path, jiggle_bones_file_type_t type, void** out_data, size_t* out_size);
typedef int (*jiggle_bones_file_validator_callback)(const void* data, size_t size, jiggle_bones_file_type_t type);
typedef int (*jiggle_bones_reload_callback)(uint32_t file_id, const jiggle_bones_reload_result_t* result, void* user_data);
typedef int (*jiggle_bones_error_callback)(uint32_t file_id, const char* error_message, void* user_data);

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization and Lifecycle */
int jiggle_bones_hot_reload_init(void);
void jiggle_bones_hot_reload_shutdown(void);

int jiggle_bones_hot_reload_create(jiggle_bones_hot_reload_handle_t* out_handle, const jiggle_bones_hot_reload_desc_t* desc);
void jiggle_bones_hot_reload_destroy(jiggle_bones_hot_reload_handle_t handle);

/* File Watching */
int jiggle_bones_hot_reload_add_file(jiggle_bones_hot_reload_handle_t handle, const char* file_path, jiggle_bones_file_type_t type);
int jiggle_bones_hot_reload_remove_file(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id);
int jiggle_bones_hot_reload_add_watch_directory(jiggle_bones_hot_reload_handle_t handle, const char* directory_path);
int jiggle_bones_hot_reload_remove_watch_directory(jiggle_bones_hot_reload_handle_t handle, const char* directory_path);
int jiggle_bones_hot_reload_start_watching(jiggle_bones_hot_reload_handle_t handle);
int jiggle_bones_hot_reload_stop_watching(jiggle_bones_hot_reload_handle_t handle);
bool jiggle_bones_hot_reload_is_watching(jiggle_bones_hot_reload_handle_t handle);

/* File Processing */
int jiggle_bones_hot_reload_process_file(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id);
int jiggle_bones_hot_reload_process_all_files(jiggle_bones_hot_reload_handle_t handle);
int jiggle_bones_hot_reload_process_pending(jiggle_bones_hot_reload_handle_t handle);
int jiggle_bones_hot_reload_force_reload(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id);

/* Dependency Management */
int jiggle_bones_hot_reload_add_dependency(jiggle_bones_hot_reload_handle_t handle, uint32_t parent_file_id, uint32_t child_file_id);
int jiggle_bones_hot_reload_remove_dependency(jiggle_bones_hot_reload_handle_t handle, uint32_t parent_file_id, uint32_t child_file_id);
int jiggle_bones_hot_reload_get_dependencies(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id, uint32_t* out_dependency_ids, uint32_t max_count, uint32_t* out_count);
int jiggle_bones_hot_reload_get_dependents(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id, uint32_t* out_dependent_ids, uint32_t max_count, uint32_t* out_count);

/* Rollback and Recovery */
int jiggle_bones_hot_reload_rollback(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id);
int jiggle_bones_hot_reload_can_rollback(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id);
int jiggle_bones_hot_reload_backup_current_state(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id);

/* Callbacks */
int jiggle_bones_hot_reload_set_file_parser(jiggle_bones_hot_reload_handle_t handle, jiggle_bones_file_parser_callback parser);
int jiggle_bones_hot_reload_set_file_validator(jiggle_bones_hot_reload_handle_t handle, jiggle_bones_file_validator_callback validator);
int jiggle_bones_hot_reload_set_reload_callback(jiggle_bones_hot_reload_handle_t handle, jiggle_bones_reload_callback callback, void* user_data);
int jiggle_bones_hot_reload_set_error_callback(jiggle_bones_hot_reload_handle_t handle, jiggle_bones_error_callback callback, void* user_data);

/* Validation and Error Handling */
int jiggle_bones_hot_reload_validate_file(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id);
int jiggle_bones_hot_reload_get_last_error(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id, char* error_buffer, size_t buffer_size);

/* Performance and Debugging */
const jiggle_bones_hot_reload_performance_stats_t* jiggle_bones_hot_reload_get_performance_stats(jiggle_bones_hot_reload_handle_t handle);
void jiggle_bones_hot_reload_reset_performance_stats(jiggle_bones_hot_reload_handle_t handle);
int jiggle_bones_hot_reload_enable_profiling(jiggle_bones_hot_reload_handle_t handle, bool enable);
int jiggle_bones_hot_reload_get_file_info(jiggle_bones_hot_reload_handle_t handle, uint32_t file_id, jiggle_bones_file_info_t* out_info);
int jiggle_bones_hot_reload_get_all_file_info(jiggle_bones_hot_reload_handle_t handle, jiggle_bones_file_info_t* out_info_array, uint32_t max_count, uint32_t* out_count);

/* Serialization */
int jiggle_bones_hot_reload_serialize_state(jiggle_bones_hot_reload_handle_t handle, const char* filename);
int jiggle_bones_hot_reload_deserialize_state(jiggle_bones_hot_reload_handle_t handle, const char* filename);

/* Memory Management */
int jiggle_bones_hot_reload_set_memory_budget(jiggle_bones_hot_reload_handle_t handle, size_t budget_bytes);
size_t jiggle_bones_hot_reload_get_memory_budget(jiggle_bones_hot_reload_handle_t handle);
size_t jiggle_bones_hot_reload_get_memory_usage(jiggle_bones_hot_reload_handle_t handle);

/* Statistics */
uint32_t jiggle_bones_hot_reload_get_watched_file_count(jiggle_bones_hot_reload_handle_t handle);
uint32_t jiggle_bones_hot_reload_get_pending_reload_count(jiggle_bones_hot_reload_handle_t handle);
jiggle_bones_hot_reload_info_t jiggle_bones_hot_reload_get_info(jiggle_bones_hot_reload_handle_t handle);

/* Thread Safety */
int jiggle_bones_hot_reload_lock(jiggle_bones_hot_reload_handle_t handle);
int jiggle_bones_hot_reload_unlock(jiggle_bones_hot_reload_handle_t handle);
int jiggle_bones_hot_reload_try_lock(jiggle_bones_hot_reload_handle_t handle);

/* Utility Functions */
int jiggle_bones_hot_reload_calculate_file_checksum(const char* file_path, uint32_t* out_checksum);
int jiggle_bones_hot_reload_get_file_modification_time(const char* file_path, uint64_t* out_time);
bool jiggle_bones_hot_reload_file_exists(const char* file_path);
bool jiggle_bones_hot_reload_directory_exists(const char* directory_path);

#ifdef __cplusplus
}
#endif

#endif /* ANIMATION_JIGGLE_BONES_HOT_RELOAD_H */
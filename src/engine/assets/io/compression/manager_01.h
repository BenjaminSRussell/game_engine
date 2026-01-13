/*
 * io_compression_manager_01.h
 *
 * Header file for io_compression_manager_01 implementation
 * Part of the Advanced 3D Rendering Engine
 */

#ifndef IO_COMPRESSION_MANAGER_01_H
#define IO_COMPRESSION_MANAGER_01_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes */
typedef enum {
    IO_COMPRESSION_SUCCESS = 0,
    IO_COMPRESSION_ERROR_INVALID_PARAM = -1,
    IO_COMPRESSION_ERROR_OUT_OF_MEMORY = -2,
    IO_COMPRESSION_ERROR_NOT_INITIALIZED = -3,
    IO_COMPRESSION_ERROR_ALREADY_INITIALIZED = -4,
    IO_COMPRESSION_ERROR_FILE_NOT_FOUND = -5,
    IO_COMPRESSION_ERROR_FILE_READ_FAILED = -6,
    IO_COMPRESSION_ERROR_FILE_WRITE_FAILED = -7,
    IO_COMPRESSION_ERROR_COMPRESSION_FAILED = -8,
    IO_COMPRESSION_ERROR_DECOMPRESSION_FAILED = -9,
    IO_COMPRESSION_ERROR_INVALID_FORMAT = -10,
    IO_COMPRESSION_ERROR_BUFFER_TOO_SMALL = -11,
    IO_COMPRESSION_ERROR_OPERATION_PENDING = -12,
    IO_COMPRESSION_ERROR_OPERATION_CANCELLED = -13,
    IO_COMPRESSION_ERROR_THREAD_ERROR = -14,
    IO_COMPRESSION_ERROR_MEMORY_BUDGET_EXCEEDED = -15,
    IO_COMPRESSION_ERROR_RESOURCE_LOCKED = -16
} io_compression_error_t;

/* Compression algorithms */
typedef enum {
    IO_COMPRESSION_ALGORITHM_NONE = 0,
    IO_COMPRESSION_ALGORITHM_LZ4 = 1,
    IO_COMPRESSION_ALGORITHM_ZSTD = 2,
    IO_COMPRESSION_ALGORITHM_AUTO = 3
} io_compression_algorithm_t;

/* Async operation status */
typedef enum {
    IO_COMPRESSION_ASYNC_PENDING = 0,
    IO_COMPRESSION_ASYNC_IN_PROGRESS = 1,
    IO_COMPRESSION_ASYNC_COMPLETED = 2,
    IO_COMPRESSION_ASYNC_FAILED = 3,
    IO_COMPRESSION_ASYNC_CANCELLED = 4
} io_compression_async_status_t;

/* Memory eviction policies */
typedef enum {
    IO_COMPRESSION_EVICTION_LRU = 0,
    IO_COMPRESSION_EVICTION_LFU = 1,
    IO_COMPRESSION_EVICTION_FIFO = 2,
    IO_COMPRESSION_EVICTION_RANDOM = 3
} io_compression_eviction_policy_t;

/* Forward declarations */
typedef struct io_compression_manager_01 io_compression_manager_01_t;
typedef struct io_compression_manager_01_desc io_compression_manager_01_desc_t;
typedef struct io_compression_manager_01_stats io_compression_manager_01_stats_t;
typedef struct io_compression_memory_budget io_compression_memory_budget_t;
typedef struct io_compression_async_operation io_compression_async_operation_t;
typedef struct io_compression_telemetry io_compression_telemetry_t;
typedef struct io_compression_resource_pool io_compression_resource_pool_t;
typedef struct io_compression_hot_reload_config io_compression_hot_reload_config_t;

/* Callback types */
typedef void (*io_compression_async_callback_t)(io_compression_async_operation_t* op, void* user_data);
typedef void (*io_compression_hot_reload_callback_t)(const char* file_path, void* user_data);

/* Memory budget configuration */
struct io_compression_memory_budget {
    size_t total_budget_bytes;
    size_t compression_cache_limit;
    size_t decompression_cache_limit;
    size_t temp_buffer_limit;
    double eviction_threshold;
    io_compression_eviction_policy_t eviction_policy;
    bool auto_eviction_enabled;
};

/* Async operation structure */
struct io_compression_async_operation {
    uint32_t operation_id;
    io_compression_async_status_t status;
    io_compression_error_t error_code;
    void* input_buffer;
    size_t input_size;
    void* output_buffer;
    size_t output_size;
    size_t output_capacity;
    io_compression_algorithm_t algorithm;
    io_compression_async_callback_t callback;
    void* user_data;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    uint32_t thread_id;
};

/* Telemetry and performance counters */
struct io_compression_telemetry {
    uint64_t total_operations;
    uint64_t successful_operations;
    uint64_t failed_operations;
    uint64_t total_bytes_compressed;
    uint64_t total_bytes_decompressed;
    double total_compression_time_ms;
    double total_decompression_time_ms;
    double avg_compression_ratio;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t evictions;
    uint64_t concurrent_operations;
    double memory_utilization_percent;
};

/* Resource pool structure */
struct io_compression_resource_pool {
    void** buffers;
    size_t* buffer_sizes;
    size_t pool_size;
    size_t pool_capacity;
    size_t default_buffer_size;
    uint32_t available_count;
    uint32_t total_allocated;
    bool is_thread_safe;
};

/* Hot reload configuration */
struct io_compression_hot_reload_config {
    bool enabled;
    char watch_directory[512];
    double poll_interval_seconds;
    io_compression_hot_reload_callback_t callback;
    void* user_data;
    bool recursive_watching;
    const char** file_extensions;
    size_t extension_count;
};

/* Creation and destruction */
int io_compression_manager_01_create(io_compression_manager_01_t** out_ctx, const io_compression_manager_01_desc_t* desc);
int io_compression_manager_01_destroy(io_compression_manager_01_t* ctx);

/* Core operations */
int io_compression_manager_01_init(io_compression_manager_01_t* ctx, void* params);
int io_compression_manager_01_shutdown(io_compression_manager_01_t* ctx, void* params);
int io_compression_manager_01_update(io_compression_manager_01_t* ctx, void* params);
int io_compression_manager_01_create_legacy(void* ctx, void* params);
int io_compression_manager_01_destroy_legacy(void* ctx, void* params);
int io_compression_manager_01_get(io_compression_manager_01_t* ctx, void* params);
int io_compression_manager_01_set(io_compression_manager_01_t* ctx, void* params);
int io_compression_manager_01_reset(io_compression_manager_01_t* ctx, void* params);
int io_compression_manager_01_validate(io_compression_manager_01_t* ctx, void* params);
int io_compression_manager_01_flush(io_compression_manager_01_t* ctx, void* params);

/* Memory management */
int io_compression_manager_01_set_memory_budget(io_compression_manager_01_t* ctx, const io_compression_memory_budget_t* budget);
int io_compression_manager_01_get_memory_budget(io_compression_manager_01_t* ctx, io_compression_memory_budget_t* out_budget);
int io_compression_manager_01_force_eviction(io_compression_manager_01_t* ctx, size_t bytes_to_free);

/* Async operations */
uint32_t io_compression_manager_01_compress_async(io_compression_manager_01_t* ctx,
                                                   const void* input, size_t input_size,
                                                   void* output, size_t output_capacity,
                                                   io_compression_algorithm_t algorithm,
                                                   io_compression_async_callback_t callback, void* user_data);
uint32_t io_compression_manager_01_decompress_async(io_compression_manager_01_t* ctx,
                                                     const void* input, size_t input_size,
                                                     void* output, size_t output_capacity,
                                                     io_compression_async_callback_t callback, void* user_data);
int io_compression_manager_01_wait_for_operation(io_compression_manager_01_t* ctx, uint32_t operation_id, uint32_t timeout_ms);
int io_compression_manager_01_cancel_operation(io_compression_manager_01_t* ctx, uint32_t operation_id);
int io_compression_manager_01_get_operation_status(io_compression_manager_01_t* ctx, uint32_t operation_id,
                                                   io_compression_async_status_t* out_status, io_compression_error_t* out_error);

/* Compression operations */
int io_compression_manager_01_compress(io_compression_manager_01_t* ctx,
                                       const void* input, size_t input_size,
                                       void* output, size_t* output_size,
                                       size_t output_capacity, io_compression_algorithm_t algorithm);
int io_compression_manager_01_decompress(io_compression_manager_01_t* ctx,
                                         const void* input, size_t input_size,
                                         void* output, size_t* output_size,
                                         size_t output_capacity);

/* Resource pooling */
int io_compression_manager_01_init_resource_pool(io_compression_manager_01_t* ctx, size_t pool_size, size_t default_buffer_size);
void* io_compression_manager_01_acquire_buffer(io_compression_manager_01_t* ctx, size_t size);
int io_compression_manager_01_release_buffer(io_compression_manager_01_t* ctx, void* buffer);
int io_compression_manager_01_cleanup_resource_pool(io_compression_manager_01_t* ctx);

/* Hot reload support */
int io_compression_manager_01_enable_hot_reload(io_compression_manager_01_t* ctx, const io_compression_hot_reload_config_t* config);
int io_compression_manager_01_disable_hot_reload(io_compression_manager_01_t* ctx);
bool io_compression_manager_01_is_hot_reload_enabled(io_compression_manager_01_t* ctx);

/* Telemetry and profiling */
int io_compression_manager_01_get_telemetry(io_compression_manager_01_t* ctx, io_compression_telemetry_t* out_telemetry);
int io_compression_manager_01_reset_telemetry(io_compression_manager_01_t* ctx);
int io_compression_manager_01_start_profiling(io_compression_manager_01_t* ctx);
int io_compression_manager_01_stop_profiling(io_compression_manager_01_t* ctx);

/* Serialization and persistence */
int io_compression_manager_01_serialize_state(io_compression_manager_01_t* ctx, void* buffer, size_t* buffer_size);
int io_compression_manager_01_deserialize_state(io_compression_manager_01_t* ctx, const void* buffer, size_t buffer_size);
int io_compression_manager_01_save_state_to_file(io_compression_manager_01_t* ctx, const char* file_path);
int io_compression_manager_01_load_state_from_file(io_compression_manager_01_t* ctx, const char* file_path);

/* Format conversion */
int io_compression_manager_01_convert_format(io_compression_manager_01_t* ctx,
                                              const void* input, size_t input_size,
                                              void* output, size_t* output_size,
                                              size_t output_capacity,
                                              io_compression_algorithm_t from_algorithm,
                                              io_compression_algorithm_t to_algorithm);

/* Scene file parsing */
int io_compression_manager_01_parse_scene_file(io_compression_manager_01_t* ctx, const char* file_path);
int io_compression_manager_01_export_scene_file(io_compression_manager_01_t* ctx, const char* file_path);

/* Multi-threading support */
int io_compression_manager_01_set_thread_count(io_compression_manager_01_t* ctx, uint32_t thread_count);
uint32_t io_compression_manager_01_get_thread_count(io_compression_manager_01_t* ctx);
int io_compression_manager_01_process_batch(io_compression_manager_01_t* ctx,
                                             const void** inputs, size_t* input_sizes,
                                             void** outputs, size_t* output_sizes,
                                             size_t* output_capacities, uint32_t batch_size,
                                             io_compression_algorithm_t algorithm);

/* Utility functions */
int io_compression_manager_01_get_stats(io_compression_manager_01_t* ctx);
int io_compression_manager_01_set_callback(io_compression_manager_01_t* ctx);
int io_compression_manager_01_get_memory_usage(io_compression_manager_01_t* ctx);
int io_compression_manager_01_optimize(io_compression_manager_01_t* ctx);
int io_compression_manager_01_debug_print(io_compression_manager_01_t* ctx);
const char* io_compression_manager_01_get_error_string(io_compression_error_t error_code);

/* Module functions */
int io_compression_manager_01_module_init(void);
int io_compression_manager_01_module_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* IO_COMPRESSION_MANAGER_01_H */

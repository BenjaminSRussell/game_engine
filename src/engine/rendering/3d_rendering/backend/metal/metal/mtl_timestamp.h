/*
 * mtl_timestamp.h
 * Metal GPU timestamp query system for precise timing measurements
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef MTL_TIMESTAMP_H
#define MTL_TIMESTAMP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#else
typedef void* id;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct metal_command_buffer metal_command_buffer_t;

// ============================================================================
// GPU Timestamp Query System
// ============================================================================

/**
 * Timestamp query pool for GPU profiling.
 * Uses MTLCounterSampleBuffer for precise GPU timing.
 */
typedef struct metal_timestamp_pool metal_timestamp_pool_t;

/**
 * Timestamp query result.
 */
typedef struct metal_timestamp_result {
    uint64_t gpu_timestamp_ns;         // GPU timestamp in nanoseconds
    uint64_t cpu_timestamp_ns;         // Corresponding CPU timestamp
    bool valid;                        // Result is valid and ready
} metal_timestamp_result_t;

/**
 * Timestamp query configuration.
 */
typedef struct metal_timestamp_config {
    uint32_t max_queries;              // Maximum number of concurrent queries
    bool enable_cpu_correlation;       // Correlate GPU/CPU timestamps
    bool enable_validation;            // Validate query results
} metal_timestamp_config_t;

/**
 * Create a timestamp query pool.
 * 
 * @param device Metal device (id<MTLDevice>)
 * @param config Pool configuration
 * @return Timestamp pool or NULL on failure
 */
#ifdef __OBJC__
metal_timestamp_pool_t* metal_timestamp_pool_create(id<MTLDevice> device, const metal_timestamp_config_t* config);
#else
metal_timestamp_pool_t* metal_timestamp_pool_create(id device, const metal_timestamp_config_t* config);
#endif

/**
 * Begin a timestamp query.
 * Call at the start of the region to measure.
 * 
 * @param pool Timestamp pool
 * @param cmd_buffer Command buffer to encode timestamp into
 * @param label Optional debug label for the query
 * @return Query ID, or UINT32_MAX on failure
 */
uint32_t metal_timestamp_begin(metal_timestamp_pool_t* pool, 
                                metal_command_buffer_t* cmd_buffer,
                                const char* label);

/**
 * End a timestamp query.
 * Call at the end of the region to measure.
 * 
 * @param pool Timestamp pool
 * @param cmd_buffer Command buffer to encode timestamp into
 * @param query_id Query ID from metal_timestamp_begin
 */
void metal_timestamp_end(metal_timestamp_pool_t* pool,
                        metal_command_buffer_t* cmd_buffer,
                        uint32_t query_id);

/**
 * Get timestamp query result in nanoseconds.
 * 
 * @param pool Timestamp pool
 * @param query_id Query ID
 * @param result Output timestamp result
 * @return true if result is available, false if still pending
 */
bool metal_timestamp_get_result(metal_timestamp_pool_t* pool,
                                uint32_t query_id,
                                metal_timestamp_result_t* result);

/**
 * Get elapsed time between two timestamps in milliseconds.
 * 
 * @param pool Timestamp pool
 * @param begin_query_id Begin query ID
 * @param end_query_id End query ID
 * @param elapsed_ms Output elapsed time in milliseconds
 * @return true if elapsed time is available
 */
bool metal_timestamp_get_elapsed(metal_timestamp_pool_t* pool,
                                 uint32_t begin_query_id,
                                 uint32_t end_query_id,
                                 double* elapsed_ms);

/**
 * Get GPU timer resolution in nanoseconds.
 * 
 * @param pool Timestamp pool
 * @return Timer resolution in nanoseconds
 */
double metal_timestamp_get_resolution_ns(metal_timestamp_pool_t* pool);

/**
 * Reset all queries in the pool for reuse.
 * 
 * @param pool Timestamp pool
 */
void metal_timestamp_pool_reset(metal_timestamp_pool_t* pool);

/**
 * Destroy timestamp pool.
 * 
 * @param pool Timestamp pool to destroy
 */
void metal_timestamp_pool_destroy(metal_timestamp_pool_t* pool);

// ============================================================================
// Scoped Timestamp Queries
// ============================================================================

/**
 * Scoped timestamp for RAII-style timing.
 */
typedef struct metal_scoped_timestamp {
    metal_timestamp_pool_t* pool;
    uint32_t query_id;
    const char* label;
} metal_scoped_timestamp_t;

/**
 * Begin a scoped timestamp.
 * 
 * @param pool Timestamp pool
 * @param cmd_buffer Command buffer
 * @param label Debug label
 * @return Scoped timestamp object
 */
metal_scoped_timestamp_t metal_timestamp_scope_begin(metal_timestamp_pool_t* pool,
                                                     metal_command_buffer_t* cmd_buffer,
                                                     const char* label);

/**
 * End a scoped timestamp and get result.
 * 
 * @param scope Scoped timestamp
 * @param cmd_buffer Command buffer
 * @param result Output result
 */
void metal_timestamp_scope_end(metal_scoped_timestamp_t* scope,
                               metal_command_buffer_t* cmd_buffer,
                               metal_timestamp_result_t* result);

// ============================================================================
// Profiling Statistics
// ============================================================================

/**
 * Profiling statistics for a named region.
 */
typedef struct metal_profiling_stats {
    const char* label;                 // Region label
    uint64_t sample_count;             // Number of samples
    double min_time_ms;                // Minimum time
    double max_time_ms;                // Maximum time
    double avg_time_ms;                // Average time
    double total_time_ms;              // Total accumulated time
} metal_profiling_stats_t;

/**
 * Get profiling statistics for a labeled region.
 * 
 * @param pool Timestamp pool
 * @param label Region label
 * @param stats Output statistics
 * @return true if statistics are available
 */
bool metal_timestamp_get_stats(metal_timestamp_pool_t* pool,
                               const char* label,
                               metal_profiling_stats_t* stats);

/**
 * Print profiling statistics to console.
 * 
 * @param pool Timestamp pool
 */
void metal_timestamp_print_stats(metal_timestamp_pool_t* pool);

/**
 * Reset profiling statistics.
 * 
 * @param pool Timestamp pool
 */
void metal_timestamp_reset_stats(metal_timestamp_pool_t* pool);

#ifdef __cplusplus
}
#endif

#endif // MTL_TIMESTAMP_H

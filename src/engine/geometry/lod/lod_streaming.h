/*
 * lod_streaming.h
 * Asynchronous LOD loading and streaming
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_LOD_STREAMING_H
#define GEOMETRY_LOD_STREAMING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lod_generator.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

// Streaming priority
typedef enum lod_stream_priority {
    LOD_PRIORITY_LOW = 0,
    LOD_PRIORITY_NORMAL,
    LOD_PRIORITY_HIGH,
    LOD_PRIORITY_CRITICAL
} lod_stream_priority_t;

// Streaming request status
typedef enum lod_stream_status {
    LOD_STREAM_PENDING,
    LOD_STREAM_LOADING,
    LOD_STREAM_LOADED,
    LOD_STREAM_FAILED
} lod_stream_status_t;

// Streaming request
typedef struct lod_stream_request {
    uint32_t chain_id;
    uint32_t lod_level;
    lod_stream_priority_t priority;
    lod_stream_status_t status;
    float distance_to_camera;
    uint64_t request_frame;
    void* user_data;
} lod_stream_request_t;

// Streaming configuration
typedef struct lod_stream_config {
    uint32_t max_concurrent_loads;
    size_t max_bytes_per_frame;
    float prefetch_distance_multiplier;
    bool enable_prefetching;
    bool enable_progressive_loading;
} lod_stream_config_t;

// Streaming statistics
typedef struct lod_stream_stats {
    uint32_t pending_requests;
    uint32_t completed_requests;
    uint32_t failed_requests;
    size_t bytes_loaded_this_frame;
    size_t total_bytes_loaded;
    float average_load_time_ms;
} lod_stream_stats_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lod_streaming_init(const lod_stream_config_t* config);
void lod_streaming_shutdown(void);

/* Configuration */
void lod_streaming_set_config(const lod_stream_config_t* config);
lod_stream_config_t lod_streaming_get_config(void);
lod_stream_stats_t lod_streaming_get_stats(void);

/* Request Management */
int lod_streaming_request(
    uint32_t chain_id,
    uint32_t lod_level,
    lod_stream_priority_t priority,
    void* user_data
);

void lod_streaming_cancel(uint32_t chain_id, uint32_t lod_level);
lod_stream_status_t lod_streaming_get_status(uint32_t chain_id, uint32_t lod_level);

/* Update */
void lod_streaming_update(float delta_time);

/* Prefetching */
void lod_streaming_prefetch_for_camera(
    const float camera_pos[3],
    const float camera_forward[3],
    float max_distance
);

void lod_streaming_prefetch_chain(
    uint32_t chain_id,
    const float object_pos[3],
    float object_radius,
    const float camera_pos[3]
);

/* Callbacks */
typedef void (*lod_stream_callback_t)(uint32_t chain_id, uint32_t lod_level, void* user_data);

void lod_streaming_set_completion_callback(lod_stream_callback_t callback);
void lod_streaming_set_failure_callback(lod_stream_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_LOD_STREAMING_H */

/*
 * lod_streaming.c
 * Asynchronous LOD loading and streaming
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#include "lod_streaming.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

#define MAX_STREAM_REQUESTS 256

static struct {
    bool initialized;
    lod_stream_config_t config;
    lod_stream_stats_t stats;
    lod_stream_request_t requests[MAX_STREAM_REQUESTS];
    uint32_t request_count;
    lod_stream_callback_t completion_callback;
    lod_stream_callback_t failure_callback;
    uint64_t current_frame;
} g_lod_streaming = {0};

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int lod_streaming_init(const lod_stream_config_t* config) {
    if (g_lod_streaming.initialized) {
        return -1;
    }
    
    if (config) {
        g_lod_streaming.config = *config;
    } else {
        // Default configuration
        g_lod_streaming.config.max_concurrent_loads = 4;
        g_lod_streaming.config.max_bytes_per_frame = 16 * 1024 * 1024; // 16MB/frame
        g_lod_streaming.config.prefetch_distance_multiplier = 2.0f;
        g_lod_streaming.config.enable_prefetching = true;
        g_lod_streaming.config.enable_progressive_loading = true;
    }
    
    memset(&g_lod_streaming.stats, 0, sizeof(lod_stream_stats_t));
    g_lod_streaming.request_count = 0;
    g_lod_streaming.current_frame = 0;
    g_lod_streaming.initialized = true;
    
    return 0;
}

void lod_streaming_shutdown(void) {
    if (!g_lod_streaming.initialized) return;
    
    // Cancel all pending requests
    for (uint32_t i = 0; i < g_lod_streaming.request_count; i++) {
        // In production: cancel actual async operations
    }
    
    memset(&g_lod_streaming, 0, sizeof(g_lod_streaming));
}

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

void lod_streaming_set_config(const lod_stream_config_t* config) {
    if (!config) return;
    g_lod_streaming.config = *config;
}

lod_stream_config_t lod_streaming_get_config(void) {
    return g_lod_streaming.config;
}

lod_stream_stats_t lod_streaming_get_stats(void) {
    return g_lod_streaming.stats;
}

/* ============================================================================
 * REQUEST MANAGEMENT
 * ============================================================================ */

static int find_request(uint32_t chain_id, uint32_t lod_level) {
    for (uint32_t i = 0; i < g_lod_streaming.request_count; i++) {
        if (g_lod_streaming.requests[i].chain_id == chain_id &&
            g_lod_streaming.requests[i].lod_level == lod_level) {
            return (int)i;
        }
    }
    return -1;
}

int lod_streaming_request(
    uint32_t chain_id,
    uint32_t lod_level,
    lod_stream_priority_t priority,
    void* user_data
) {
    if (!g_lod_streaming.initialized) return -1;
    if (g_lod_streaming.request_count >= MAX_STREAM_REQUESTS) return -1;
    
    // Check if already requested
    int existing = find_request(chain_id, lod_level);
    if (existing >= 0) {
        // Update priority if higher
        if (priority > g_lod_streaming.requests[existing].priority) {
            g_lod_streaming.requests[existing].priority = priority;
        }
        return existing;
    }
    
    // Add new request
    lod_stream_request_t* req = &g_lod_streaming.requests[g_lod_streaming.request_count++];
    req->chain_id = chain_id;
    req->lod_level = lod_level;
    req->priority = priority;
    req->status = LOD_STREAM_PENDING;
    req->distance_to_camera = 0.0f;
    req->request_frame = g_lod_streaming.current_frame;
    req->user_data = user_data;
    
    g_lod_streaming.stats.pending_requests++;
    
    return (int)(g_lod_streaming.request_count - 1);
}

void lod_streaming_cancel(uint32_t chain_id, uint32_t lod_level) {
    if (!g_lod_streaming.initialized) return;
    
    int idx = find_request(chain_id, lod_level);
    if (idx < 0) return;
    
    // Remove request (swap with last)
    g_lod_streaming.requests[idx] = g_lod_streaming.requests[g_lod_streaming.request_count - 1];
    g_lod_streaming.request_count--;
    
    if (g_lod_streaming.stats.pending_requests > 0) {
        g_lod_streaming.stats.pending_requests--;
    }
}

lod_stream_status_t lod_streaming_get_status(uint32_t chain_id, uint32_t lod_level) {
    if (!g_lod_streaming.initialized) return LOD_STREAM_FAILED;
    
    int idx = find_request(chain_id, lod_level);
    if (idx < 0) return LOD_STREAM_FAILED;
    
    return g_lod_streaming.requests[idx].status;
}

/* ============================================================================
 * UPDATE
 * ============================================================================ */

static int compare_requests(const void* a, const void* b) {
    const lod_stream_request_t* req_a = (const lod_stream_request_t*)a;
    const lod_stream_request_t* req_b = (const lod_stream_request_t*)b;
    
    // Sort by priority (higher first), then by distance (closer first)
    if (req_a->priority != req_b->priority) {
        return (int)req_b->priority - (int)req_a->priority;
    }
    
    if (req_a->distance_to_camera < req_b->distance_to_camera) {
        return -1;
    } else if (req_a->distance_to_camera > req_b->distance_to_camera) {
        return 1;
   }
    
    return 0;
}

void lod_streaming_update(float delta_time) {
    if (!g_lod_streaming.initialized) return;
    
    g_lod_streaming.current_frame++;
    g_lod_streaming.stats.bytes_loaded_this_frame = 0;
    
    // Sort requests by priority
    qsort(g_lod_streaming.requests, g_lod_streaming.request_count, 
          sizeof(lod_stream_request_t), compare_requests);
    
    // Process requests
    uint32_t concurrent_loads = 0;
    size_t bytes_this_frame = 0;
    
    for (uint32_t i = 0; i < g_lod_streaming.request_count && 
         concurrent_loads < g_lod_streaming.config.max_concurrent_loads &&
         bytes_this_frame < g_lod_streaming.config.max_bytes_per_frame; i++) {
        
        lod_stream_request_t* req = &g_lod_streaming.requests[i];
        
        if (req->status == LOD_STREAM_PENDING) {
            // Start loading (simulation)
            req->status = LOD_STREAM_LOADING;
            concurrent_loads++;
            
            // Simulate: Immediately complete for demo
            // In production: kick off async I/O
            req->status = LOD_STREAM_LOADED;
            
            // Estimate size (simplified)
            size_t estimated_size = 1024 * 1024; // 1MB per LOD
            bytes_this_frame += estimated_size;
            g_lod_streaming.stats.bytes_loaded_this_frame += estimated_size;
            g_lod_streaming.stats.total_bytes_loaded += estimated_size;
            
            g_lod_streaming.stats.completed_requests++;
            if (g_lod_streaming.stats.pending_requests > 0) {
                g_lod_streaming.stats.pending_requests--;
            }
            
            // Callback
            if (g_lod_streaming.completion_callback) {
                g_lod_streaming.completion_callback(req->chain_id, req->lod_level, req->user_data);
            }
        }
    }
    
    // Remove completed requests
    uint32_t write_idx = 0;
    for (uint32_t read_idx = 0; read_idx < g_lod_streaming.request_count; read_idx++) {
        if (g_lod_streaming.requests[read_idx].status != LOD_STREAM_LOADED) {
            g_lod_streaming.requests[write_idx++] = g_lod_streaming.requests[read_idx];
        }
    }
    g_lod_streaming.request_count = write_idx;
}

/* ============================================================================
 * PREFETCHING
 * ============================================================================ */

void lod_streaming_prefetch_chain(
    uint32_t chain_id,
    const float object_pos[3],
    float object_radius,
    const float camera_pos[3]
) {
    if (!g_lod_streaming.initialized || !g_lod_streaming.config.enable_prefetching) {
        return;
    }
    
    // Calculate distance
    float dx = object_pos[0] - camera_pos[0];
    float dy = object_pos[1] - camera_pos[1];
    float dz = object_pos[2] - camera_pos[2];
    float distance = sqrtf(dx*dx + dy*dy + dz*dz);
    
    // Determine which LOD to prefetch based on distance
    // This is a heuristic - actual thresholds would come from LOD chain
    float prefetch_distance = distance * g_lod_streaming.config.prefetch_distance_multiplier;
    
    // Prefetch LOD 1 or 2 for medium distance objects
    uint32_t lod_to_prefetch = 1;
    if (prefetch_distance > object_radius * 20.0f) {
        lod_to_prefetch = 2;
    }
    
    lod_streaming_request(chain_id, lod_to_prefetch, LOD_PRIORITY_LOW, NULL);
}

void lod_streaming_prefetch_for_camera(
    const float camera_pos[3],
    const float camera_forward[3],
    float max_distance
) {
    if (!g_lod_streaming.initialized || !g_lod_streaming.config.enable_prefetching) {
        return;
    }
    
    // In production: Query spatial structure for objects in frustum
    // For now: placeholder
    // This would iterate visible objects and call lod_streaming_prefetch_chain
}

/* ============================================================================
 * CALLBACKS
 * ============================================================================ */

void lod_streaming_set_completion_callback(lod_stream_callback_t callback) {
    g_lod_streaming.completion_callback = callback;
}

void lod_streaming_set_failure_callback(lod_stream_callback_t callback) {
    g_lod_streaming.failure_callback = callback;
}

/* End of lod_streaming.c */

/*
 * texture_streamer.c
 * Texture streaming system implementation
 */

#include "texture_streamer.h"
#include "stream_priority.h"
#include "resident_mips.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static texture_streamer_t g_global_streamer = {0};

/* ============================================================================
 * PRIVATE FUNCTIONS
 * ============================================================================ */

static void priority_queue_push(priority_queue_t* queue, stream_request_t request) {
    if (queue->count >= queue->capacity) {
        uint32_t new_capacity = queue->capacity == 0 ? 16 : queue->capacity * 2;
        stream_request_t* new_requests = realloc(queue->requests, new_capacity * sizeof(stream_request_t));
        if (!new_requests) return;
        queue->requests = new_requests;
        queue->capacity = new_capacity;
    }

    // Simple insertion sort for priority (for now, could be a heap)
    uint32_t i = queue->count;
    while (i > 0 && queue->requests[i - 1].priority < request.priority) {
        queue->requests[i] = queue->requests[i - 1];
        i--;
    }
    queue->requests[i] = request;
    queue->count++;
}

static stream_request_t priority_queue_pop(priority_queue_t* queue) {
    if (queue->count == 0) {
        return (stream_request_t){0};
    }
    return queue->requests[--queue->count];
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

texture_streamer_t* texture_streamer_get_global(void) {
    return &g_global_streamer;
}

int texture_streamer_init(texture_streamer_t* streamer, const texture_texture_streamer_desc_t* desc) {
    if (!streamer) return -1;
    
    memset(streamer, 0, sizeof(texture_streamer_t));
    
    streamer->texture_capacity = desc->max_textures > 0 ? desc->max_textures : 1024;
    streamer->textures = calloc(streamer->texture_capacity, sizeof(texture_entry_t));
    if (!streamer->textures) return -2;
    
    streamer->memory_budget = desc->memory_budget > 0 ? desc->memory_budget : DEFAULT_STREAMING_BUDGET;
    streamer->memory_used = 0;
    
    streamer->request_queue.capacity = MAX_STREAMING_REQUESTS;
    streamer->request_queue.requests = malloc(streamer->request_queue.capacity * sizeof(stream_request_t));
    streamer->request_queue.count = 0;
    
    streamer->initialized = true;
    return 0;
}

void texture_streamer_shutdown(texture_streamer_t* streamer) {
    if (!streamer || !streamer->initialized) return;
    
    if (streamer->textures) free(streamer->textures);
    if (streamer->request_queue.requests) free(streamer->request_queue.requests);
    
    memset(streamer, 0, sizeof(texture_streamer_t));
}

int texture_streamer_register_texture(texture_streamer_t* streamer, uint32_t handle, uint32_t max_mips) {
    if (!streamer || !streamer->initialized) return -1;
    
    if (streamer->texture_count >= streamer->texture_capacity) return -2;
    
    uint32_t index = streamer->texture_count++;
    texture_entry_t* entry = &streamer->textures[index];
    
    entry->handle = handle;
    entry->max_mips = max_mips;
    entry->current_mip = max_mips - 1; // Start at lowest mip
    entry->target_mip = max_mips - 1;
    entry->is_streaming = false;
    
    return (int)index;
}

void texture_streamer_unregister_texture(texture_streamer_t* streamer, uint32_t handle) {
    if (!streamer || !streamer->initialized) return;
    
    for (uint32_t i = 0; i < streamer->texture_count; i++) {
        if (streamer->textures[i].handle == handle) {
            // Swap with last and decrement count
            streamer->textures[i] = streamer->textures[--streamer->texture_count];
            return;
        }
    }
}

void texture_streamer_update(texture_streamer_t* streamer, void* camera) {
    if (!streamer || !streamer->initialized) return;
    
    streamer->request_queue.count = 0; // Clear queue for new frame
    
    for (uint32_t i = 0; i < streamer->texture_count; i++) {
        texture_entry_t* entry = &streamer->textures[i];
        
        // Calculate screen coverage and target mip
        entry->screen_coverage = texture_streamer_calculate_screen_coverage(entry->handle, camera);
        entry->target_mip = texture_streamer_calculate_target_mip(entry->screen_coverage, entry->max_mips);
        
        // Calculate distance score for priority
        float distance_score = texture_priority_calculate_distance_score(NULL, camera); // Simplified
        entry->priority = texture_priority_calculate_final_score(entry->screen_coverage, distance_score, entry->current_mip, entry->target_mip);
        
        if (entry->target_mip < entry->current_mip && !entry->is_streaming) {
            texture_streamer_queue_request(streamer, i, entry->target_mip, entry->priority);
        }
    }
    
    texture_streamer_process_requests(streamer);
}

void texture_streamer_queue_request(texture_streamer_t* streamer, uint32_t texture_index, uint32_t target_mip, float priority) {
    stream_request_t req = {
        .texture_index = texture_index,
        .target_mip = target_mip,
        .priority = priority
    };
    priority_queue_push(&streamer->request_queue, req);
}

void texture_streamer_process_requests(texture_streamer_t* streamer) {
    while (streamer->request_queue.count > 0 && streamer->memory_used < streamer->memory_budget) {
        stream_request_t req = priority_queue_pop(&streamer->request_queue);
        texture_entry_t* entry = &streamer->textures[req.texture_index];
        
        size_t mip_size = texture_residency_get_mip_memory_size(entry->handle, req.target_mip);
        if (streamer->memory_used + mip_size <= streamer->memory_budget) {
            if (texture_streamer_load_mip(entry->handle, req.target_mip) == 0) {
                streamer->memory_used += mip_size;
                entry->current_mip = req.target_mip;
            }
        } else {
            // Out of budget, stop processing
            break;
        }
    }
}

float texture_streamer_calculate_screen_coverage(uint32_t handle, void* camera) {
    // Placeholder: Use bounds from handle to calculate coverage
    float bounds_min[3] = {-1, -1, -1};
    float bounds_max[3] = {1, 1, 1};
    return texture_priority_calculate_screen_coverage(bounds_min, bounds_max, camera);
}

uint32_t texture_streamer_calculate_target_mip(float screen_coverage, uint32_t max_mips) {
    return texture_priority_calculate_target_mip(screen_coverage, max_mips, 0.0f);
}

int texture_streamer_load_mip(uint32_t handle, uint32_t mip) {
    return texture_residency_set_mip_status(handle, mip, true);
}

int texture_streamer_unload_mip(uint32_t handle, uint32_t mip) {
    return texture_residency_set_mip_status(handle, mip, false);
}

/* Statistics */
int texture_streamer_get_info(const texture_streamer_t* streamer, texture_texture_streamer_info_t* out_info) {
    if (!streamer || !out_info) return -1;
    out_info->texture_count = streamer->texture_count;
    out_info->memory_budget = streamer->memory_budget;
    out_info->memory_used = streamer->memory_used;
    out_info->pending_requests = streamer->request_queue.count;
    return 0;
}

void texture_streamer_debug_print(const texture_streamer_t* streamer) {
    printf("Texture Streamer Debug:\n");
    printf("  Textures: %u / %u\n", streamer->texture_count, streamer->texture_capacity);
    printf("  Memory: %u MB / %u MB\n", streamer->memory_used / (1024*1024), streamer->memory_budget / (1024*1024));
    printf("  Pending Requests: %u\n", streamer->request_queue.count);
}

/* Core API compatibility */
int texture_texture_streamer_init(void) {
    texture_texture_streamer_desc_t desc = {
        .memory_budget = DEFAULT_STREAMING_BUDGET,
        .max_textures = 1024
    };
    return texture_streamer_init(&g_global_streamer, &desc);
}

void texture_texture_streamer_shutdown(void) {
    texture_streamer_shutdown(&g_global_streamer);
}

int texture_texture_streamer_create(texture_texture_streamer_handle_t* out_handle, const texture_texture_streamer_desc_t* desc) {
    // For original API, we still use the global instance
    if (!g_global_streamer.initialized) {
        texture_streamer_init(&g_global_streamer, desc);
    }
    out_handle->id = 0; // Fixed ID for global instance
    return 0;
}

void texture_texture_streamer_destroy(texture_texture_streamer_handle_t handle) {
    (void)handle;
    // Don't shut down global streamer on destroy handle
}

int texture_texture_streamer_process_pending(void) {
    if (!g_global_streamer.initialized) return 0;
    texture_streamer_process_requests(&g_global_streamer);
    return g_global_streamer.request_queue.count;
}


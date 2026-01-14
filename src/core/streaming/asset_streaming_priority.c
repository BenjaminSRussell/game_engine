#include "core/resource/streaming/asset_streaming_priority.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <include/math/math_all.h>

// Helper function to swap two asset requests
static void swap_requests(AssetRequest* a, AssetRequest* b) {
    AssetRequest temp = *a;
    *a = *b;
    *b = temp;
}

// Helper function to compare priority (higher priority = lower score)
static bool compare_priority(AssetRequest* a, AssetRequest* b) {
    if (a->priority != b->priority) {
        return a->priority < b->priority; // Lower enum value = higher priority
    }
    return a->distance_from_player < b->distance_from_player;
}

// Heap operations for priority queue
static void heapify_up(AssetPriorityQueue* queue, u32 index) {
    while (index > 0) {
        u32 parent = (index - 1) / 2;
        if (compare_priority(&queue->requests[index], &queue->requests[parent])) {
            swap_requests(&queue->requests[index], &queue->requests[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

static void heapify_down(AssetPriorityQueue* queue, u32 index) {
    u32 size = queue->count;
    
    while (1) {
        u32 left = 2 * index + 1;
        u32 right = 2 * index + 2;
        u32 smallest = index;
        
        if (left < size && compare_priority(&queue->requests[left], &queue->requests[smallest])) {
            smallest = left;
        }
        if (right < size && compare_priority(&queue->requests[right], &queue->requests[smallest])) {
            smallest = right;
        }
        
        if (smallest != index) {
            swap_requests(&queue->requests[index], &queue->requests[smallest]);
            index = smallest;
        } else {
            break;
        }
    }
}

bool asset_priority_init(AssetStreamingPriority* system) {
    if (!system) return false;
    
    memset(system, 0, sizeof(AssetStreamingPriority));
    
    // Allocate priority queue
    system->queue = (AssetPriorityQueue*)memory_alloc(sizeof(AssetPriorityQueue));
    if (!system->queue) {
        LOG_ERROR("Failed to allocate asset priority queue");
        return false;
    }
    
    memset(system->queue, 0, sizeof(AssetPriorityQueue));
    system->queue->capacity = MAX_PRIORITY_QUEUE_SIZE;
    
    // Initialize default priority factors
    system->queue->factors.distance_weight = 0.4f;
    system->queue->factors.visibility_weight = 0.3f;
    system->queue->factors.type_weight = 0.2f;
    system->queue->factors.velocity_weight = 0.05f;
    system->queue->factors.urgency_weight = 0.05f;
    
    // Initialize default distance thresholds
    system->critical_distance = 10.0f;    // 10 meters
    system->high_distance = 50.0f;        // 50 meters
    system->medium_distance = 200.0f;      // 200 meters
    system->preload_distance = 500.0f;     // 500 meters
    
    // Initialize type-specific priorities
    system->type_priorities[ASSET_TYPE_TEXTURE] = 0.8f;
    system->type_priorities[ASSET_TYPE_MESH] = 0.9f;
    system->type_priorities[ASSET_TYPE_AUDIO] = 0.7f;
    system->type_priorities[ASSET_TYPE_MATERIAL] = 0.6f;
    system->type_priorities[ASSET_TYPE_ANIMATION] = 0.5f;
    system->type_priorities[ASSET_TYPE_SHADER] = 0.4f;
    
    system->initialized = true;
    system->hit_rate = 0.0f;
    
    LOG_INFO("Asset streaming priority system initialized");
    return true;
}

void asset_priority_shutdown(AssetStreamingPriority* system) {
    if (!system || !system->initialized) return;
    
    if (system->queue) {
        memory_free(system->queue);
        system->queue = NULL;
    }
    
    system->initialized = false;
    LOG_INFO("Asset streaming priority system shutdown");
}

bool asset_priority_add_request(AssetStreamingPriority* system, u64 asset_id, 
                                AssetType type, const char* filepath, Vec3 position) {
    if (!system || !system->initialized || !filepath) return false;
    
    AssetPriorityQueue* queue = system->queue;
    if (queue->count >= queue->capacity) {
        LOG_WARN("Asset priority queue is full, cannot add request");
        return false;
    }
    
    // Check if request already exists
    for (u32 i = 0; i < queue->count; i++) {
        if (queue->requests[i].asset_id == asset_id) {
            LOG_DEBUG("Asset request already exists in queue: %llu", (unsigned long long)asset_id);
            return false;
        }
    }
    
    // Create new request
    AssetRequest* request = &queue->requests[queue->count];
    memset(request, 0, sizeof(AssetRequest));
    
    request->asset_id = asset_id;
    request->type = type;
    request->position = position;
    strncpy(request->filepath, filepath, sizeof(request->filepath) - 1);
    
    // Calculate initial distance and priority
    Vec3 diff = vec3_sub(position, queue->player_position);
    request->distance_from_player = vec3_length(diff);
    request->request_time = time_get_current_ms();
    request->last_access_time = request->request_time;
    request->ref_count = 1;
    
    // Calculate priority
    request->priority = asset_priority_calculate(system, type, request->distance_from_player, 
                                              false, false);
    
    // Add to heap
    queue->count++;
    heapify_up(queue, queue->count - 1);
    
    queue->total_requests++;
    
    LOG_TRACE("Added asset request: %llu (type: %d, distance: %.2f, priority: %d)", 
              (unsigned long long)asset_id, type, request->distance_from_player, request->priority);
    
    return true;
}

bool asset_priority_remove_request(AssetStreamingPriority* system, u64 asset_id) {
    if (!system || !system->initialized) return false;
    
    AssetPriorityQueue* queue = system->queue;
    
    // Find the request
    u32 index = UINT32_MAX;
    for (u32 i = 0; i < queue->count; i++) {
        if (queue->requests[i].asset_id == asset_id) {
            index = i;
            break;
        }
    }
    
    if (index == UINT32_MAX) {
        LOG_DEBUG("Asset request not found for removal: %llu", (unsigned long long)asset_id);
        return false;
    }
    
    // Replace with last element and heapify down
    queue->count--;
    if (index < queue->count) {
        queue->requests[index] = queue->requests[queue->count];
        heapify_down(queue, index);
    }
    
    queue->processed_requests++;
    
    LOG_TRACE("Removed asset request: %llu", (unsigned long long)asset_id);
    return true;
}

AssetRequest* asset_priority_get_next(AssetStreamingPriority* system) {
    if (!system || !system->initialized) return NULL;
    
    AssetPriorityQueue* queue = system->queue;
    if (queue->count == 0) return NULL;
    
    // Return highest priority request (root of heap)
    return &queue->requests[0];
}

void asset_priority_update_queue(AssetStreamingPriority* system, Vec3 player_pos, Vec3 player_vel) {
    if (!system || !system->initialized) return;
    
    AssetPriorityQueue* queue = system->queue;
    u64 current_time = time_get_current_ms();
    
    // Check if we should update (rate limiting)
    if (current_time - queue->last_update_time < PRIORITY_UPDATE_INTERVAL_MS) {
        return;
    }
    
    queue->player_position = player_pos;
    queue->player_velocity = player_vel;
    queue->last_update_time = current_time;
    
    // Update all requests
    for (u32 i = 0; i < queue->count; i++) {
        AssetRequest* request = &queue->requests[i];
        
        // Update distance
        Vec3 diff = vec3_sub(request->position, player_pos);
        request->distance_from_player = vec3_length(diff);
        
        // Recalculate priority
        AssetPriority old_priority = request->priority;
        request->priority = asset_priority_calculate(system, request->type, 
                                                   request->distance_from_player,
                                                   request->is_visible, request->is_preload);
        
        // Update access time if priority changed significantly
        if (old_priority != request->priority) {
            request->last_access_time = current_time;
        }
    }
    
    // Rebuild heap (simple approach - could be optimized)
    for (i32 i = (queue->count / 2) - 1; i >= 0; i--) {
        heapify_down(queue, (u32)i);
    }
    
    queue->priority_updates++;
    
    LOG_TRACE("Updated asset priority queue with %u requests", queue->count);
}

AssetPriority asset_priority_calculate(AssetStreamingPriority* system, 
                                        AssetType type, f32 distance, 
                                        bool is_visible, bool is_preload) {
    if (!system) return PRIORITY_LOW;
    
    // Distance-based priority
    if (distance <= system->critical_distance) {
        return PRIORITY_CRITICAL;
    } else if (distance <= system->high_distance) {
        return PRIORITY_HIGH;
    } else if (distance <= system->medium_distance) {
        return PRIORITY_MEDIUM;
    } else if (distance <= system->preload_distance) {
        return is_preload ? PRIORITY_LOW : PRIORITY_BACKGROUND;
    } else {
        return PRIORITY_BACKGROUND;
    }
}

f32 asset_priority_calculate_score(AssetStreamingPriority* system, AssetRequest* request) {
    if (!system || !request) return 0.0f;
    
    AssetPriorityQueue* queue = system->queue;
    PriorityFactors* factors = &queue->factors;
    
    // Distance score (lower distance = higher score)
    f32 distance_score = 1.0f - (request->distance_from_player / system->preload_distance);
    distance_score = (distance_score < 0.0f) ? 0.0f : distance_score;
    
    // Visibility score
    f32 visibility_score = request->is_visible ? 1.0f : 0.3f;
    
    // Type score
    f32 type_score = system->type_priorities[request->type];
    
    // Velocity score (assets in direction of movement get higher score)
    f32 velocity_score = 0.5f; // Default
    if (vec3_length(queue->player_velocity) > 0.1f) {
        Vec3 to_asset = vec3_normalize(vec3_sub(request->position, queue->player_position));
        Vec3 player_dir = vec3_normalize(queue->player_velocity);
        f32 dot = vec3_dot(to_asset, player_dir);
        velocity_score = (dot + 1.0f) * 0.5f; // Convert from [-1,1] to [0,1]
    }
    
    // Urgency score (based on age and reference count)
    u64 current_time = time_get_current_ms();
    u64 age_ms = current_time - request->request_time;
    f32 age_score = (age_ms < 5000) ? 1.0f : 0.5f; // Higher score for recent requests
    f32 ref_score = (request->ref_count > 0) ? 1.0f : 0.5f;
    f32 urgency_score = (age_score + ref_score) * 0.5f;
    
    // Calculate weighted score
    f32 score = (distance_score * factors->distance_weight) +
                (visibility_score * factors->visibility_weight) +
                (type_score * factors->type_weight) +
                (velocity_score * factors->velocity_weight) +
                (urgency_score * factors->urgency_weight);
    
    return score;
}

void asset_priority_set_factors(AssetStreamingPriority* system, PriorityFactors* factors) {
    if (!system || !system->initialized || !factors) return;
    
    system->queue->factors = *factors;
    
    LOG_INFO("Updated asset priority factors");
}

bool asset_priority_is_queue_full(AssetStreamingPriority* system) {
    if (!system || !system->initialized) return true;
    
    return system->queue->count >= system->queue->capacity;
}

void asset_priority_clear_queue(AssetStreamingPriority* system) {
    if (!system || !system->initialized) return;
    
    system->queue->count = 0;
    memset(system->queue->requests, 0, sizeof(system->queue->requests));
    
    LOG_INFO("Cleared asset priority queue");
}

u32 asset_priority_get_queue_size(AssetStreamingPriority* system) {
    if (!system || !system->initialized) return 0;
    
    return system->queue->count;
}

void asset_priority_get_stats(AssetStreamingPriority* system, u32* total_requests, 
                             u32* processed, f32* hit_rate) {
    if (!system || !system->initialized) return;
    
    if (total_requests) *total_requests = system->queue->total_requests;
    if (processed) *processed = system->queue->processed_requests;
    if (hit_rate) *hit_rate = system->hit_rate;
}

void asset_priority_print_queue(AssetStreamingPriority* system) {
    if (!system || !system->initialized) return;
    
    AssetPriorityQueue* queue = system->queue;
    
    LOG_INFO("=== Asset Priority Queue ===");
    LOG_INFO("Queue size: %u / %u", queue->count, queue->capacity);
    LOG_INFO("Total requests: %u", queue->total_requests);
    LOG_INFO("Processed requests: %u", queue->processed_requests);
    LOG_INFO("Priority updates: %u", queue->priority_updates);
    
    for (u32 i = 0; i < queue->count && i < 10; i++) { // Show top 10
        AssetRequest* req = &queue->requests[i];
        LOG_INFO("  [%u] Asset: %llu, Type: %d, Priority: %d, Distance: %.2f, File: %s",
                 i, (unsigned long long)req->asset_id, req->type, req->priority, req->distance_from_player, req->filepath);
    }
    
    if (queue->count > 10) {
        LOG_INFO("  ... and %u more", queue->count - 10);
    }
}

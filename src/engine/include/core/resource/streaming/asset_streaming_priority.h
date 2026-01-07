#ifndef ASSET_STREAMING_PRIORITY_H
#define ASSET_STREAMING_PRIORITY_H

#include "include/core/types.h"
#include "../math/vec3.h"

#define MAX_PRIORITY_QUEUE_SIZE 512
#define PRIORITY_UPDATE_INTERVAL_MS 100  // Update priorities every 100ms

// Asset types for priority calculation
typedef enum {
    ASSET_TYPE_TEXTURE = 0,
    ASSET_TYPE_MESH,
    ASSET_TYPE_AUDIO,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_ANIMATION,
    ASSET_TYPE_SHADER,
    ASSET_TYPE_COUNT
} AssetType;

// Priority levels
typedef enum {
    PRIORITY_CRITICAL = 0,   // Immediate loading (UI, player equipment)
    PRIORITY_HIGH,           // Near player, visible soon
    PRIORITY_MEDIUM,         // Medium distance, might be needed
    PRIORITY_LOW,            // Far distance, optional
    PRIORITY_BACKGROUND      // Background loading only
} AssetPriority;

// Asset priority factors
typedef struct {
    f32 distance_weight;     // How much distance affects priority (0.0-1.0)
    f32 visibility_weight;   // How much visibility affects priority (0.0-1.0)
    f32 type_weight;         // How much asset type affects priority (0.0-1.0)
    f32 velocity_weight;     // How much player velocity affects priority (0.0-1.0)
    f32 urgency_weight;      // How much time sensitivity affects priority (0.0-1.0)
} PriorityFactors;

// Asset request for priority queue
typedef struct {
    u64 asset_id;
    AssetType type;
    AssetPriority priority;
    Vec3 position;
    f32 distance_from_player;
    f32 visibility_score;
    u64 request_time;
    u64 last_access_time;
    bool is_visible;
    bool is_preload;
    u32 ref_count;
    char filepath[256];
} AssetRequest;

// Priority queue implementation
typedef struct {
    AssetRequest requests[MAX_PRIORITY_QUEUE_SIZE];
    u32 count;
    u32 capacity;
    PriorityFactors factors;
    
    // Player tracking for distance-based priority
    Vec3 player_position;
    Vec3 player_velocity;
    u64 last_update_time;
    
    // Statistics
    u32 total_requests;
    u32 processed_requests;
    u32 priority_updates;
    f32 average_processing_time;
} AssetPriorityQueue;

// Asset streaming priority system
typedef struct {
    AssetPriorityQueue* queue;
    bool initialized;
    
    // Configuration
    f32 critical_distance;    // Distance for critical priority
    f32 high_distance;        // Distance for high priority
    f32 medium_distance;      // Distance for medium priority
    f32 preload_distance;      // Distance for preloading
    
    // Type-specific priorities
    f32 type_priorities[ASSET_TYPE_COUNT];
    
    // Performance tracking
    u64 total_processing_time;
    u32 requests_per_second;
    f32 hit_rate;            // How often requested assets are already loaded
} AssetStreamingPriority;

// Lifecycle
bool asset_priority_init(AssetStreamingPriority* system);
void asset_priority_shutdown(AssetStreamingPriority* system);

// Priority queue management
bool asset_priority_add_request(AssetStreamingPriority* system, u64 asset_id, 
                                AssetType type, const char* filepath, Vec3 position);
bool asset_priority_remove_request(AssetStreamingPriority* system, u64 asset_id);
AssetRequest* asset_priority_get_next(AssetStreamingPriority* system);
void asset_priority_update_queue(AssetStreamingPriority* system, Vec3 player_pos, Vec3 player_vel);

// Priority calculation
AssetPriority asset_priority_calculate(AssetStreamingPriority* system, 
                                        AssetType type, f32 distance, 
                                        bool is_visible, bool is_preload);
f32 asset_priority_calculate_score(AssetStreamingPriority* system, AssetRequest* request);
void asset_priority_set_factors(AssetStreamingPriority* system, PriorityFactors* factors);

// Utility functions
bool asset_priority_is_queue_full(AssetStreamingPriority* system);
void asset_priority_clear_queue(AssetStreamingPriority* system);
u32 asset_priority_get_queue_size(AssetStreamingPriority* system);

// Statistics and debugging
void asset_priority_get_stats(AssetStreamingPriority* system, u32* total_requests, 
                             u32* processed, f32* hit_rate);
void asset_priority_print_queue(AssetStreamingPriority* system);

#endif // ASSET_STREAMING_PRIORITY_H

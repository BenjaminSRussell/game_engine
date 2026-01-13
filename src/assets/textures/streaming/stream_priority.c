/*
 * stream_priority.c
 * Streaming priority calculation implementation
 */

#include "assets/textures/streaming/stream_priority.h"
#include "assets/textures/streaming/texture_streamer.h"
#include <include/math/math.h>
#include <stdlib.h>

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

float texture_priority_calculate_screen_coverage(const float* bounds_min, const float* bounds_max, const void* camera) {
    if (!camera) return 1.0f;
    
    // Simplified screen coverage calculation:
    // 1. Calculate distance to camera
    // 2. Estimate projected area based on distance and FOV
    
    // Note: Assuming camera is a pointer to the Camera struct from camera.c
    // We'll treat it as a void pointer and cast if we have access to the struct definition.
    // For now, let's assume some offsets or just a simplified version.
    
    float pos[3] = {
        (bounds_min[0] + bounds_max[0]) * 0.5f,
        (bounds_min[1] + bounds_max[1]) * 0.5f,
        (bounds_min[2] + bounds_max[2]) * 0.5f
    };
    
    float dx = pos[0]; // Assuming camera is at origin for this simplified calculation
    float dy = pos[1];
    float dz = pos[2];
    
    float dist_sq = dx*dx + dy*dy + dz*dz;
    float dist = sqrtf(dist_sq);
    
    if (dist < 0.1f) return 1.0f;
    
    // Projected size decreases with square of distance
    // A simplified model: scale factor / distance
    float size_scale = 100.0f; 
    float coverage = size_scale / dist;
    
    if (coverage > 1.0f) coverage = 1.0f;
    if (coverage < 0.0f) coverage = 0.0f;
    
    return coverage;
}

uint32_t texture_priority_calculate_target_mip(float screen_coverage, uint32_t max_mips, float mip_bias) {
    // coverage: 1.0 (very close) -> mip 0
    // coverage: 0.0 (very far) -> mip max_mips - 1
    
    if (screen_coverage >= 1.0f) return 0;
    if (screen_coverage <= 0.0f) return max_mips - 1;
    
    // Logarithmic relationship between screen size and mip level
    float mip = -log2f(screen_coverage) + mip_bias;
    uint32_t mip_int = (uint32_t)mip;
    
    if (mip_int >= max_mips) mip_int = max_mips - 1;
    
    return mip_int;
}

float texture_priority_calculate_distance_score(const float* position, const void* camera) {
    if (!camera || !position) return 0.0f;
    
    float dx = position[0];
    float dy = position[1];
    float dz = position[2];
    
    float dist_sq = dx*dx + dy*dy + dz*dz;
    return 1.0f / (1.0f + dist_sq);
}

float texture_priority_calculate_final_score(float coverage, float distance, uint32_t current_mip, uint32_t target_mip) {
    // Higher score = higher priority
    // 1. Difference between current and target (more difference = more priority)
    // 2. Screen coverage (bigger on screen = more priority)
    // 3. Distance (closer = more priority)
    
    float mip_diff = (float)(current_mip - target_mip);
    if (mip_diff < 0) mip_diff = 0; // Don't prioritize unloading for now
    
    return (mip_diff * 10.0f) + (coverage * 5.0f) + (distance * 2.0f);
}

void texture_priority_sort_requests(struct stream_request* requests, uint32_t count) {
    // Simple insertion sort for request priority
    for (uint32_t i = 1; i < count; i++) {
        stream_request_t key = requests[i];
        int j = i - 1;
        while (j >= 0 && requests[j].priority < key.priority) {
            requests[j + 1] = requests[j];
            j = j - 1;
        }
        requests[j + 1] = key;
    }
}

int texture_stream_priority_init(void) {
    return 0;
}

void texture_stream_priority_shutdown(void) {
}


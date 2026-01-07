//
// lod_selection_compute.metal
// GPU-driven LOD selection compute shader
//
// Part of the Geometry subsystem
// Advanced 3D Rendering Engine
//

#include <metal_stdlib>
using namespace metal;

// Camera data
struct CameraData {
    float3 position;
    float fov_y;
    float4x4 view_matrix;
    float4x4 proj_matrix;
    float aspect_ratio;
    float near_plane;
    float far_plane;
    uint viewport_width;
    uint viewport_height;
};

// Per-instance data
struct InstanceData {
    float3 position;
    float radius;
    uint chain_id;
    float last_distance;
    uint current_lod;
    uint padding;
};

// LOD selection result
struct LODResult {
    uint selected_lod;
    float distance;
    float screen_coverage;
    uint padding;
};

// LOD thresholds (could be per-chain)
struct LODThresholds {
    float distances[8];
    float screen_sizes[8];
    uint lod_count;
};

// Compute shader for LOD selection
kernel void lod_selection_compute(
    device const InstanceData* instances [[buffer(0)]],
    device LODResult* results [[buffer(1)]],
    constant CameraData& camera [[buffer(2)]],
    constant LODThresholds& thresholds [[buffer(3)]],
    uint gid [[thread_position_in_grid]]
) {
    // Bounds check
    if (gid >= 4096) return; // Max instances
    
    const InstanceData inst = instances[gid];
    
    // Calculate distance to camera
    float3 to_camera = inst.position - camera.position;
    float distance = length(to_camera);
    
    // Calculate screen-space coverage
    // Project bounding sphere to screen using perspective projection
    float tan_half_fov = tan(camera.fov_y * 0.5f);
    float screen_height_pixels = (inst.radius * float(camera.viewport_height)) / 
                                 (distance * tan_half_fov + 0.001f);
    float screen_coverage = screen_height_pixels / float(camera.viewport_height);
    
    // Select LOD based on screen coverage
    uint selected_lod = 0;
    
    for (uint i = 0; i < thresholds.lod_count; i++) {
        if (screen_coverage >= thresholds.screen_sizes[i]) {
            selected_lod = i;
            break;
        }
    }
    
    // Hysteresis: require significant change to switch LOD
    float distance_change = abs(distance - inst.last_distance);
    float hysteresis_threshold = 0.1f; // 10% change required
    
    if (distance_change < distance * hysteresis_threshold) {
        // Not enough change, keep current LOD
        selected_lod = inst.current_lod;
    }
    
    // Write results
    results[gid].selected_lod = selected_lod;
    results[gid].distance = distance;
    results[gid].screen_coverage = screen_coverage;
    results[gid].padding = 0;
}

// Alternative: Distance-based selection
kernel void lod_selection_distance(
    device const InstanceData* instances [[buffer(0)]],
    device LODResult* results [[buffer(1)]],
    constant CameraData& camera [[buffer(2)]],
    constant LODThresholds& thresholds [[buffer(3)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid >= 4096) return;
    
    const InstanceData inst = instances[gid];
    
    float3 to_camera = inst.position - camera.position;
    float distance = length(to_camera);
    
    // Select LOD based on distance thresholds
    uint selected_lod = thresholds.lod_count - 1; // Start with lowest detail
    
    for (uint i = 0; i < thresholds.lod_count; i++) {
        if (distance < thresholds.distances[i]) {
            selected_lod = i;
            break;
        }
    }
    
    // Apply hysteresis
    float distance_change = abs(distance - inst.last_distance);
    if (distance_change < distance * 0.1f) {
        selected_lod = inst.current_lod;
    }
    
    results[gid].selected_lod = selected_lod;
    results[gid].distance = distance;
    results[gid].screen_coverage = 0.0f; // Not calculated in distance mode
    results[gid].padding = 0;
}

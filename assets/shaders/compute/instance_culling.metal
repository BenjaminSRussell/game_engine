//
// instance_culling.metal
// GPU-driven instance culling compute shader
//
// Part of the Geometry subsystem
// Advanced 3D Rendering Engine
//

#include <metal_stdlib>
using namespace metal;

/* ============================================================================
 * STRUCTURES
 * ============================================================================ */

// Instance data (must match CPU-side struct)
struct InstanceData {
    float4x4 transform;             // 64 bytes
    float4 base_color;              // 16 bytes
    float roughness;                // 4 bytes
    float metallic;                 // 4 bytes
    float emission;                 // 4 bytes
    uint texture_index;             // 4 bytes
    uint padding[3];                // 12 bytes (align to 16)
    uint visibility_flags;          // 4 bytes
    uint mesh_id;                   // 4 bytes
    uint material_id;               // 4 bytes
    uint custom_data[2];            // 8 bytes
};

// Frustum plane
struct FrustumPlane {
    float4 coefficients; // (a, b, c, d) for ax + by + cz + d = 0
};

// Camera frustum (6 planes)
struct CameraFrustum {
    FrustumPlane planes[6];
};

// Culling configuration
struct CullingConfig {
    bool enable_frustum_culling;    // 4 bytes
    bool enable_occlusion_culling;  // 4 bytes
    bool enable_distance_culling;   // 4 bytes
    bool enable_backface_culling;   // 4 bytes
    
    float near_distance;            // 4 bytes
    float far_distance;             // 4 bytes
    float lod0_distance;            // 4 bytes
    float lod1_distance;            // 4 bytes
    float lod2_distance;            // 4 bytes
    
    uint max_visible_instances;     // 4 bytes
    float3 camera_position;         // 12 bytes
    uint padding;                   // 4 bytes (align to 16)
};

// Culling statistics
struct CullingStats {
    atomic_uint total_instances;
    atomic_uint visible_instances;
    atomic_uint frustum_culled;
    atomic_uint occlusion_culled;
    atomic_uint distance_culled;
    atomic_uint backface_culled;
};

/* ============================================================================
 * CULLING FUNCTIONS
 * ============================================================================ */

// Test if point is inside frustum
bool test_point_frustum(float3 point, constant CameraFrustum& frustum) {
    for (int i = 0; i < 6; i++) {
        float4 plane = frustum.planes[i].coefficients;
        float distance = dot(plane.xyz, point) + plane.w;
        if (distance < 0.0) {
            return false; // Outside this plane
        }
    }
    return true;
}

// Test if sphere is inside frustum
bool test_sphere_frustum(float3 center, float radius, constant CameraFrustum& frustum) {
    for (int i = 0; i < 6; i++) {
        float4 plane = frustum.planes[i].coefficients;
        float distance = dot(plane.xyz, center) + plane.w;
        if (distance < -radius) {
            return false; // Sphere completely outside this plane
        }
    }
    return true;
}

// Extract bounding sphere from instance transform
void extract_bounding_sphere(constant InstanceData& instance, 
                            thread float3& out_center, 
                            thread float& out_radius) {
    // Extract position from transform matrix (last column)
    out_center = instance.transform[3].xyz;
    
    // Calculate radius from scale (approximate)
    float3 scale;
    scale.x = length(instance.transform[0].xyz);
    scale.y = length(instance.transform[1].xyz);
    scale.z = length(instance.transform[2].xyz);
    
    // Use maximum scale component as radius
    out_radius = max(max(scale.x, scale.y), scale.z);
}

// Calculate distance from camera to instance
float calculate_distance(float3 instance_position, float3 camera_position) {
    return length(instance_position - camera_position);
}

// Determine LOD level based on distance
uint calculate_lod_level(float distance, constant CullingConfig& config) {
    if (distance < config.lod0_distance) {
        return 0;
    } else if (distance < config.lod1_distance) {
        return 1;
    } else if (distance < config.lod2_distance) {
        return 2;
    } else {
        return 3; // Beyond LOD 2
    }
}

/* ============================================================================
 * MAIN CULLING KERNEL
 * ============================================================================ */

kernel void instance_culling_kernel(
    constant InstanceData* instances [[buffer(0)]],
    device uint* visible_instance_ids [[buffer(1)]],
    device atomic_uint* visible_count [[buffer(2)]],
    constant CameraFrustum& frustum [[buffer(3)]],
    constant CullingConfig& config [[buffer(4)]],
    device CullingStats* stats [[buffer(5)]],
    uint gid [[thread_position_in_grid]],
    uint total_instances [[threads_per_grid]])
{
    // Check bounds
    if (gid >= total_instances) {
        return;
    }
    
    // Get instance data
    constant InstanceData& instance = instances[gid];
    
    // Update total count
    atomic_fetch_add_explicit(&stats->total_instances, 1, memory_order_relaxed);
    
    // Check visibility flag
    bool visible = (instance.visibility_flags & 0x1) != 0;
    if (!visible) {
        return;
    }
    
    // Extract bounding sphere
    float3 center;
    float radius;
    extract_bounding_sphere(instance, center, radius);
    
    // === FRUSTUM CULLING ===
    if (config.enable_frustum_culling) {
        if (!test_sphere_frustum(center, radius, frustum)) {
            atomic_fetch_add_explicit(&stats->frustum_culled, 1, memory_order_relaxed);
            return;
        }
    }
    
    // === DISTANCE CULLING ===
    float distance = 0.0;
    if (config.enable_distance_culling) {
        distance = calculate_distance(center, config.camera_position);
        
        // Cull if too far
        if (distance > config.far_distance) {
            atomic_fetch_add_explicit(&stats->distance_culled, 1, memory_order_relaxed);
            return;
        }
        
        // Cull if too near
        if (distance < config.near_distance) {
            atomic_fetch_add_explicit(&stats->distance_culled, 1, memory_order_relaxed);
            return;
        }
    }
    
    // === BACKFACE CULLING (for large instances) ===
    if (config.enable_backface_culling) {
        float3 view_dir = normalize(center - config.camera_position);
        float3 forward = instance.transform[2].xyz; // Z-axis
        
        // If instance is facing away from camera
        if (dot(view_dir, forward) > 0.0) {
            atomic_fetch_add_explicit(&stats->backface_culled, 1, memory_order_relaxed);
            return;
        }
    }
    
    // === OCCLUSION CULLING ===
    // TODO: Implement Hi-Z occlusion culling in future enhancement
    if (config.enable_occlusion_culling) {
        // Placeholder for occlusion test
        // Would sample Hi-Z buffer and compare depths
    }
    
    // Instance is visible - add to output
    uint output_index = atomic_fetch_add_explicit(visible_count, 1, memory_order_relaxed);
    
    // Check capacity
    if (output_index < config.max_visible_instances) {
        visible_instance_ids[output_index] = gid;
        atomic_fetch_add_explicit(&stats->visible_instances, 1, memory_order_relaxed);
    }
}

/* ============================================================================
 * STATS RESET KERNEL
 * ============================================================================ */

kernel void reset_culling_stats(
    device CullingStats* stats [[buffer(0)]],
    device atomic_uint* visible_count [[buffer(1)]])
{
    // Reset all counters
    atomic_store_explicit(&stats->total_instances, 0, memory_order_relaxed);
    atomic_store_explicit(&stats->visible_instances, 0, memory_order_relaxed);
    atomic_store_explicit(&stats->frustum_culled, 0, memory_order_relaxed);
    atomic_store_explicit(&stats->occlusion_culled, 0, memory_order_relaxed);
    atomic_store_explicit(&stats->distance_culled, 0, memory_order_relaxed);
    atomic_store_explicit(&stats->backface_culled, 0, memory_order_relaxed);
    
    atomic_store_explicit(visible_count, 0, memory_order_relaxed);
}

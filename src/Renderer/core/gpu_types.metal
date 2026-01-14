#ifndef RENDERING_GPU_TYPES_METAL_H
#define RENDERING_GPU_TYPES_METAL_H

#include <metal_stdlib>
using namespace metal;

// ============================================================================
// GPU INSTANCE DATA
// ============================================================================

struct GPUInstanceData {
    float4x4 transform;                 // 4x4 matrix
    float4 bounds_min;                  // AABB min
    float4 bounds_max;                  // AABB max
    uint material_id;
    uint visibility_flags;
    uint mesh_id;
    uint instance_custom_data;
};

// ============================================================================
// GPU MATERIAL DATA
// ============================================================================

struct GPUMaterialData {
    float4 base_color;
    float4 metallic_roughness_ao;
    uint texture_indices[8];
};

// ============================================================================
// INDIRECT DRAW ARGUMENTS
// ============================================================================

struct IndirectDrawArgs {
    uint indexCount;
    uint instanceCount;
    uint indexStart;
    int baseVertex;
    uint baseInstance;
};

// ============================================================================
// CULLING DATA
// ============================================================================

struct FrustumData {
    float planes[6][4];
    float3 camera_pos;
    float padding;
};

struct LODSelectionData {
    float lod_distances[5];
    float lod_hysteresis;
    uint max_lod_level;
    uint padding;
};

// ============================================================================
// CULLING RESULTS
// ============================================================================

struct CullingResult {
    uint visible_flag;
    uint lod_selected;
    uint draw_index;
    uint padding;
};

// ============================================================================
// GPU COUNTERS
// ============================================================================

struct GPUCounters {
    atomic_uint visible_instance_count;
    atomic_uint draw_call_count;
    atomic_uint compaction_offset;
    uint frame_number;
};

// ============================================================================
// FRUSTUM CULLING UTILITIES
// ============================================================================

// Test if a point is inside the frustum
inline bool point_in_frustum(float3 point, constant FrustumData& frustum) {
    for (int i = 0; i < 6; ++i) {
        float4 plane = float4(frustum.planes[i][0], frustum.planes[i][1],
                             frustum.planes[i][2], frustum.planes[i][3]);
        float distance = dot(float4(point, 1.0), plane);
        if (distance < 0.0) {
            return false;
        }
    }
    return true;
}

// Test if AABB intersects frustum using separating axis theorem
inline bool aabb_in_frustum(float3 center, float3 extents, constant FrustumData& frustum) {
    for (int i = 0; i < 6; ++i) {
        float3 normal = float3(frustum.planes[i][0], frustum.planes[i][1], frustum.planes[i][2]);
        float d = frustum.planes[i][3];

        float r = dot(extents, abs(normal));
        float s = dot(normal, center) + d;

        if (s + r < 0.0) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// DISTANCE-BASED LOD SELECTION
// ============================================================================

inline uint select_lod(float distance, constant LODSelectionData& lod_data) {
    for (uint i = 0; i < 5; ++i) {
        if (distance < lod_data.lod_distances[i]) {
            return i;
        }
    }
    return min(4u, lod_data.max_lod_level);
}

inline uint select_lod_with_hysteresis(float distance, uint current_lod,
                                        constant LODSelectionData& lod_data) {
    float hysteresis = lod_data.lod_hysteresis;

    // Check if we should change LOD
    uint new_lod = select_lod(distance, lod_data);

    // Apply hysteresis to prevent popping
    if (new_lod > current_lod) {
        // Only go to coarser LOD if distance is significantly further
        float threshold = lod_data.lod_distances[current_lod] * (1.0 + hysteresis);
        if (distance < threshold) {
            new_lod = current_lod;
        }
    } else if (new_lod < current_lod) {
        // Only go to finer LOD if distance is significantly closer
        float threshold = lod_data.lod_distances[new_lod] * (1.0 - hysteresis);
        if (distance > threshold) {
            new_lod = current_lod;
        }
    }

    return new_lod;
}

#endif // RENDERING_GPU_TYPES_METAL_H

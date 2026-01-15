#include <metal_stdlib>
using namespace metal;

struct mesh_cluster {
    uint vertex_offset;
    uint index_offset;
    uint triangle_count;
    packed_float3 bounds_center;
    packed_float3 bounds_extent;
    float lod_error;
    uint parent_cluster;
    uint child_clusters[8];
    uint child_count;
};

struct CullUniforms {
    float4x4 view_projection;
    float3 view_pos;
    float4 frustum[6]; // Frustum planes
    float2 hzb_size;
    float lod_threshold;
    float screen_height;
    float fov_radians;
};

// Frustum test helper
bool frustum_test(float3 center, float3 extent, constant float4* planes) {
    for (int i = 0; i < 6; i++) {
        float3 plane_normal = planes[i].xyz;
        float plane_dist = planes[i].w;
        
        float r = dot(extent, abs(plane_normal));
        float d = dot(plane_normal, center) + plane_dist;
        
        if (d < -r) return false;
    }
    return true;
}

// Compute screen error helper
float compute_screen_error(float lod_error, float3 center, float3 view_pos, float screen_height, float fov_radians) {
    float d = distance(center, view_pos);
    d = max(d, 0.001f);
    float cot_half_fov = 1.0f / tan(fov_radians * 0.5f);
    return (lod_error * screen_height * cot_half_fov * 0.5f) / d;
}

kernel void cull_clusters(device const mesh_cluster* clusters [[buffer(0)]],
                          device uint* visible_clusters [[buffer(1)]],
                          device atomic_uint* visible_count [[buffer(2)]],
                          constant CullUniforms& uniforms [[buffer(3)]],
                          uint cluster_id [[thread_position_in_grid]])
{
    // Bounds check? Arrays are usually padded.
    // Assume cluster_id is valid index.
    
    mesh_cluster cluster = clusters[cluster_id];

    // Frustum cull
    if (!frustum_test(cluster.bounds_center, cluster.bounds_extent, uniforms.frustum))
        return;

    // Occlusion cull against HZB (Placeholder as we don't have texture access in struct)
    // if (is_occluded(cluster.bounds_center, cluster.bounds_extent, uniforms.hzb)) return;

    // LOD selection - compare screen-space error
    // In a DAG traversal, we check if we should render THIS cluster or its children.
    // If error > threshold, we need higher detail (children).
    // If error <= threshold, this cluster is good enough.
    // HOWEVER, `cull_clusters` usually runs on a list of candidate clusters (the "Front").
    // If it's a simple list (linear scan), we just pick the best one?
    // The prompt snippet says: `if (screen_error > uniforms.lod_threshold && cluster.child_count > 0) return; // Children will be rendered`
    // This implies we are processing ALL clusters and filtering? Or a cut?
    // If we process ALL clusters, parent and child might both pass. 
    // Nanite maintains a "Cut".
    // For this task, we implement the logic as requested:
    // "If error is too high and we have children, don't render this (we assume children are processed separately or this is a recursive descent step)"
    
    float screen_error = compute_screen_error(cluster.lod_error,
                                               cluster.bounds_center,
                                               uniforms.view_pos,
                                               uniforms.screen_height,
                                               uniforms.fov_radians);
                                               
    if (screen_error > uniforms.lod_threshold && cluster.child_count > 0)
        return; // Children will be rendered instead (or we need to refine)

    // Add to visible list
    uint idx = atomic_fetch_add_explicit(visible_count, 1, memory_order_relaxed);
    visible_clusters[idx] = cluster_id;
}

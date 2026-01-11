#include <metal_stdlib>
using namespace metal;

struct BoundingBox {
    float3 center;
    float3 extent;
};

struct Cluster {
    uint vertex_offset;
    uint index_offset;
    uint triangle_count;
    float3 bounds_center;
    float3 bounds_extent;
    float lod_error;
    uint parent_cluster;
    uint child_clusters[8];
    uint child_count;
};

struct ViewArgs {
    float4x4 view_projection;
    float3 view_position;
    float view_resolution; // screen height in pixels
    float4 frustum_planes[6];
};

struct CullResult {
    atomic_uint visible_count;
    // visible_clusters buffer follows
};

// Check if sphere is inside frustum
bool is_sphere_in_frustum(float3 center, float radius, constant float4* planes) {
    for (int i = 0; i < 6; i++) {
        if (dot(planes[i].xyz, center) + planes[i].w < -radius) {
            return false;
        }
    }
    return true;
}

// Check if AABB is inside frustum
bool is_aabb_in_frustum(float3 center, float3 extent, constant float4* planes) {
    for (int i = 0; i < 6; i++) {
        float3 normal = planes[i].xyz;
        float dist = planes[i].w;
        float r = dot(extent, abs(normal));
        float s = dot(center, normal) + dist;
        if (s + r < 0) return false;
    }
    return true;
}

// Compute Shader for Cluster Culling
kernel void cull_clusters(
    device const Cluster* clusters [[ buffer(0) ]],
    device uint* visible_clusters [[ buffer(1) ]],
    device atomic_uint* params [[ buffer(2) ]], // [0] = visible_count
    constant ViewArgs& args [[ buffer(3) ]],
    uint id [[ thread_position_in_grid ]]
) {
    // Basic iterative culling would require a persistent thread approach or 
    // a multi-pass approach (traverse level by level).
    // For V1, we just do a linear sweep over all clusters (brute force)
    // and check if they are "renderable" (error <= threshold OR leaf)
    
    // NOTE: In a real persistent-thread Nanite implementation, we would 
    // traverse the DAG. Here we assume 'clusters' contains ALL clusters.
    
    // We treat every cluster as a candidate and verify the LOD condition.
    // This is O(N) where N=total_clusters, appearing redundant but acts as a simple GPU culler.
    // Optimization: Only process roots and expand? 
    // Let's stick to the brute force linear check implementation for now 
    // as it maps 1:1 with the thread dispatch.
    
    // To properly support LOD, we should know if a parent selected us.
    // But since we are stateless here, we implement the "Selection" logic:
    // A cluster is selected if:
    // 1. It is visible
    // 2. Its error is small enough
    // 3. Its parent's error was TOO BIG (implying we refined to this level)
    
    device const Cluster& c = clusters[id];
    
    // 1. Frustum Cull
    if (!is_aabb_in_frustum(c.bounds_center, c.bounds_extent, args.frustum_planes)) {
        return;
    }
    
    // 2. Compute Error
    float sphere_radius = length(c.bounds_extent);
    float dist = distance(c.bounds_center, args.view_position);
    if (dist < sphere_radius) dist = sphere_radius + 0.001f;
    
    float screen_error = (c.lod_error * args.view_resolution) / dist;
    
    // 3. LOD Selection Logic
    // We render this cluster IF:
    // (Error fits) AND (Parent Error didn't fit OR I am root)
    
    bool error_fits = (screen_error <= 1.0f);
    bool parent_error_failed = true;
    
    if (c.parent_cluster != 0xFFFFFFFF) {
        device const Cluster& parent = clusters[c.parent_cluster];
        float p_dist = distance(parent.bounds_center, args.view_position);
        float p_error = (parent.lod_error * args.view_resolution) / (p_dist + 0.001f);
        if (p_error <= 1.0f) {
            parent_error_failed = false; // Parent was good enough, so we shouldn't render (parent should render)
        }
    }
    
    // If I am a leaf, I must render if my parent failed, even if my error is bad (fallback)
    bool is_leaf = (c.child_count == 0);
    
    if (parent_error_failed && (error_fits || is_leaf)) {
        // Append to visible list
        uint index = atomic_fetch_add_explicit(params, 1, memory_order_relaxed);
        visible_clusters[index] = id;
    }
}

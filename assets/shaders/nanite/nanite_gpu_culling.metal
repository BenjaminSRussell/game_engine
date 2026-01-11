// geometry/nanite/nanite_gpu_culling.metal
// GPU-driven cluster culling compute shader
#include <metal_stdlib>
using namespace metal;

struct ClusterBounds {
    float3 min_bound;
    float3 max_bound;
    float3 cone_apex;
    float3 cone_axis;
    float cone_angle;
    uint lod_level;
    float lod_error;
};

struct CullParams {
    float4x4 view_proj;
    float3 camera_pos;
    float2 screen_size;
    float near_plane;
    float far_plane;
};

struct DrawCommand {
    uint vertex_count;
    uint instance_count;
    uint first_vertex;
    uint first_instance;
};

// Frustum planes in world space
constant float4 frustum_planes[6] = {
    // Calculated from view_proj matrix
};

// Test AABB against frustum
bool is_aabb_in_frustum(float3 min_bound, float3 max_bound, constant CullParams& params) {
    // Test each frustum plane
    for (int i = 0; i < 6; i++) {
        float3 positive_vertex = select(min_bound, max_bound, frustum_planes[i].xyz > 0);
        if (dot(float4(positive_vertex, 1.0), frustum_planes[i]) < 0) {
            return false; // Outside frustum
        }
    }
    return true;
}

// Test backface cone culling
bool is_cluster_backfacing(float3 cone_apex, float3 cone_axis, float cone_angle,
                          float3 camera_pos) {
    float3 to_camera = normalize(camera_pos - cone_apex);
    float angle = dot(to_camera, cone_axis);
    return angle > cos(cone_angle); // Cluster fully backfacing
}

// Calculate screen space LOD
uint calculate_lod(float3 bounds_center, float lod_error, constant CullParams& params) {
    float dist = length(params.camera_pos - bounds_center);
    float screen_size_error = (lod_error * params.screen_size.y) / dist;
    
    // Select LOD based on screen space error
    if (screen_size_error < 1.0) return 4; // Lowest LOD
    if (screen_size_error < 2.0) return 3;
    if (screen_size_error < 4.0) return 2;
    if (screen_space_error < 8.0) return 1;
    return 0; // Highest detail
}

kernel void nanite_cull_clusters(
    constant ClusterBounds* clusters [[buffer(0)]],
    constant CullParams& params [[buffer(1)]],
    device atomic_uint* visible_count [[buffer(2)]],
    device DrawCommand* draw_commands [[buffer(3)]],
    device uint* visible_cluster_ids [[buffer(4)]],
    uint cluster_id [[thread_position_in_grid]],
    uint num_clusters [[threads_per_grid]])
{
    if (cluster_id >= num_clusters) return;
    
    const ClusterBounds cluster = clusters[cluster_id];
    
    // Frustum culling
    if (!is_aabb_in_frustum(cluster.min_bound, cluster.max_bound, params)) {
        return; // Culled
    }
    
    // Backface culling
    if (is_cluster_backfacing(cluster.cone_apex, cluster.cone_axis, 
                             cluster.cone_angle, params.camera_pos)) {
        return; // Culled
    }
    
    // LOD selection
    float3 center = (cluster.min_bound + cluster.max_bound) * 0.5;
    uint selected_lod = calculate_lod(center, cluster.lod_error, params);
    
    if (selected_lod != cluster.lod_level) {
        return; // Wrong LOD for this distance
    }
    
    // Passed all culling tests - add to visible list
    uint index = atomic_fetch_add_explicit(visible_count, 1, memory_order_relaxed);
    visible_cluster_ids[index] = cluster_id;
}

// Second pass: generate draw commands from visible clusters
kernel void nanite_generate_draw_commands(
    constant ClusterBounds* clusters [[buffer(0)]],
    device uint* visible_cluster_ids [[buffer(1)]],
    device uint* visible_count [[buffer(2)]],
    device DrawCommand* draw_commands [[buffer(3)]],
    uint index [[thread_position_in_grid]])
{
    uint count = *visible_count;
    if (index >= count) return;
    
    uint cluster_id = visible_cluster_ids[index];
    const ClusterBounds cluster = clusters[cluster_id];
    
    // Generate indirect draw command
    DrawCommand cmd;
    cmd.vertex_count = 128 * 3; // 128 triangles
    cmd.instance_count = 1;
    cmd.first_vertex = cluster_id * 128 * 3;
    cmd.first_instance = 0;
    
    draw_commands[index] = cmd;
}

// Nanite Rendering Shaders
#include <metal_stdlib>
using namespace metal;

struct ClusterData {
    float3 bounds_center;
    float bounds_radius;
    uint vertex_offset;
    uint triangle_count;
    uint lod_level;
    uint parent_index;
};

struct VertexOut {
    float4 position [[position]];
    float3 normal;
    float2 uv;
    float3 world_pos;
};

// Vertex shader
vertex VertexOut nanite_vertex(
    uint vertex_id [[vertex_id]],
    constant float3* positions [[buffer(0)]],
    constant float3* normals [[buffer(1)]],
    constant float2* uvs [[buffer(2)]],
    constant float4x4& view_proj [[buffer(3)]],
    constant uint& vertex_offset [[buffer(4)]]
) {
    VertexOut out;
    
    uint idx = vertex_offset + vertex_id;
    float3 pos = positions[idx];
    
    out.position = view_proj * float4(pos, 1.0);
    out.normal = normals[idx];
    out.uv = uvs[idx];
    out.world_pos = pos;
    
    return out;
}

// Fragment shader  
fragment float4 nanite_fragment(
    VertexOut in [[stage_in]],
    constant float3& light_dir [[buffer(0)]]
) {
    // Simple lighting
    float3 normal = normalize(in.normal);
    float ndotl = max(dot(normal, -light_dir), 0.0);
    
    float3 base_color = float3(0.8, 0.8, 0.8);
    float3 lit_color = base_color * (0.3 + 0.7 * ndotl);
    
    return float4(lit_color, 1.0);
}

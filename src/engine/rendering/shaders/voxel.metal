#include <metal_stdlib>
using namespace metal;

struct VoxelVertex {
    float3 position [[attribute(0)]];
    float3 normal   [[attribute(1)]];
    float2 uv       [[attribute(2)]];
    uchar  ao       [[attribute(3)]];
    uchar  light    [[attribute(4)]];
    ushort block_id [[attribute(5)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 normal;
    float2 uv;
    float  ao;
    float  light;
    uint   block_id;
};

struct Uniforms {
    float4x4 model_view_projection;
};

vertex VertexOut voxel_vertex_main(VoxelVertex v [[stage_in]],
                                  constant Uniforms &u [[buffer(1)]]) {
    VertexOut out;
    out.position = u.model_view_projection * float4(v.position, 1.0);
    out.normal = v.normal;
    out.uv = v.uv;
    out.ao = float(v.ao) / 255.0;
    out.light = float(v.light) / 255.0;
    out.block_id = uint(v.block_id);
    return out;
}

fragment float4 voxel_fragment_main(VertexOut in [[stage_in]]) {
    // Basic coloring for now based on block_id
    float3 color = float3(0.5);
    if (in.block_id == 1) color = float3(0.3, 0.3, 0.3); // Stone
    if (in.block_id == 2) color = float3(0.1, 0.8, 0.2); // Grass
    if (in.block_id == 3) color = float3(0.5, 0.3, 0.1); // Dirt
    
    // Apply basic AO and light
    color *= in.ao * in.light;
    
    return float4(color, 1.0);
}

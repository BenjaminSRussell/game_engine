#include <metal_stdlib>
using namespace metal;

struct SkinVertex {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
    uint4 bone_indices;
    float4 bone_weights;
};

struct Vertex {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
};

kernel void compute_skinning(device const SkinVertex* input [[buffer(0)]],
                             device Vertex* output [[buffer(1)]],
                             device const float4x4* bones [[buffer(2)]],
                             uint vid [[thread_position_in_grid]])
{
    SkinVertex v = input[vid];

    // Calculate skinning matrix
    float4x4 skin_matrix = bones[v.bone_indices.x] * v.bone_weights.x
                         + bones[v.bone_indices.y] * v.bone_weights.y
                         + bones[v.bone_indices.z] * v.bone_weights.z
                         + bones[v.bone_indices.w] * v.bone_weights.w;

    // Transform position
    output[vid].position = (skin_matrix * float4(v.position, 1.0)).xyz;
    
    // Transform normal (assuming uniform scaling, otherwise use inverse transpose)
    output[vid].normal = normalize((skin_matrix * float4(v.normal, 0.0)).xyz);
    
    // Transform tangent
    output[vid].tangent = float4(normalize((skin_matrix * float4(v.tangent.xyz, 0.0)).xyz),
                                  v.tangent.w);
    
    // Pass through UV
    output[vid].uv = v.uv;
}

// shaders/pbr.metal
#include <metal_stdlib>
using namespace metal;

// Function constants for permutations
constant bool HAS_NORMAL_MAP [[function_constant(0)]];
constant bool HAS_EMISSIVE [[function_constant(1)]];

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 texcoord [[attribute(2)]];
    float4 tangent [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 world_pos;
    float3 normal;
    float2 texcoord;
    float3 tangent;
    float3 bitangent;
};

struct Uniforms {
    float4x4 model;
    float4x4 view_proj;
};

vertex VertexOut pbr_vertex(VertexIn in [[stage_in]],
                            constant Uniforms& uniforms [[buffer(1)]]) {
    VertexOut out;
    float4 world = uniforms.model * float4(in.position, 1.0);
    out.position = uniforms.view_proj * world;
    out.world_pos = world.xyz;
    out.normal = normalize((uniforms.model * float4(in.normal, 0.0)).xyz);
    out.texcoord = in.texcoord;

    if (HAS_NORMAL_MAP) {
        // Tangent w component typically flips bitangent
        out.tangent = normalize((uniforms.model * float4(in.tangent.xyz, 0.0)).xyz);
        out.bitangent = cross(out.normal, out.tangent) * in.tangent.w;
    }

    return out;
}

fragment float4 pbr_fragment(VertexOut in [[stage_in]],
                              texture2d<float> albedo_tex [[texture(0)]],
                              texture2d<float> normal_tex [[texture(1), function_constant(HAS_NORMAL_MAP)]],
                              texture2d<float> metallic_roughness_tex [[texture(2)]],
                              texture2d<float> emissive_tex [[texture(3), function_constant(HAS_EMISSIVE)]],
                              sampler samp [[sampler(0)]]) {
    float4 albedo = albedo_tex.sample(samp, in.texcoord);

    float3 N = normalize(in.normal);
    if (HAS_NORMAL_MAP) {
        float3 T = normalize(in.tangent);
        float3 B = normalize(in.bitangent);
        float3x3 TBN = float3x3(T, B, N);
        float3 normal_sample = normal_tex.sample(samp, in.texcoord).xyz * 2.0 - 1.0;
        N = normalize(TBN * normal_sample);
    }

    float4 mr = metallic_roughness_tex.sample(samp, in.texcoord);
    // float metallic = mr.b; 
    // float roughness = mr.g;

    // ... PBR lighting ...
    // Placeholder simple lighting
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    float diff = max(dot(N, lightDir), 0.0);
    float3 color = albedo.rgb * diff; 

    if (HAS_EMISSIVE) {
        color += emissive_tex.sample(samp, in.texcoord).rgb;
    }

    return float4(color, albedo.a);
}

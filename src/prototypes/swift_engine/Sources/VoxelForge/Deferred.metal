#include <metal_stdlib>
#include "ShaderTypes.h"

using namespace metal;

struct GBufferOutput {
    float4 albedo [[color(GBufferTextureIndexAlbedo)]];
    float4 normal [[color(GBufferTextureIndexNormal)]];
    float  depth  [[color(GBufferTextureIndexDepth)]];
};

vertex RasterizerData gBufferVertex(uint vertexID [[vertex_id]],
                                   uint instanceID [[instance_id]],
                                   constant Vertex *vertices [[buffer(0)]],
                                   constant Uniforms &uniforms [[buffer(1)]],
                                   constant InstanceData *instances [[buffer(2)]]) {
    RasterizerData out;
    Vertex v = vertices[vertexID];
    InstanceData i = instances[instanceID];
    
    float4 worldPosition = float4(v.position + i.instancePosition, 1.0);
    out.position = uniforms.modelViewProjectionMatrix * worldPosition;
    out.color = i.instanceColor;
    out.texCoord = float2(v.position.x + 0.5, 1.0 - (v.position.y + 0.5));
    out.textureIndex = i.textureIndex;
    
    return out;
}

fragment GBufferOutput gBufferFragment(RasterizerData in [[stage_in]],
                                      array<texture2d<float>, 128> textureHeap [[buffer(0)]]) {
    constexpr sampler textureSampler (mag_filter::nearest, min_filter::nearest);
    float4 albedo = textureHeap[in.textureIndex].sample(textureSampler, in.texCoord) * in.color;
    
    GBufferOutput out;
    out.albedo = albedo;
    out.normal = float4(0, 1, 0, 1); // Simple flat normal for voxels for now
    out.depth = in.position.z;
    return out;
}

// MARK: - Lighting Pass

struct LightingAttributes {
    float4 position [[position]];
    float2 texCoord;
};

vertex LightingAttributes lightingVertex(uint vertexID [[vertex_id]]) {
    // Fullscreen quad
    const float2 positions[4] = {
        float2(-1, -1), float2(1, -1),
        float2(-1,  1), float2(1,  1)
    };
    const float2 uvs[4] = {
        float2(0, 1), float2(1, 1),
        float2(0, 0), float2(1, 0)
    };
    
    LightingAttributes out;
    out.position = float4(positions[vertexID], 0, 1);
    out.texCoord = uvs[vertexID];
    return out;
}

fragment float4 lightingFragment(LightingAttributes in [[stage_in]],
                                texture2d<float> albedoTex [[texture(GBufferTextureIndexAlbedo)]],
                                texture2d<float> normalTex [[texture(GBufferTextureIndexNormal)]],
                                texture2d<float> depthTex  [[texture(GBufferTextureIndexDepth)]],
                                constant Light &light [[buffer(0)]]) {
    
    float4 albedo = albedoTex.sample(sampler(filter::nearest), in.texCoord);
    float3 normal = normalTex.sample(sampler(filter::nearest), in.texCoord).xyz;
    
    if (albedo.a == 0) discard_fragment();
    
    // Simple Lambertian lighting for now (PBR math to be expanded)
    float3 L = normalize(light.position);
    float diffuse = max(dot(normal, L), 0.1f);
    
    return float4(albedo.rgb * light.color * diffuse * light.intensity, 1.0);
}

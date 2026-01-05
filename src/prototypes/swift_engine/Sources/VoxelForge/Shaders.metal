#include <metal_stdlib>
#include "ShaderTypes.h"

using namespace metal;

struct RasterizerData {
    float4 position [[position]];
    float4 color;
    float2 texCoord;
    uint textureIndex;
};

vertex RasterizerData vertexShader(uint vertexID [[vertex_id]],
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
    out.textureIndex = i.textureIndex; // Pass index to fragment shader
    
    return out;
}

fragment float4 fragmentShader(RasterizerData in [[stage_in]],
                               array<texture2d<float>, 128> textureHeap [[buffer(0)]]) {
    constexpr sampler textureSampler (mag_filter::nearest,
                                      min_filter::nearest);
    
    // Sample the texture from the heap using the instance's index
    float4 colorSample = textureHeap[in.textureIndex].sample(textureSampler, in.texCoord);
    
    return colorSample * in.color;
}

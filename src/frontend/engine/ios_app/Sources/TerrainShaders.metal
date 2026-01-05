#include <metal_stdlib>
using namespace metal;

struct TerrainVertex {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
    float3 normal [[attribute(2)]];
};

struct Uniforms {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float3 normal;
    float3 worldPosition;
};

vertex VertexOut terrain_vertex(
    const device TerrainVertex* vertices [[buffer(0)]],
    constant Uniforms& uniforms [[buffer(1)]],
    uint vid [[vertex_id]]
) {
    VertexOut out;
    float4 position = float4(vertices[vid].position, 1.0);
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * uniforms.modelMatrix * position;
    out.color = vertices[vid].color;
    out.normal = (uniforms.modelMatrix * float4(vertices[vid].normal, 0.0)).xyz;
    out.worldPosition = (uniforms.modelMatrix * position).xyz;
    return out;
}

fragment float4 terrain_fragment(VertexOut in [[stage_in]]) {
    // Simple directional lighting
    float3 lightDir = normalize(float3(0.5, -1.0, 0.5));
    float3 normal = normalize(in.normal);
    float diffuse = max(0.2, dot(normal, -lightDir));
    
    // Height-based atmospheric fog placeholder
    float fog = saturate(in.worldPosition.y * 0.05);
    float3 finalColor = mix(in.color.rgb, float3(0.8, 0.9, 1.0), fog);
    
    return float4(finalColor * diffuse, 1.0);
}

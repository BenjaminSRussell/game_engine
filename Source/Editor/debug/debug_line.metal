#include <metal_stdlib>
using namespace metal;

struct DebugLine {
    float3 start;
    float4 start_color;
    float3 end;
    float4 end_color;
};

struct DebugUniforms {
    float4x4 view_proj;
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

vertex VertexOut vertex_debug(
    uint vertexID [[vertex_id]],
    constant DebugLine* lines [[buffer(0)]],
    constant DebugUniforms& uniforms [[buffer(1)]]
) {
    VertexOut out;
    
    // Each line has 2 vertices (0 and 1)
    // line index = vertexID / 2
    // endpoint index = vertexID % 2
    
    uint lineIndex = vertexID / 2;
    uint pointIndex = vertexID % 2;
    
    DebugLine line = lines[lineIndex];
    
    float3 pos = (pointIndex == 0) ? line.start : line.end;
    float4 col = (pointIndex == 0) ? line.start_color : line.end_color;
    
    out.position = uniforms.view_proj * float4(pos, 1.0);
    out.color = col;
    
    return out;
}

fragment float4 fragment_debug(VertexOut in [[stage_in]]) {
    return in.color;
}

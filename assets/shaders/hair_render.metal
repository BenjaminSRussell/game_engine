#include <metal_stdlib>
using namespace metal;

struct HairControlPoint {
    float3 position;
    float inv_mass;
    float3 velocity;
    float thickness;
    float4 tangent;
};

struct VertexOut {
    float4 position [[position]];
    float3 world_pos;
    float3 tangent;
    float2 uv;
    float thickness;
};

// Vertex Shader
vertex VertexOut hair_vertex(const device HairControlPoint* points [[buffer(0)]],
                             constant float4x4& view_proj [[buffer(1)]],
                             uint vid [[vertex_id]]) {
    VertexOut out;
    
    device const HairControlPoint& p = points[vid];
    
    out.position = view_proj * float4(p.position, 1.0);
    out.world_pos = p.position;
    out.tangent = p.tangent.xyz;
    
    // UV could be based on strand ID or segment index if passed, 
    // but we don't have it easily in simple vertex fetch without encoding it.
    // For now, dummy UV.
    out.uv = float2(0,0); 
    out.thickness = p.thickness;
    
    return out;
}

// Kajiya-Kay Lighting Helper
float strand_specular(float3 T, float3 V, float3 L, float exponent) {
    float3 H = normalize(L + V);
    float dotTH = dot(T, H);
    float sinTH = sqrt(1.0 - dotTH * dotTH);
    float dirAtten = smoothstep(-1.0, 0.0, dotTH);
    return dirAtten * pow(sinTH, exponent);
}

// Fragment Shader
fragment float4 hair_fragment(VertexOut in [[stage_in]],
                              texture2d<float> density_map [[texture(0)]]) {
    
    float3 V = normalize(float3(0,0,1)); // View vector (simplified, should be passed)
    float3 L = normalize(float3(0.5, 1.0, 0.5)); // Light vector (simplified)
    float3 T = normalize(in.tangent);
    
    // Base Color
    float3 base_color = float3(0.4, 0.25, 0.1); // Brown hair
    
    // Kajiya-Kay
    // Diffuse-like
    float diffuse = clamp(sin(acos(dot(T, L))), 0.0, 1.0);
    
    // Specular
    // Shift tangents for highlights
    float3 T1 = normalize(T + float3(0.1, 0, 0)); // Shift for primary
    float3 T2 = normalize(T - float3(0.2, 0, 0)); // Shift for secondary
    
    float spec1 = strand_specular(T1, V, L, 40.0);
    float spec2 = strand_specular(T2, V, L, 20.0);
    
    float3 color = base_color * diffuse + 
                   float3(1.0) * spec1 * 0.4 + // White primary
                   base_color * spec2 * 0.3;   // Colored secondary
    
    return float4(color, 1.0);
}

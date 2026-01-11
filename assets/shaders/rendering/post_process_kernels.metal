// shader_common.metal
// Standard library for all custom shaders
#include <metal_stdlib>
using namespace metal;

// Common Math
#define PI 3.14159265359

// Random
float rand(float2 co) {
    return fract(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

// Tonemapping - ACES
float3 aces_tonemap(float3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Compute Kernels for Post Process
kernel void pp_tonemap(
    texture2d<float, access::read> input [[texture(0)]],
    texture2d<float, access::write> output [[texture(1)]],
    constant struct { float exposure; float contrast; }& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= output.get_width() || gid.y >= output.get_height()) return;
    
    float3 color = input.read(gid).rgb;
    
    // Exposure
    color *= params.exposure;
    
    // Tonemap
    color = aces_tonemap(color);
    
    // Gamma correction
    color = pow(color, 1.0/2.2);
    
    output.write(float4(color, 1.0), gid);
}

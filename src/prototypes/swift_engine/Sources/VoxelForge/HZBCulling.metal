// HZB Culling Compute Shader
#include <metal_stdlib>
using namespace metal;

struct HZBUniforms {
    float4x4 viewProjection;
    uint2 hzbSize;
};

kernel void hzbCulling(
    device const float4* worldPositions [[buffer(0)]],
    device const float* hzbBuffer [[buffer(1)]],
    device atomic_uint* visibleIndices [[buffer(2)]],
    constant HZBUniforms& uniforms [[buffer(3)]],
    uint id [[thread_position_in_grid]]
) {
    // 1. Transform world position to clip space
    // 2. Sample HZB buffer at appropriate mip level
    // 3. Occlusion test: if z > hzb_z, then occluded
    // 4. If visible, add to visibleIndices
}

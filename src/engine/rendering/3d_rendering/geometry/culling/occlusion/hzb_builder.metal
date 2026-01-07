#include <metal_stdlib>
using namespace metal;

/**
 * Hierarchical Z-Buffer (HZB) Builder
 * 
 * Generates a mipmap pyramid from the depth buffer where each mip level
 * contains the minimum (furthest) depth of a 2x2 region from the previous level.
 * This is used for efficient ray marching in screen-space reflections.
 */

kernel void hzb_reduce(
    texture2d<float, access::read> input_depth [[texture(0)]],
    texture2d<float, access::write> output_depth [[texture(1)]],
    uint2 gid [[thread_position_in_grid]]
) {
    // Each thread reads a 2x2 region from input and writes min depth to output
    uint2 input_coord = gid * 2;
    
    // Get dimensions to handle edge cases
    uint2 input_size = uint2(input_depth.get_width(), input_depth.get_height());
    
    // Sample 2x2 region (with bounds checking)
    float d00 = input_depth.read(input_coord).r;
    
    float d10 = (input_coord.x + 1 < input_size.x) ? 
                input_depth.read(input_coord + uint2(1, 0)).r : d00;
    
    float d01 = (input_coord.y + 1 < input_size.y) ? 
                input_depth.read(input_coord + uint2(0, 1)).r : d00;
    
    float d11 = (input_coord.x + 1 < input_size.x && input_coord.y + 1 < input_size.y) ? 
                input_depth.read(input_coord + uint2(1, 1)).r : d00;
    
    // Conservative depth reduction (furthest/max for reverse-Z, nearest/min for forward-Z)
    // Assuming reverse-Z (1.0 = near, 0.0 = far), we want MAX
    // For forward-Z (0.0 = near, 1.0 = far), we want MIN
    // Most modern engines use reverse-Z, so using max here
    float min_depth = max(max(d00, d10), max(d01, d11));
    
    output_depth.write(float4(min_depth, 0, 0, 0), gid);
}

/**
 * Single-pass HZB generation with mip level parameter
 * More efficient when generating multiple levels
 */
kernel void hzb_reduce_mip(
    texture2d<float, access::read> depth_pyramid [[texture(0)]],
    texture2d<float, access::write> output_mip [[texture(1)]],
    constant uint& source_mip [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint2 input_coord = gid * 2;
    
    // Sample from specified mip level
    constexpr sampler s(coord::pixel, filter::nearest, address::clamp_to_edge);
    
    float d00 = depth_pyramid.read(input_coord, source_mip).r;
    float d10 = depth_pyramid.read(input_coord + uint2(1, 0), source_mip).r;
    float d01 = depth_pyramid.read(input_coord + uint2(0, 1), source_mip).r;
    float d11 = depth_pyramid.read(input_coord + uint2(1, 1), source_mip).r;
    
    // Conservative max for reverse-Z
    float max_depth = max(max(d00, d10), max(d01, d11));
    
    output_mip.write(float4(max_depth, 0, 0, 0), gid);
}

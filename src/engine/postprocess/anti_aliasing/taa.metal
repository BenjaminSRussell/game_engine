#include <metal_stdlib>
using namespace metal;

struct TAAUniforms {
    uint2 screen_size;
};

// TAA resolve shader:
kernel void taa_resolve(
    texture2d<float, access::read> current [[texture(0)]],
    texture2d<float, access::read> history [[texture(1)]],
    texture2d<float, access::read> velocity [[texture(2)]],
    texture2d<float, access::read> depth [[texture(3)]],
    texture2d<float, access::write> output [[texture(4)]],
    constant TAAUniforms& uniforms [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= uniforms.screen_size.x || gid.y >= uniforms.screen_size.y) {
        return;
    }

    float2 uv = (float2(gid) + 0.5) / float2(uniforms.screen_size);

    // Sample current frame
    float4 current_color = current.read(gid);

    // Get velocity
    // Assuming 2D velocity is stored in xy
    float2 vel = velocity.read(gid).xy;

    // Reproject to previous frame
    float2 history_uv = uv - vel;

    // Check if reprojection is valid
    if (history_uv.x < 0.0 || history_uv.x > 1.0 || history_uv.y < 0.0 || history_uv.y > 1.0) {
        output.write(current_color, gid);
        return;
    }

    // Sample history with bilinear
    sampler linear_sampler(coord::normalized, address::clamp_to_edge, filter::linear);
    float4 history_color = history.sample(linear_sampler, history_uv);

    // Neighborhood clamping (variance clip)
    float4 neighbor_min = current_color;
    float4 neighbor_max = current_color;

    // 3x3 neighborhood
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            uint2 neighbor_pos = gid + uint2(x, y);
            // Clamp neighbor position to screen bounds to avoid out of bounds read?
            // texture read usually handles this or returns 0, but good to be safe or rely on clamp_to_edge if sampling
            // Using read() with uint2, we should be careful. 
            // Ideally we clamp to width/height-1. 
            // For simplicity and speed in this block, we'll assume texture has border or acceptable behavior,
            // but let's do a safe clamp if possible or just rely on the variance loop being robust enough.
            // Actually, let's just clamp the coordinates found.
             neighbor_pos.x = clamp(neighbor_pos.x, 0u, uniforms.screen_size.x - 1);
             neighbor_pos.y = clamp(neighbor_pos.y, 0u, uniforms.screen_size.y - 1);
            
            float4 neighbor = current.read(neighbor_pos);
            neighbor_min = min(neighbor_min, neighbor);
            neighbor_max = max(neighbor_max, neighbor);
        }
    }

    // Variance clip
    float4 mu = (neighbor_min + neighbor_max) * 0.5;
    float4 sigma = (neighbor_max - neighbor_min) * 0.5;
    float4 aabb_min = mu - sigma * 1.5;
    float4 aabb_max = mu + sigma * 1.5;

    history_color = clamp(history_color, aabb_min, aabb_max);

    // Blend
    float blend_factor = 0.1;  // Favor history for stability

    // Increase blend factor for fast motion or disoccluded areas
    // Velocity is usually in UV space [-1, 1] or similar. Assuming normalized UV space delta.
    // If velocity is large, history is less reliable.
    float motion_weight = saturate(length(vel) * 10.0); // Tuning factor 10.0
    blend_factor = mix(blend_factor, 0.5, motion_weight);

    float4 result = mix(history_color, current_color, blend_factor);
    output.write(result, gid);
}

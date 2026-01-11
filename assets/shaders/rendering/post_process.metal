// rendering/post_process.metal
// Advanced Post-Process Compute Kernels (TAA, SSR)
#include <metal_stdlib>
using namespace metal;

struct TAAParams {
    float jitter_x;
    float jitter_y;
    float blend_factor;
};

struct SSRParams {
    float max_distance;
    float stride;
    float thickness;
    int max_steps;
};

// ==========================================
// TAA Resolve Logic (History Clamping + Blend)
// ==========================================
kernel void pp_taa_resolve(
    texture2d<float, access::read> current_frame [[texture(0)]],
    texture2d<float, access::read> history_frame [[texture(1)]],
    texture2d<float, access::read> velocity_tex [[texture(2)]],
    texture2d<float, access::read> depth_tex [[texture(3)]],
    texture2d<float, access::write> output [[texture(4)]],
    constant TAAParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= output.get_width() || gid.y >= output.get_height()) return;
    
    // 1. Sample current color and neighborhood for clamping
    float3 color_center = current_frame.read(gid).rgb;
    float3 color_min = color_center;
    float3 color_max = color_center;
    
    // 3x3 Neighborhood clamp (Simple AABB)
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            uint2 sample_pos = uint2(int2(gid) + int2(x, y));
            // Bounds check
            if (sample_pos.x < output.get_width() && sample_pos.y < output.get_height()) {
                float3 neighbor = current_frame.read(sample_pos).rgb;
                color_min = min(color_min, neighbor);
                color_max = max(color_max, neighbor);
            }
        }
    }
    
    // 2. Reproject history using velocity
    float2 velocity = velocity_tex.read(gid).rg; // Screen space velocity
    float2 uv = (float2(gid) + 0.5) / float2(output.get_width(), output.get_height());
    float2 prev_uv = uv - velocity;
    
    float3 history = float3(0.0);
    float blend = params.blend_factor;
    
    // Check if history is valid (inside screen)
    if (prev_uv.x >= 0.0 && prev_uv.x <= 1.0 && prev_uv.y >= 0.0 && prev_uv.y <= 1.0) {
        // Sample history (Bilinear interpolation would be better here, using read with sampler in real engine)
        uint2 history_pos = uint2(prev_uv * float2(output.get_width(), output.get_height()));
        history = history_frame.read(history_pos).rgb;
        
        // 3. Clamp/Clip history to current neighborhood (reduces ghosting)
        // Simple clamp:
        history = clamp(history, color_min, color_max);
    } else {
        // History invalid, use current
        blend = 1.0; 
        history = color_center;
    }
    
    // 4. Resolve
    float3 result = mix(history, color_center, blend);
    output.write(float4(result, 1.0), gid);
}

// ==========================================
// SSR (Screen Space Reflections) Raymarching
// ==========================================
kernel void pp_ssr_trace(
    texture2d<float, access::read> scene_color [[texture(0)]],
    texture2d<float, access::read> depth_tex [[texture(1)]],
    texture2d<float, access::read> normal_tex [[texture(2)]],
    texture2d<float, access::write> output [[texture(3)]],
    constant float4x4& projection [[buffer(0)]],
    constant float4x4& inv_projection [[buffer(1)]],
    constant SSRParams& params [[buffer(2)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= output.get_width() || gid.y >= output.get_height()) return;

    // 1. Reconstruct View Space Position
    float depth = depth_tex.read(gid).r;
    float2 uv = (float2(gid) + 0.5) / float2(output.get_width(), output.get_height());
    float4 clip_pos = float4(uv * 2.0 - 1.0, depth, 1.0);
    clip_pos.y = -clip_pos.y; // Metal coordinates
    float4 view_pos_unhomogenized = inv_projection * clip_pos;
    float3 view_pos = view_pos_unhomogenized.xyz / view_pos_unhomogenized.w;

    // 2. Reflect View Vector
    float3 normal_world = normal_tex.read(gid).xyz * 2.0 - 1.0; // Assuming world space normals in G-buffer
    float3 view_dir = normalize(view_pos); // Camera is at 0,0,0 in view space
    
    // Transform normal to view space (simplified, assumes view matrix is identity for rotation/scale or passed in)
    // For now assume standard deferred normals
    float3 reflect_dir = reflect(view_dir, normal_world);

    // 3. Raymarch
    // Linear search step
    float3 ray_pos = view_pos;
    float3 ray_step = reflect_dir * params.stride;
    
    float4 hit_color = float4(0.0);
    
    for (int i = 0; i < params.max_steps; i++) {
        ray_pos += ray_step;
        
        // Project ray pos to screen space to sample depth buffer
        float4 ray_clip = projection * float4(ray_pos, 1.0);
        float3 ray_ndc = ray_clip.xyz / ray_clip.w;
        
        if (ray_ndc.x < -1 || ray_ndc.x > 1 || ray_ndc.y < -1 || ray_ndc.y > 1) break; // Off screen
        
        float2 sample_uv = ray_ndc.xy * 0.5 + 0.5;
        sample_uv.y = 1.0 - sample_uv.y;
        
        uint2 sample_coords = uint2(sample_uv * float2(output.get_width(), output.get_height()));
        float sample_depth = depth_tex.read(sample_coords).r;
        
        // Linearize depths for comparison (simplified)
        // Check intersection
        float ray_linear_depth = ray_pos.z; // Approximate
        if (ray_linear_depth > sample_depth && ray_linear_depth < sample_depth + params.thickness) {
            // Hit found!
            hit_color = scene_color.read(sample_coords);
            // Distance fade (fresnel etc)
            hit_color.a *= (1.0 - float(i)/params.max_steps);
            break;
        }
    }
    
    // 4. Output
    // If no hit, alpha is 0
    output.write(hit_color, gid);
}

#include <metal_stdlib>
using namespace metal;

struct OceanParams {
    uint resolution;
    float tile_size;
    float amplitude;
    float wind_speed;
    float2 wind_direction;
    float chopiness;
};

// --- Spectrum Update ---

kernel void ocean_spectrum_update(
    texture2d<float, access::read> h0 [[texture(0)]],
    texture2d<float, access::write> ht [[texture(1)]],
    constant float& time [[buffer(0)]],
    constant OceanParams& params [[buffer(1)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= params.resolution || gid.y >= params.resolution) return;

    float4 h0_data = h0.read(gid);
    float2 h0k = h0_data.xy;
    float2 h0k_conj = h0_data.zw;

    // Wave vector
    float2 k = (float2(int2(gid)) - float2(params.resolution / 2)) * (2.0 * M_PI / params.tile_size);
    float k_len = length(k);

    // Dispersion relation: omega = sqrt(g * k)
    float omega = sqrt(9.81 * k_len);

    // Time evolution
    float cos_t = cos(omega * time);
    float sin_t = sin(omega * time);

    // h(k,t) = h0(k) * exp(i*omega*t) + h0*(-k) * exp(-i*omega*t)
    float2 ht_k;
    ht_k.x = h0k.x * cos_t - h0k.y * sin_t + h0k_conj.x * cos_t + h0k_conj.y * sin_t;
    ht_k.y = h0k.x * sin_t + h0k.y * cos_t - h0k_conj.x * sin_t + h0k_conj.y * cos_t;

    ht.write(float4(ht_k, 0, 0), gid);
}

// --- FFT (Horizontal & Vertical) ---
// Note: This is a simplified Radix-2 FFT placeholder as requested/referenced.
// In a production engine, you'd use a more optimized butterfly-based FFT.

kernel void ocean_fft_horizontal(
    texture2d<float, access::read> input [[texture(0)]],
    texture2d<float, access::write> output [[texture(1)]],
    constant OceanParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= params.resolution || gid.y >= params.resolution) return;

    // Simplified Inverse FFT (horizontal pass)
    float2 result = float2(0.0);
    for (uint x = 0; x < params.resolution; x++) {
        float2 val = input.read(uint2(x, gid.y)).xy;
        float angle = 2.0 * M_PI * float(x) * float(gid.x) / float(params.resolution);
        float2 twiddle = float2(cos(angle), sin(angle));
        
        // Complex multiplication: (a+bi)(c+di) = (ac-bd) + (ad+bc)i
        result.x += val.x * twiddle.x - val.y * twiddle.y;
        result.y += val.x * twiddle.y + val.y * twiddle.x;
    }
    
    output.write(float4(result / float(params.resolution), 0, 0), gid);
}

kernel void ocean_fft_vertical(
    texture2d<float, access::read> input [[texture(0)]],
    texture2d<float, access::write> output [[texture(1)]],
    constant OceanParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= params.resolution || gid.y >= params.resolution) return;

    // Simplified Inverse FFT (vertical pass)
    float2 result = float2(0.0);
    for (uint y = 0; y < params.resolution; y++) {
        float2 val = input.read(uint2(gid.x, y)).xy;
        float angle = 2.0 * M_PI * float(y) * float(gid.y) / float(params.resolution);
        float2 twiddle = float2(cos(angle), sin(angle));
        
        result.x += val.x * twiddle.x - val.y * twiddle.y;
        result.y += val.x * twiddle.y + val.y * twiddle.x;
    }
    
    output.write(float4(result / float(params.resolution), 0, 0), gid);
}

// --- Displacement & Derivatives ---

kernel void ocean_displacement(
    texture2d<float, access::read> height_map [[texture(0)]],
    texture2d<float, access::write> displacement_derivatives [[texture(1)]],
    constant OceanParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= params.resolution || gid.y >= params.resolution) return;

    // Read height and neighboring heights for normal calculation
    float h = height_map.read(gid).x;
    float h_r = height_map.read(uint2((gid.x + 1) % params.resolution, gid.y)).x;
    float h_u = height_map.read(uint2(gid.x, (gid.y + 1) % params.resolution)).x;

    // Finite difference for normals
    float dx = (h_r - h) * (params.resolution / params.tile_size);
    float dy = (h_u - h) * (params.resolution / params.tile_size);
    
    float3 normal = normalize(float3(-dx, 1.0, -dy));

    // Simple Jacobian for foam
    float jacobian = 1.0 - (dx * dx + dy * dy);
    float foam = saturate(1.0 - jacobian);

    displacement_derivatives.write(float4(normal, foam), gid);
}

// --- Render Shaders ---

struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 world_pos;
    float2 texcoord;
    float3 normal;
    float foam;
};

vertex VertexOut ocean_vertex(
    VertexIn in [[stage_in]],
    texture2d<float, access::sample> displacement_map [[texture(0)]],
    constant float4x4& view_projection [[buffer(1)]],
    constant OceanParams& params [[buffer(2)]]
) {
    float4 disp_raw = displacement_map.read(uint2(in.texcoord * float(params.resolution)));
    float height = disp_raw.x;
    
    float3 pos = in.position;
    pos.y += height * params.amplitude;

    VertexOut out;
    out.position = view_projection * float4(pos, 1.0);
    out.world_pos = pos;
    out.texcoord = in.texcoord;
    // Normal and foam will be sampled in fragment shader from derivatives texture
    return out;
}

fragment float4 ocean_fragment(
    VertexOut in [[stage_in]],
    texture2d<float, access::sample> derivative_map [[texture(0)]],
    constant float3& light_dir [[buffer(0)]],
    constant float3& camera_pos [[buffer(1)]]
) {
    constexpr sampler s(mag_filter::linear, min_filter::linear);
    float4 deriv = derivative_map.sample(s, in.texcoord);
    float3 normal = normalize(deriv.xyz);
    float foam = deriv.w;

    float3 view_dir = normalize(camera_pos - in.world_pos);
    float diffuse = saturate(dot(normal, light_dir));
    
    // Simple ocean shading
    float3 ocean_base = float3(0.0, 0.1, 0.3);
    float3 foam_color = float3(0.8, 0.9, 1.0);
    
    float3 color = mix(ocean_base * (diffuse + 0.2), foam_color, foam);
    
    return float4(color, 1.0);
}

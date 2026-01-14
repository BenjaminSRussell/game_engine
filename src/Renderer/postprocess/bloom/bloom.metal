#include <metal_stdlib>
using namespace metal;

// 13-tap tent filter downsample
// Based on Call of Duty: Advanced Warfare presentation
kernel void bloom_downsample(
    texture2d<float, access::read> src [[texture(0)]],
    texture2d<float, access::write> dst [[texture(1)]],
    constant float& threshold [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= dst.get_width() || gid.y >= dst.get_height()) {
        return;
    }

    float2 texel_size = 1.0 / float2(src.get_width(), src.get_height());
    float2 uv = (float2(gid) + 0.5) * 1.0 / float2(dst.get_width(), dst.get_height()); // Fix UV mapping for downsampling

    // 13-tap filter
    constexpr sampler linear_sampler(coord::normalized, filter::linear, address::clamp_to_edge);
    
    float4 a = src.sample(linear_sampler, uv + float2(-2, -2) * texel_size);
    float4 b = src.sample(linear_sampler, uv + float2( 0, -2) * texel_size);
    float4 c = src.sample(linear_sampler, uv + float2( 2, -2) * texel_size);
    float4 d = src.sample(linear_sampler, uv + float2(-1, -1) * texel_size);
    float4 e = src.sample(linear_sampler, uv + float2( 1, -1) * texel_size);
    float4 f = src.sample(linear_sampler, uv + float2(-2,  0) * texel_size);
    float4 g = src.sample(linear_sampler, uv);
    float4 h = src.sample(linear_sampler, uv + float2( 2,  0) * texel_size);
    float4 i = src.sample(linear_sampler, uv + float2(-1,  1) * texel_size);
    float4 j = src.sample(linear_sampler, uv + float2( 1,  1) * texel_size);
    float4 k = src.sample(linear_sampler, uv + float2(-2,  2) * texel_size);
    float4 l = src.sample(linear_sampler, uv + float2( 0,  2) * texel_size);
    float4 m = src.sample(linear_sampler, uv + float2( 2,  2) * texel_size);

    float4 color = (d + e + i + j) * 0.5 * 0.25 +
                   (a + b + g + f) * 0.25 * 0.25 +
                   (b + c + h + g) * 0.25 * 0.25 +
                   (f + g + l + k) * 0.25 * 0.25 +
                   (g + h + m + l) * 0.25 * 0.25;

    // Apply threshold on first mip
    if (threshold > 0) {
        float brightness = max(color.r, max(color.g, color.b));
        float soft = brightness - threshold + 0.01;
        soft = clamp(soft, 0.0, 2.0 * 0.01);
        soft = soft * soft / (4.0 * 0.01 + 0.0001);
        float contribution = max(soft, brightness - threshold) / max(brightness, 0.0001);
        color.rgb *= contribution;
    }

    dst.write(color, gid);
}

// 3x3 tent filter upsample logic with accumulation
kernel void bloom_upsample(
    texture2d<float, access::sample> src [[texture(0)]], // Low res
    texture2d<float, access::read_write> dst [[texture(1)]], // High res (input/output)
    constant float& radius [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= dst.get_width() || gid.y >= dst.get_height()) {
        return;
    }

    float2 texel_size = 1.0 / float2(src.get_width(), src.get_height());
    float2 uv = (float2(gid) + 0.5) * 1.0 / float2(dst.get_width(), dst.get_height());

    constexpr sampler linear_sampler(coord::normalized, filter::linear, address::clamp_to_edge);

    // 9-tap tent filter
    float4 d = src.sample(linear_sampler, uv + float2(-1.0, -1.0) * texel_size * radius);
    float4 e = src.sample(linear_sampler, uv + float2( 0.0, -1.0) * texel_size * radius);
    float4 f = src.sample(linear_sampler, uv + float2( 1.0, -1.0) * texel_size * radius);
    
    float4 g = src.sample(linear_sampler, uv + float2(-1.0,  0.0) * texel_size * radius);
    float4 h = src.sample(linear_sampler, uv);
    float4 i = src.sample(linear_sampler, uv + float2( 1.0,  0.0) * texel_size * radius);
    
    float4 j = src.sample(linear_sampler, uv + float2(-1.0,  1.0) * texel_size * radius);
    float4 k = src.sample(linear_sampler, uv + float2( 0.0,  1.0) * texel_size * radius);
    float4 l = src.sample(linear_sampler, uv + float2( 1.0,  1.0) * texel_size * radius);

    float4 bloom = e * 0.0625 + f * 0.0625 +
                   g * 0.0625 + h * 0.25   + i * 0.0625 +
                   j * 0.0625 + k * 0.0625;
                   
    bloom = (d + e * 2.0 + f) +
            (g * 2.0 + h * 4.0 + i * 2.0) +
            (j + k * 2.0 + l);
    bloom *= 0.0625;

    float4 current = dst.read(gid);
    dst.write(current + bloom, gid);
}

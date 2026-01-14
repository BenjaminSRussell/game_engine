#include <metal_stdlib>
using namespace metal;

// ACES tonemapping
float3 aces_tonemap(float3 color) {
    float3x3 m1 = float3x3(
        0.59719, 0.35458, 0.04823,
        0.07600, 0.90834, 0.01566,
        0.02840, 0.13383, 0.83777
    );
    float3x3 m2 = float3x3(
        1.60475, -0.53108, -0.07367,
        -0.10208, 1.10813, -0.00605,
        -0.00327, -0.07276, 1.07602
    );

    float3 v = m1 * color;
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return m2 * (a / b);
}

// Histogram computation
kernel void tonemapping_histogram(
    texture2d<float, access::read> src [[texture(0)]],
    device atomic_uint* histogram [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= src.get_width() || gid.y >= src.get_height()) {
        return;
    }

    float4 color = src.read(gid);
    // Calculate luminance (Rec. 709)
    float luminance = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
    
    // Convert to log domain
    // const float min_log_lum = -10.0f;
    // const float max_log_lum = 4.0f; // Approx
    
    // Simplified mapping for now, should match exposure settings
    if (luminance < 0.0001) luminance = 0.0001;
    float log_lum = log2(luminance);
    
    // Map to [0, 255] bin
    // Assuming range [-10, 10]
    float normalized = (log_lum + 10.0) / 20.0;
    uint bin = clamp(uint(normalized * 255.0), 0u, 255u);
    
    atomic_fetch_add_explicit(&histogram[bin], 1, memory_order_relaxed);
}

// Tonemap application
kernel void tonemapping_apply(
    texture2d<float, access::read> hdr_input [[texture(0)]],
    texture2d<float, access::read> bloom [[texture(1)]],
    texture2d<float, access::write> ldr_output [[texture(2)]],
    constant float& exposure [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= ldr_output.get_width() || gid.y >= ldr_output.get_height()) {
        return;
    }

    float4 c_hdr = hdr_input.read(gid);
    float4 c_bloom = bloom.sample(sampler(filter::linear), (float2(gid) + 0.5) / float2(ldr_output.get_width(), ldr_output.get_height()));

    // Combine
    float3 combined = c_hdr.rgb + c_bloom.rgb;

    // Apply exposure
    combined *= exposure;

    // Tonemap
    combined = aces_tonemap(combined);

    // Gamma correction
    combined = pow(combined, float3(1.0/2.2));

    ldr_output.write(float4(combined, 1.0), gid);
}

// Exposure adaptation kernel
kernel void tonemapping_adapt_exposure(
    device atomic_uint* histogram [[buffer(0)]],
    device float* exposure_buffer [[buffer(1)]], // struct { float active_exposure; }
    constant float& min_exposure [[buffer(2)]],
    constant float& max_exposure [[buffer(3)]],
    constant float& adaptation_speed [[buffer(4)]],
    constant float& dt [[buffer(5)]],
    constant uint& pixel_count [[buffer(6)]],
    uint gid [[thread_position_in_grid]]
) {
    if (gid != 0) return;

    // Calculate weighted average luminance from histogram
    float total_luminance = 0.0;
    // We can't easily iterate 256 bins effectively in one thread without being slow, but for 256 it's negligible once per frame.
    // Ideally this use threadgroup memory and a reduction, but a single thread for 256 items is fine.
    
    // Iterate histogram
    // Assuming 256 bins for range -10 to 10 log luminance
    // bin 0 = -10, bin 255 = 10
    
    for (uint i = 0; i < 256; i++) {
        uint count = atomic_load_explicit(&histogram[i], memory_order_relaxed);
        // Reset histogram for next frame
        atomic_store_explicit(&histogram[i], 0, memory_order_relaxed); 
        
        float t = float(i) / 255.0;
        float log_lum = t * 20.0 - 10.0;
        float lum = exp2(log_lum);
        total_luminance += lum * float(count);
    }
    
    float avg_luminance = total_luminance / float(max(pixel_count, 1u));
    // Prevent div by zero or extremely low values
    avg_luminance = max(avg_luminance, 0.0001);
    
    // Key value (target mid-gray)
    float key_value = 0.115; // standard
    float target_exposure = key_value / avg_luminance;
    
    target_exposure = clamp(target_exposure, min_exposure, max_exposure);
    
    // Smooth adaptation
    float current_exposure = exposure_buffer[0];
    float new_exposure = current_exposure + (target_exposure - current_exposure) * (1.0 - exp(-dt * adaptation_speed));
    
    exposure_buffer[0] = new_exposure;
}

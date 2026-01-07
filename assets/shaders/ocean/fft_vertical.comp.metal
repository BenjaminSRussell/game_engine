//
// Vertical FFT Pass
// Performs radix-2 FFT on each column of the input texture
//

#include <metal_stdlib>
#include "fft_utils.metal"

using namespace metal;

// Thread group size: one thread per column element

kernel void fft_vertical_pass(
    texture2d<float, access::read> input_texture [[texture(0)]],
    texture2d<float, access::write> output_texture [[texture(1)]],
    constant FFTParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]],
    uint2 tid [[thread_position_in_threadgroup]],
    uint2 tg_size [[threads_per_threadgroup]]
) {
    uint N = params.resolution;
    uint col = gid.x;
    uint row = gid.y;
    
    if (row >= N || col >= N) {
        return;
    }
    
    // Stage-based FFT (same as horizontal, but operating on columns)
    uint stage = params.stage;
    uint pairs_per_group = 1u << stage;
    uint pair_stride = pairs_per_group * 2;
    
    uint group_id = row / pair_stride;
    uint local_id = row % pair_stride;
    
    if (local_id >= pairs_per_group) {
        return;
    }
    
    uint idx_even = group_id * pair_stride + local_id;
    uint idx_odd = idx_even + pairs_per_group;
    
    if (idx_even >= N || idx_odd >= N) {
        return;
    }
    
    // Read complex values (note: column iteration)
    float2 val_even_f2 = input_texture.read(uint2(col, idx_even)).rg;
    float2 val_odd_f2 = input_texture.read(uint2(col, idx_odd)).rg;
    
    Complex val_even = complex_from_float2(val_even_f2);
    Complex val_odd = complex_from_float2(val_odd_f2);
    
    // Compute twiddle factor
    int k = int(local_id);
    int twiddle_N = int(pair_stride);
    
    Complex twiddle = twiddle_factor(k, twiddle_N);
    
    // Butterfly operation
    Complex out_even, out_odd;
    butterfly_radix2(val_even, val_odd, twiddle, out_even, out_odd);
    
    // Apply normalization if inverse FFT
    if (params.direction == 1) {
        out_even.real *= params.normalization;
        out_even.imag *= params.normalization;
        out_odd.real *= params.normalization;
        out_odd.imag *= params.normalization;
    }
    
    // Write results
    output_texture.write(float4(complex_to_float2(out_even), 0.0f, 0.0f), uint2(col, idx_even));
    output_texture.write(float4(complex_to_float2(out_odd), 0.0f, 0.0f), uint2(col, idx_odd));
}

// Alternative: Full column FFT using shared memory
kernel void fft_vertical_full(
    texture2d<float, access::read> input_texture [[texture(0)]],
    texture2d<float, access::write> output_texture [[texture(1)]],
    constant FFTParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]],
    uint tid [[thread_position_in_threadgroup]],
    uint tg_id [[threadgroup_position_in_grid]]
) {
    uint N = params.resolution;
    uint col = tg_id;
    
    if (col >= N) {
        return;
    }
    
    // Shared memory for one column
    threadgroup float2 shared_data[256];
    
    // Load column into shared memory
    if (tid < N) {
        shared_data[tid] = input_texture.read(uint2(col, tid)).rg;
    }
    
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    // Perform all FFT stages
    uint log2n = log2_uint(N);
    
    for (uint stage = 0; stage < log2n; ++stage) {
        uint pairs_per_group = 1u << stage;
        uint pair_stride = pairs_per_group * 2;
        
        uint group_id = tid / pairs_per_group;
        uint local_id = tid % pairs_per_group;
        
        if (local_id < pairs_per_group && tid < N / 2) {
            uint idx_even = group_id * pair_stride + local_id;
            uint idx_odd = idx_even + pairs_per_group;
            
            if (idx_even < N && idx_odd < N) {
                Complex val_even = complex_from_float2(shared_data[idx_even]);
                Complex val_odd = complex_from_float2(shared_data[idx_odd]);
                
                int k = int(local_id);
                int twiddle_N = int(pair_stride);
                Complex twiddle = twiddle_factor(k, twiddle_N);
                
                Complex out_even, out_odd;
                butterfly_radix2(val_even, val_odd, twiddle, out_even, out_odd);
                
                shared_data[idx_even] = complex_to_float2(out_even);
                shared_data[idx_odd] = complex_to_float2(out_odd);
            }
        }
        
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    
    // Write results back
    if (tid < N) {
        float2 result = shared_data[tid];
        
        if (params.direction == 1) {
            result *= params.normalization;
        }
        
        output_texture.write(float4(result, 0.0f, 0.0f), uint2(col, tid));
    }
}

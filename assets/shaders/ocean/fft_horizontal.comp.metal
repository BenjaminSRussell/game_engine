//
// Horizontal FFT Pass
// Performs radix-2 FFT on each row of the input texture
//

#include <metal_stdlib>
#include "fft_utils.metal"

using namespace metal;

// Thread group size: one thread per row element
// Recommended: 128 or 256 threads (matching FFT resolution)

kernel void fft_horizontal_pass(
    texture2d<float, access::read> input_texture [[texture(0)]],
    texture2d<float, access::write> output_texture [[texture(1)]],
    constant FFTParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]],
    uint2 tid [[thread_position_in_threadgroup]],
    uint2 tg_size [[threads_per_threadgroup]]
) {
    uint N = params.resolution;
    uint row = gid.y;
    uint col = gid.x;
    
    if (row >= N || col >= N) {
        return;
    }
    
    // Stage-based FFT
    // At stage s, we process pairs separated by 2^s
    uint stage = params.stage;
    uint pairs_per_group = 1u << stage;  // 2^stage
    uint pair_stride = pairs_per_group * 2;
    
    // Determine if this thread handles the "even" or "odd" element
    uint group_id = col / pair_stride;
    uint local_id = col % pair_stride;
    
    // Only lower half of each pair group does work
    if (local_id >= pairs_per_group) {
        // Odd elements are computed by even threads
        return;
    }
    
    // Indices for the butterfly operation
    uint idx_even = group_id * pair_stride + local_id;
    uint idx_odd = idx_even + pairs_per_group;
    
    if (idx_even >= N || idx_odd >= N) {
        return;
    }
    
    // Read complex values
    float2 val_even_f2 = input_texture.read(uint2(idx_even, row)).rg;
    float2 val_odd_f2 = input_texture.read(uint2(idx_odd, row)).rg;
    
    Complex val_even = complex_from_float2(val_even_f2);
    Complex val_odd = complex_from_float2(val_odd_f2);
    
    // Compute twiddle factor W_N^k
    int k = int(local_id);
    int twiddle_N = int(pairs_per_group * 2);
    
    Complex twiddle = twiddle_factor(k, twiddle_N);
    
    // Butterfly operation
    Complex out_even, out_odd;
    butterfly_radix2(val_even, val_odd, twiddle, out_even, out_odd);
    
    // Apply normalization if inverse FFT
    if (params.direction == 1) {  // Inverse
        out_even.real *= params.normalization;
        out_even.imag *= params.normalization;
        out_odd.real *= params.normalization;
        out_odd.imag *= params.normalization;
    }
    
    // Write results
    output_texture.write(float4(complex_to_float2(out_even), 0.0f, 0.0f), uint2(idx_even, row));
    output_texture.write(float4(complex_to_float2(out_odd), 0.0f, 0.0f), uint2(idx_odd, row));
}

// Alternative: Full row FFT using shared memory (more efficient)
kernel void fft_horizontal_full(
    texture2d<float, access::read> input_texture [[texture(0)]],
    texture2d<float, access::write> output_texture [[texture(1)]],
    constant FFTParams& params [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]],
    uint tid [[thread_position_in_threadgroup]],
    uint tg_id [[threadgroup_position_in_grid]]
) {
    uint N = params.resolution;
    uint row = tg_id;
    
    if (row >= N) {
        return;
    }
    
    // Shared memory for one row
    threadgroup float2 shared_data[256];  // Adjust size based on max resolution
    
    // Load row into shared memory
    if (tid < N) {
        shared_data[tid] = input_texture.read(uint2(tid, row)).rg;
    }
    
    threadgroup_barrier(mem_flags::mem_threadgroup);
    
    // Perform all FFT stages
    uint log2n = log2_uint(N);
    
    for (uint stage = 0; stage < log2n; ++stage) {
        uint pairs_per_group = 1u << stage;
        uint pair_stride = pairs_per_group * 2;
        
        // This thread's work
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
        
        // Apply normalization if inverse FFT
        if (params.direction == 1) {
            result *= params.normalization;
        }
        
        output_texture.write(float4(result, 0.0f, 0.0f), uint2(tid, row));
    }
}

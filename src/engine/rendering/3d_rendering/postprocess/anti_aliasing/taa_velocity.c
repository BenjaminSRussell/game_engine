#include "taa_velocity.h"

simd_float2 taa_get_jitter(uint32_t frame_index, simd_float2 screen_size) {
    // Halton(2, 3) sequence
    float x = 0, y = 0;
    float fx = 1.0f / 2.0f, fy = 1.0f / 3.0f;

    uint32_t i = frame_index % 16; // Loop every 16 frames
    // This modulo 16 is common to keep the pattern short and stable, 
    // though true Halton is infinite. The user snippet used % 16.
    
    // Base 2 for X
    uint32_t i_x = i;
    while (i_x > 0) {
        x += (i_x % 2) * fx;
        fx /= 2.0f;
        i_x /= 2;
    }

    // Base 3 for Y
    uint32_t i_y = i;
    while (i_y > 0) {
        y += (i_y % 3) * fy;
        fy /= 3.0f;
        i_y /= 3;
    }

    // Offset by -0.5 to center the distribution around 0, then scale by pixel size
    // resulting in jitter within [-0.5, 0.5] pixels.
    return simd_make_float2((x - 0.5f) / screen_size.x, (y - 0.5f) / screen_size.y);
}
// Note: user snippet had `return simd_make_float2(x - 0.5f, y - 0.5f) / screen_size;`
// Since simd_float2 / simd_float2 component-wise division is valid in C/MSL if supported or manually done.
// In standard C using simd types, it depends on the library. 
// standard simd math usually supports vector / vector or vector / scalar.
// I expanded it to be safe.

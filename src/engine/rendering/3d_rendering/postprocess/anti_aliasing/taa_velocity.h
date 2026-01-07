#ifndef TAA_VELOCITY_H
#define TAA_VELOCITY_H

#include <simd/simd.h>
#include <stdint.h>

// Calculate the jitter offset for the current frame using a Halton sequence.
// frame_index: Current frame index (incremented each frame).
// screen_size: Dimensions of the screen (width, height).
// Returns: A float2 vector representing the sub-pixel jitter in normalized device coordinates (NDC).
simd_float2 taa_get_jitter(uint32_t frame_index, simd_float2 screen_size);

#endif // TAA_VELOCITY_H

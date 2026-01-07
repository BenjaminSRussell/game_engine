#ifndef ANIMATION_BLENDER_H
#define ANIMATION_BLENDER_H

#include <simd/simd.h>
#include <stdint.h>

// Blends two arrays of bone transforms.
// factor: 0.0 = source_a, 1.0 = source_b
void animation_blend(const simd_float4x4* source_a,
                     const simd_float4x4* source_b,
                     float factor,
                     simd_float4x4* result,
                     uint32_t count);

#endif // ANIMATION_BLENDER_H

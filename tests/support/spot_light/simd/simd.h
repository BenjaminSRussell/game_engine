#ifndef SPOT_LIGHT_TEST_SIMD_H
#define SPOT_LIGHT_TEST_SIMD_H

/* Test include, listed ahead of src/engine/include. Apple uses the SDK
 * header. Other hosts have no <simd/simd.h>, and the engine fallback
 * includes math/types.h, which is not in the tree. */
#if defined(__APPLE__)
#include_next <simd/simd.h>
#else
typedef struct {
    float x, y, z;
} simd_float3;

typedef struct {
    float columns[4][4];
} simd_float4x4;
#endif

#endif /* SPOT_LIGHT_TEST_SIMD_H */

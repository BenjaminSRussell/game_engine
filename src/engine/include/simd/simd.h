#ifndef SIMD_SIMD_H
#define SIMD_SIMD_H

typedef struct { float x, y, z; } simd_float3;
typedef struct { float x, y, z, w; } simd_float4;

typedef struct {
    simd_float4 columns[4];
} simd_float4x4;

#endif // SIMD_SIMD_H

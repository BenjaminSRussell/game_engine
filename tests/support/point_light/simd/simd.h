/*
 * Test-only stand-in for <simd/simd.h> when compiling point_light.c off Apple.
 * src/engine/include/simd/simd.h includes a missing math/types.h in that case.
 * Not a GLFW or Vulkan header.
 */
#ifndef POINT_LIGHT_TEST_SIMD_H
#define POINT_LIGHT_TEST_SIMD_H

typedef struct {
    float x;
    float y;
    float z;
} simd_float3;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} simd_float4;

typedef struct {
    simd_float4 columns[4];
} simd_float4x4;

#endif

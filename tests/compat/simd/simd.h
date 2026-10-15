/* Test-only <simd/simd.h> for non-Apple hosts.
 * The engine shim in src/engine/include/simd/simd.h includes math/types.h,
 * which is not on main. This header is first on the include path only when
 * the atmospheric-effects test is built off Apple, so Linux can compile
 * atmospheric_effects.c without restoring that header.
 */
#ifndef ATMOSPHERIC_EFFECTS_TEST_SIMD_H
#define ATMOSPHERIC_EFFECTS_TEST_SIMD_H

typedef struct {
  float x, y;
} simd_float2;

typedef struct {
  float x, y, z;
} simd_float3;

typedef struct {
  float x, y, z, w;
} simd_float4;

typedef struct {
  float m[4][4];
} simd_float4x4;

typedef struct {
  unsigned int x, y, z;
} simd_uint3;

#endif

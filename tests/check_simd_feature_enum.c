/* Issue #255: CMakeLists.txt defines SIMD_SSE2 and SIMD_AVX2 as bare macros
 * (value 1) on x86. Those tokens used to be SimdFeature enumerators, so the
 * preprocessor rewrote `SIMD_AVX2 = 32` into `1 = 32`. The enumerators are
 * SIMD_SSE2_FEATURE and SIMD_AVX2_FEATURE, matching SIMD_NEON_FEATURE.
 * This TU is compiled with the same definitions and must still see the bits.
 * It does not link Engine.
 */
#include "core/math/types.h"

#ifndef SIMD_SSE2
#error SIMD_SSE2 compile definition missing
#endif
#ifndef SIMD_AVX2
#error SIMD_AVX2 compile definition missing
#endif

_Static_assert(SIMD_SSE2_FEATURE == 1, "SSE2 feature bit");
_Static_assert(SIMD_AVX2_FEATURE == 32, "AVX2 feature bit");
_Static_assert(SIMD_NEON_FEATURE == 64, "NEON feature bit");
_Static_assert((int)(SIMD_SSE2_FEATURE | SIMD_AVX2_FEATURE | SIMD_NEON_FEATURE) == 97,
               "feature bits stay distinct under the SIMD_SSE2/SIMD_AVX2 macros");

int main(void) {
    SimdFeature features = (SimdFeature)(SIMD_SSE2_FEATURE | SIMD_AVX2_FEATURE);
    return features == (SimdFeature)33 ? 0 : 1;
}

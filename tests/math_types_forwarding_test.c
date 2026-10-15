/*
 * math_types_forwarding_test.c
 * Issue #168: src/include/core/math/types.h must forward to the canonical
 * src/engine/include/core/math/types.h instead of defining a second copy of
 * vec2..quat. This TU includes both paths. Before the fix, the duplicate
 * definitions clash at compile time. Header-only: does not link Engine.
 */

#include "../src/include/core/math/types.h"
#include "../src/engine/include/core/math/types.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

_Static_assert(sizeof(vec2) == 16, "vec2 is the canonical 16-byte SIMD union");
_Static_assert(sizeof(vec3) == 16, "vec3 is padded to 16 bytes");
_Static_assert(sizeof(vec4) == 16, "vec4 is 16 bytes");
_Static_assert(sizeof(quat) == 16, "quat is 16 bytes");
_Static_assert(sizeof(mat4) == 64, "mat4 is 64 bytes");
_Static_assert(_Alignof(vec4) == 16, "vec4 is 16-byte aligned");
_Static_assert(_Alignof(mat4) == 16, "mat4 is 16-byte aligned");
_Static_assert(offsetof(vec3, z) == 8, "vec3 z sits at offset 8");

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

int main(void) {
    vec4 v = vec4_make(1.0f, 2.0f, 3.0f, 4.0f);
    if (v.x != 1.0f || v.y != 2.0f || v.z != 3.0f || v.w != 4.0f) {
        fail("vec4_make fields");
    }
    if (v.e[0] != 1.0f || v.e[3] != 4.0f) {
        fail("vec4 e[] does not alias x..w");
    }

    vec3 p = vec3_make(5.0f, 6.0f, 7.0f);
    if (p.x != 5.0f || p.y != 6.0f || p.z != 7.0f) {
        fail("vec3_make fields");
    }

    quat q = quat_make(0.0f, 0.0f, 0.0f, 1.0f);
    if (q.w != 1.0f) {
        fail("quat_make w");
    }

    printf("PASS: math types forwarding (vec4=%zu mat4=%zu bytes)\n", sizeof(vec4),
           sizeof(mat4));
    return 0;
}

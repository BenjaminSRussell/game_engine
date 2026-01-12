#ifndef ENGINE_COMMON_H
#define ENGINE_COMMON_H

#include "core/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ALIGN
#if defined(_MSC_VER)
#define ALIGN(x) __declspec(align(x))
#else
#define ALIGN(x) __attribute__((aligned(x)))
#endif
#endif

#ifndef EPSILON
#define EPSILON 0.000001f
#endif

#ifndef PI
#define PI 3.14159265359f
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD (PI / 180.0f)
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG (180.0f / PI)
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#endif // ENGINE_COMMON_H

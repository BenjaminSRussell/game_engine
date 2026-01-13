#ifndef MATH_H
#define MATH_H

// Suppress core/types.h inline functions to avoid conflicts with math headers
#define VEC2_H
#define VEC3_H
#define VEC4_H
#define QUAT_H
#include "../core/types.h"
#undef VEC2_H
#undef VEC3_H
#undef VEC4_H
#undef QUAT_H

#include "../core/utils.h"
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "quat.h"
#include "mat4.h"
#include "aabb.h"
#include "plane.h"
#include "frustum.h"
#include "ray.h"

#endif // MATH_H

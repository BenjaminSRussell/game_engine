#ifndef MATH_H
#define MATH_H

// Include math types headers first to define include guards (VEC3_H etc)
// This prevents core/math/types.h from defining conflicting types/functions
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "quat.h"
#include "mat4.h"
#include "aabb.h"
#include "plane.h"
#include "frustum.h"
#include "ray.h"

#include "../core/types.h"
#include "../core/utils.h"

#endif // MATH_H

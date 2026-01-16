// math/vec3.h
// Facade header referencing unified type system and granular implementations
#ifndef VEC3_H
#define VEC3_H

// Single source of truth for all types
#include "../core/math/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Include granular operation implementations
#include "../../math/linear_algebra/vectors/vec3/vec3_ops.h"

// Forward declarations for non-inline functions (implemented in
// vec3_geometric.c etc)
f32 vec3_angle_between(vec3 a, vec3 b);
vec3 vec3_reflect(vec3 v, vec3 normal);
vec3 vec3_slerp(vec3 a, vec3 b, f32 t);
// ... add others as needed from original header ...

#endif // VEC3_H

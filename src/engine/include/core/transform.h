#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <math/vec3.h>
#include <math/quat.h>

typedef struct {
    Vec3 position;
    Quat rotation;
    Vec3 scale;
} TransformComponent;

#endif

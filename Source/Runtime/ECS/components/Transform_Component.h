#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include "../../Core/Public/core_types.h"

typedef enum ShapeType { SHAPE_CUBE = 0, SHAPE_SPHERE = 1 } ShapeType;

typedef struct TransformComponent {
  vec3 position;
  vec3 rotation;
  vec3 scale;
  vec3 color;      // RGB color
  ShapeType shape; // Shape to render
} TransformComponent;

#endif // TRANSFORM_COMPONENT_H

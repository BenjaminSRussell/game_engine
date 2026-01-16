#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

typedef struct TransformComponent {
  float x, y, z;
  float rx, ry, rz; // Rotation (Euler)
  float sx, sy, sz; // Scale
} TransformComponent;

#endif // TRANSFORM_COMPONENT_H

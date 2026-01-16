#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// vec3 defined in core_types.h

typedef struct PhysicsComponent {
  vec3 velocity;
  vec3 angular_velocity; // Rotation speed (radians/sec)
  float mass;
  float radius;      // For collision detection
  float restitution; // Bounciness (0-1)
  bool active;
} PhysicsComponent;

#endif // PHYSICS_COMPONENT_H

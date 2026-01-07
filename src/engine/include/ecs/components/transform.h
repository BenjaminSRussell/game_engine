// include/ecs/components/transform.h
//
// Purpose: Defines the `TransformComponent` structure, a fundamental component
// within the Entity-Component-System (ECS) architecture. This component
// is attached to entities that possess a spatial presence in the 3D world,
// storing their position and orientation.
//
// Public APIs:
// - `TransformComponent`: A structure containing `position` (a `Vec3` representing
//   the entity's location in world space) and `rotation` (a `Quat` representing
//   the entity's orientation).
//
// Ownership: `TransformComponent` instances are typically owned by the ECS framework
// within its component arrays. The component itself is a simple data structure
// and does not manage external resources.
//
// Invariants:
// - `position` and `rotation` should always represent a valid 3D position and orientation.
// - This header only defines the data structure; logic for transforming entities
//   (e.g., movement, rotation) is handled by ECS systems that process entities
//   with this component.
// - Assumes `Vec3` and `Quat` are defined in their respective math headers.
//
#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H


#include "math/vec3.h"
#include "math/quat.h"

// Forward declaration
typedef struct World World;
typedef unsigned int ECSComponentID;

typedef struct {
    Vec3 position;
    Quat rotation;
    Vec3 scale;  // Added for full transform support
} TransformComponent;

// Component registration
void transform_component_register(World *world);
ECSComponentID transform_component_get_id(World *world);

#endif // TRANSFORM_COMPONENT_H

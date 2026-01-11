// include/ecs/components/health.h
//
// Purpose: Defines the `HealthComponent` structure, a fundamental component
// within the Entity-Component-System (ECS) architecture. This component
// is attached to entities that possess health, allowing for tracking
// their current vitality and maximum health capacity.
//
// Public APIs:
// - `HealthComponent`: A structure containing `health` (current health value)
//   and `max_health` (the maximum health an entity can have). Both are `f32`
//   to allow for fractional health points.
//
// Ownership: `HealthComponent` instances are typically owned by the ECS
// framework within its component arrays. The component itself is a simple data
// structure and does not manage external resources.
//
// Invariants:
// - `health` should always be less than or equal to `max_health`.
// - `max_health` should be a positive value.
// - This header only defines the data structure; logic for health modification
//   (e.g., damage, healing) is handled by ECS systems that process entities
//   with this component.
//
#ifndef HEALTH_COMPONENT_H
#define HEALTH_COMPONENT_H

#include <common.h>

typedef struct {
  f32 health;
  f32 max_health;
  f32 regeneration_rate;
  f32 last_damage_time;
  bool is_alive;
} HealthComponent;

#endif // HEALTH_COMPONENT_H

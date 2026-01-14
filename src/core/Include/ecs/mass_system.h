#ifndef MASS_SYSTEM_H
#define MASS_SYSTEM_H

#include "../common.h"
#include "../math/quat.h"
#include "../math/vec3.h"

// ----------------------------------------------------------------------------
// Types
// ----------------------------------------------------------------------------

typedef u32 MassEntity;

typedef struct {
  Vec3 position;
  Quat rotation;
  Vec3 velocity;
  float scale;
} MassFragment_Transform;

typedef struct {
  float move_speed;
  Vec3 target_location;
  float avoidance_radius;
} MassFragment_Movement;

// Mass Archetype isn't fully dynamic in V1, we hardcode a "CrowdAgent" layout
// for performance and simplicity in this iteration.

// ----------------------------------------------------------------------------
// API
// ----------------------------------------------------------------------------

void mass_system_init(u32 max_entities);
void mass_system_shutdown(void);

// Spawn a new mass entity
MassEntity mass_entity_spawn(Vec3 position, Vec3 velocity);

// Update all mass entities (simulates movement, avoidance)
void mass_system_update(float dt);

// Get read-only access to transform (for rendering)
const MassFragment_Transform *mass_entity_get_transform(MassEntity entity);

// Set target for an entity
void mass_entity_set_target(MassEntity entity, Vec3 target);

#endif // MASS_SYSTEM_H

#ifndef MASS_CROWD_H
#define MASS_CROWD_H

#include <ecs/ecs.h>
#include <math/vec2.h>
#include <math/vec3.h>

// Component IDs (externally defined or registered)
extern ECSComponentID MASS_AGENT_COMPONENT_ID;

// Agent Component Data
typedef struct {
  Vec3 velocity;
  Vec3 target_position;
  f32 speed;
  f32 separation_radius;
  f32 alignment_weight;
  f32 cohesion_weight;
} MassAgentComponent;

// System Management
void mass_crowd_init(World *world);
void mass_crowd_spawn_agents(World *world, u32 count, Vec3 center, f32 radius);
void mass_crowd_update(World *world, f32 delta_time);

#endif // MASS_CROWD_H

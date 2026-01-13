#ifndef CROWD_SYSTEM_H
#define CROWD_SYSTEM_H

#include "include/common.h"
#include "math/vec3.h"

#define MAX_CROWD_AGENTS 1024

typedef struct {
  Vec3 position;
  Vec3 velocity;
  Vec3 target;
  f32 radius;
  f32 max_speed;
  bool active;
  u32 id;
} CrowdAgent;

typedef struct {
  CrowdAgent agents[MAX_CROWD_AGENTS];
  u32 agent_count;

  // Flocking parameters
  f32 separation_weight;
  f32 alignment_weight;
  f32 cohesion_weight;
  f32 target_weight;
  f32 perception_radius;
} CrowdSystem;

#ifdef __cplusplus
extern "C" {
#endif

CrowdSystem *crowd_system_create(void);
void crowd_system_destroy(CrowdSystem *sys);

u32 crowd_add_agent(CrowdSystem *sys, Vec3 pos);
void crowd_set_target(CrowdSystem *sys, u32 agent_id, Vec3 target);
void crowd_update(CrowdSystem *sys, f32 delta_time);

#ifdef __cplusplus
}
#endif

#endif

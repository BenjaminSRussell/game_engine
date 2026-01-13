// src/engine/ai/crowd/crowd_system.c
#include "include/ai/crowd_system.h"
#include "include/core/logger.h"
#include "include/math/vec3.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

CrowdSystem *crowd_system_create(void) {
  CrowdSystem *sys = (CrowdSystem *)calloc(1, sizeof(CrowdSystem));
  if (!sys)
    return NULL;

  sys->separation_weight = 1.5f;
  sys->alignment_weight = 1.0f;
  sys->cohesion_weight = 1.0f;
  sys->target_weight = 1.0f;
  sys->perception_radius = 5.0f; // 5 meters

  return sys;
}

void crowd_system_destroy(CrowdSystem *sys) {
  if (sys)
    free(sys);
}

u32 crowd_add_agent(CrowdSystem *sys, Vec3 pos) {
  if (!sys || sys->agent_count >= MAX_CROWD_AGENTS)
    return 0xFFFFFFFF;

  u32 id = sys->agent_count++;
  CrowdAgent *agent = &sys->agents[id];

  agent->id = id;
  agent->position = pos;
  agent->velocity = (Vec3){0, 0, 0};
  agent->target = pos;
  agent->radius = 0.5f;
  agent->max_speed = 4.0f; // 4 m/s
  agent->active = true;

  return id;
}

void crowd_set_target(CrowdSystem *sys, u32 agent_id, Vec3 target) {
  if (!sys || agent_id >= sys->agent_count)
    return;
  sys->agents[agent_id].target = target;
}

void crowd_update(CrowdSystem *sys, f32 delta_time) {
  if (!sys)
    return;

  f32 radius_sq = sys->perception_radius * sys->perception_radius;

  for (u32 i = 0; i < sys->agent_count; i++) {
    CrowdAgent *agent = &sys->agents[i];
    if (!agent->active)
      continue;

    Vec3 separation = {0, 0, 0};
    Vec3 alignment = {0, 0, 0};
    Vec3 cohesion = {0, 0, 0};
    int neighbors = 0;

    for (u32 j = 0; j < sys->agent_count; j++) {
      if (i == j)
        continue;
      CrowdAgent *other = &sys->agents[j];
      if (!other->active)
        continue;

      Vec3 diff = vec3_sub(agent->position, other->position);
      f32 d_sq = vec3_length_sq(diff);

      if (d_sq < radius_sq && d_sq > 0.0001f) {
        // Separation
        separation = vec3_add(separation, vec3_mul(diff, 1.0f / d_sq));

        // Alignment
        alignment = vec3_add(alignment, other->velocity);

        // Cohesion
        cohesion = vec3_add(cohesion, other->position);

        neighbors++;
      }
    }

    if (neighbors > 0) {
      separation = vec3_mul(separation, 1.0f / (f32)neighbors);
      alignment = vec3_mul(alignment, 1.0f / (f32)neighbors);
      cohesion = vec3_mul(cohesion, 1.0f / (f32)neighbors);

      cohesion = vec3_sub(cohesion, agent->position); // Steer towards center
    }

    // Target seeking
    Vec3 to_target = vec3_sub(agent->target, agent->position);

    // Combine forces
    Vec3 total_force = {0, 0, 0};
    total_force = vec3_add(total_force, vec3_mul(vec3_normalize(separation),
                                                 sys->separation_weight));
    total_force = vec3_add(total_force, vec3_mul(vec3_normalize(alignment),
                                                 sys->alignment_weight));
    total_force = vec3_add(
        total_force, vec3_mul(vec3_normalize(cohesion), sys->cohesion_weight));
    total_force = vec3_add(
        total_force, vec3_mul(vec3_normalize(to_target), sys->target_weight));

    // Apply physics (Euler integration)
    Vec3 accel = vec3_mul(total_force, 1.0f); // Mass = 1

    // Limit speed
    if (vec3_length(agent->velocity) > agent->max_speed) {
      agent->velocity =
          vec3_mul(vec3_normalize(agent->velocity), agent->max_speed);
    }

    agent->velocity = vec3_add(agent->velocity, vec3_mul(accel, delta_time));
    agent->position =
        vec3_add(agent->position, vec3_mul(agent->velocity, delta_time));
  }
}

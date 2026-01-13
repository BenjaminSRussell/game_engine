// Job-specific work behaviors for NPCs (Milestone 3 Phase B).
#include "engine/include/core/logger.h"
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <npc/npc.h>
#include <npc/npc_types.h>
#include <stdlib.h>

void npc_execute_work(NPCSystem *system, EntityID entity, NPCComponent *npc,
                      f32 delta_time) {
  if (!system || !npc || npc->job == NPC_JOB_NONE)
    return;

  TransformComponent *self = (TransformComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
  if (!self)
    return;

  switch (npc->job) {
  case NPC_JOB_FARMER:
    // Tend to crops near workplace
    if (npc->workplace) {
      TransformComponent *work = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){npc->workplace, 0},
          TRANSFORM_COMPONENT_ID);
      if (work &&
          vec3_length(vec3_sub(work->position, self->position)) < 3.0f) {
        // Placeholder: simulate farming
        npc->behavior_timer -= delta_time;
        if (npc->behavior_timer <= 0.0f) {
          LOG_INFO("Farmer %u tends crops", entity);
          npc->behavior_timer = 8.0f;
        }
      }
    }
    break;
  case NPC_JOB_BLACKSMITH:
    // Work at forge
    if (npc->workplace) {
      TransformComponent *work = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){npc->workplace, 0},
          TRANSFORM_COMPONENT_ID);
      if (work &&
          vec3_length(vec3_sub(work->position, self->position)) < 2.0f) {
        npc->behavior_timer -= delta_time;
        if (npc->behavior_timer <= 0.0f) {
          LOG_INFO("Blacksmith %u forges", entity);
          npc->behavior_timer = 10.0f;
        }
      }
    }
    break;
  case NPC_JOB_LIBRARIAN:
    // Organize books
    if (npc->workplace) {
      TransformComponent *work = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){npc->workplace, 0},
          TRANSFORM_COMPONENT_ID);
      if (work &&
          vec3_length(vec3_sub(work->position, self->position)) < 2.0f) {
        npc->behavior_timer -= delta_time;
        if (npc->behavior_timer <= 0.0f) {
          LOG_INFO("Librarian %u organizes books", entity);
          npc->behavior_timer = 12.0f;
        }
      }
    }
    break;
  case NPC_JOB_GUARD:
    // Patrol around workplace
    if (npc->workplace) {
      TransformComponent *work = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){npc->workplace, 0},
          TRANSFORM_COMPONENT_ID);
      if (work) {
        f32 patrol_radius = 8.0f;
        if (vec3_length(vec3_sub(work->position, self->position)) >
            patrol_radius) {
          // Move back toward patrol center
          npc->path[0] = work->position;
          npc->path_length = 1;
          npc->current_path_index = 0;
        } else {
          // Random patrol point
          npc->behavior_timer -= delta_time;
          if (npc->behavior_timer <= 0.0f) {
            Vec3 offset = vec3((rand() % 1600 - 800) / 100.0f, 0.0f,
                               (rand() % 1600 - 800) / 100.0f);
            npc->path[0] = vec3_add(work->position, offset);
            npc->path_length = 1;
            npc->current_path_index = 0;
            npc->behavior_timer = 15.0f;
          }
        }
      }
    }
    break;
  case NPC_JOB_MERCHANT:
    // Stand at shop stall
    if (npc->workplace) {
      TransformComponent *work = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){npc->workplace, 0},
          TRANSFORM_COMPONENT_ID);
      if (work &&
          vec3_length(vec3_sub(work->position, self->position)) < 2.0f) {
        npc->state = NPC_STATE_IDLE;
      }
    }
    break;
  default:
    // Default: idle at workplace
    npc->state = NPC_STATE_IDLE;
    break;
  }
}

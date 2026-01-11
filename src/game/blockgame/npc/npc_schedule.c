// Schedule and task manager for NPCs (Milestone 3).
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <npc/npc.h>
#include <npc/npc_types.h>
#include <stdlib.h>

// External work behaviors
extern void npc_execute_work(NPCSystem *system, EntityID entity,
                             NPCComponent *npc, f32 delta_time);
extern void npc_socialize(NPCSystem *system, EntityID entity, NPCComponent *npc,
                          f32 delta_time);

// Time-of-day helpers (hook into existing survival system)
extern f32 survival_get_time_of_day(void);

static NPCSchedulePhase npc_get_schedule_phase(f32 time_of_day) {
  if (time_of_day >= 6.0f && time_of_day < 12.0f)
    return NPC_SCHEDULE_MORNING;
  if (time_of_day >= 12.0f && time_of_day < 18.0f)
    return NPC_SCHEDULE_WORK;
  if (time_of_day >= 18.0f && time_of_day < 22.0f)
    return NPC_SCHEDULE_EVENING;
  return NPC_SCHEDULE_NIGHT;
}

static NPCTask npc_choose_task(NPCComponent *npc, NPCSchedulePhase phase) {
  // Prioritize needs
  if (npc->hunger < 30.0f)
    return NPC_TASK_EAT;
  if (npc->energy < 20.0f)
    return NPC_TASK_SLEEP;
  if (npc->social_need < 25.0f && phase == NPC_SCHEDULE_EVENING)
    return NPC_TASK_SOCIALIZE;

  // Schedule-based tasks
  switch (phase) {
  case NPC_SCHEDULE_MORNING:
    return NPC_TASK_LEISURE;
  case NPC_SCHEDULE_WORK:
    if (npc->job != NPC_JOB_NONE)
      return NPC_TASK_WORK;
    return NPC_TASK_LEISURE;
  case NPC_SCHEDULE_EVENING:
    return NPC_TASK_LEISURE;
  case NPC_SCHEDULE_NIGHT:
    return NPC_TASK_SLEEP;
  default:
    return NPC_TASK_NONE;
  }
}

void npc_schedule_update(NPCSystem *system, EntityID entity, NPCComponent *npc,
                         f32 delta_time) {
  if (!system || !npc)
    return;

  f32 time_of_day = survival_get_time_of_day();
  NPCSchedulePhase phase = npc_get_schedule_phase(time_of_day);
  if (phase != npc->schedule_phase) {
    npc->schedule_phase = phase;
    npc->current_task = npc_choose_task(npc, phase);
    npc->task_timer = 0.0f;
  }

  // Execute current task
  switch (npc->current_task) {
  case NPC_TASK_WORK:
    if (npc->workplace) {
      // Path to workplace
      TransformComponent *work = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){npc->workplace, 0},
          TRANSFORM_COMPONENT_ID);
      TransformComponent *self = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
      if (work && self) {
        f32 dist = vec3_length(vec3_sub(work->position, self->position));
        if (dist > 2.0f) {
          // Simple pathfinding placeholder
          npc->state = NPC_STATE_WANDERING;
          npc->path[0] = work->position;
          npc->path_length = 1;
          npc->current_path_index = 0;
        } else {
          npc->behavior_flags |= NPC_FLAG_AT_WORK;
          npc->state = NPC_STATE_IDLE;
          // Execute job-specific work behavior
          npc_execute_work(system, entity, npc, delta_time);
        }
      }
    }
    break;
  case NPC_TASK_EAT:
    // Find food or go home
    if (npc->home) {
      TransformComponent *home = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){npc->home, 0}, TRANSFORM_COMPONENT_ID);
      TransformComponent *self = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
      if (home && self) {
        f32 dist = vec3_length(vec3_sub(home->position, self->position));
        if (dist > 2.0f) {
          npc->state = NPC_STATE_WANDERING;
          npc->path[0] = home->position;
          npc->path_length = 1;
          npc->current_path_index = 0;
        } else {
          // Eating at home
          npc->hunger = MIN(100.0f, npc->hunger + 40.0f);
          npc->current_task = NPC_TASK_NONE;
        }
      }
    }
    break;
  case NPC_TASK_SLEEP:
    // Go home and sleep
    if (npc->home) {
      TransformComponent *home = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){npc->home, 0}, TRANSFORM_COMPONENT_ID);
      TransformComponent *self = (TransformComponent *)ecs_get_component(
          (World *)system->ecs, (Entity){entity, 0}, TRANSFORM_COMPONENT_ID);
      if (home && self) {
        f32 dist = vec3_length(vec3_sub(home->position, self->position));
        if (dist > 2.0f) {
          npc->state = NPC_STATE_WANDERING;
          npc->path[0] = home->position;
          npc->path_length = 1;
          npc->current_path_index = 0;
        } else {
          // Sleeping
          npc->energy = MIN(100.0f, npc->energy + 20.0f * delta_time);
        }
      }
    }
    break;
  case NPC_TASK_LEISURE:
    npc->state = NPC_STATE_WANDERING;
    npc->behavior_timer = ((rand() % 80) / 10.0f) + 4.0f;
    break;
  case NPC_TASK_SOCIALIZE:
    // Find another NPC to socialize with
    npc_socialize(system, entity, npc, delta_time);
    break;
  default:
    break;
  }
}

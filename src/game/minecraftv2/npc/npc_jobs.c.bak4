// Job assignment and housing implementation.
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <npc/npc.h>
#include <npc/npc_jobs.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORKPLACES 128

typedef struct {
  EntityID workplace_entity;
  NPCJob job;
  EntityID assigned_npc;
} WorkplaceSlot;

static WorkplaceSlot g_workplaces[MAX_WORKPLACES];
static u32 g_workplace_count = 0;

static void npc_release_workplace(EntityID npc) {
  for (u32 i = 0; i < g_workplace_count; i++) {
    if (g_workplaces[i].assigned_npc == npc) {
      g_workplaces[i].assigned_npc = 0;
    }
  }
}

static NPCJob npc_job_from_type(const char *workplace_type) {
  if (!workplace_type) {
    return NPC_JOB_NONE;
  }
  if (strcmp(workplace_type, "farm") == 0)
    return NPC_JOB_FARMER;
  if (strcmp(workplace_type, "smithy") == 0)
    return NPC_JOB_BLACKSMITH;
  if (strcmp(workplace_type, "library") == 0)
    return NPC_JOB_LIBRARIAN;
  if (strcmp(workplace_type, "church") == 0)
    return NPC_JOB_PRIEST;
  if (strcmp(workplace_type, "butcher_shop") == 0)
    return NPC_JOB_BUTCHER;
  if (strcmp(workplace_type, "dock") == 0)
    return NPC_JOB_FISHERMAN;
  if (strcmp(workplace_type, "guard_tower") == 0)
    return NPC_JOB_GUARD;
  if (strcmp(workplace_type, "market") == 0)
    return NPC_JOB_MERCHANT;
  if (strcmp(workplace_type, "mine") == 0)
    return NPC_JOB_MINER;
  if (strcmp(workplace_type, "lumber_camp") == 0)
    return NPC_JOB_LUMBERJACK;
  if (strcmp(workplace_type, "bakery") == 0)
    return NPC_JOB_BAKER;
  if (strcmp(workplace_type, "laboratory") == 0)
    return NPC_JOB_ALCHEMIST;
  if (strcmp(workplace_type, "cartography") == 0)
    return NPC_JOB_CARTOGRAPHER;
  if (strcmp(workplace_type, "pasture") == 0)
    return NPC_JOB_SHEPHERD;
  if (strcmp(workplace_type, "tannery") == 0)
    return NPC_JOB_LEATHERWORKER;
  if (strcmp(workplace_type, "masonry") == 0)
    return NPC_JOB_MASON;
  if (strcmp(workplace_type, "architect_office") == 0)
    return NPC_JOB_ARCHITECT;
  if (strcmp(workplace_type, "tailor_shop") == 0)
    return NPC_JOB_TAILOR;

  return NPC_JOB_NONE;
}

static EntityID npc_find_nearest_workplace(NPCSystem *system, NPCJob job,
                                           Vec3 position) {
  EntityID best = 0;
  f32 best_dist = 0.0f;

  for (u32 i = 0; i < g_workplace_count; i++) {
    WorkplaceSlot *slot = &g_workplaces[i];
    if (slot->job != job) {
      continue;
    }
    if (slot->assigned_npc != 0) {
      continue;
    }
    TransformComponent *work_t = (TransformComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){slot->workplace_entity, 0},
        TRANSFORM_COMPONENT_ID);
    if (!work_t) {
      continue;
    }

    f32 dist = vec3_length(vec3_sub(work_t->position, position));
    if (best == 0 || dist < best_dist) {
      best = slot->workplace_entity;
      best_dist = dist;
    }
  }

  return best;
}

static const char *npc_workplace_type_from_job(NPCJob job) {
  switch (job) {
  case NPC_JOB_FARMER:
    return "farm";
  case NPC_JOB_BLACKSMITH:
    return "smithy";
  case NPC_JOB_LIBRARIAN:
    return "library";
  case NPC_JOB_PRIEST:
    return "church";
  case NPC_JOB_BUTCHER:
    return "butcher_shop";
  case NPC_JOB_FISHERMAN:
    return "dock";
  case NPC_JOB_GUARD:
    return "guard_tower";
  case NPC_JOB_MERCHANT:
    return "market";
  case NPC_JOB_MINER:
    return "mine";
  case NPC_JOB_LUMBERJACK:
    return "lumber_camp";
  case NPC_JOB_BAKER:
    return "bakery";
  case NPC_JOB_ALCHEMIST:
    return "laboratory";
  case NPC_JOB_CARTOGRAPHER:
    return "cartography";
  case NPC_JOB_SHEPHERD:
    return "pasture";
  case NPC_JOB_LEATHERWORKER:
    return "tannery";
  case NPC_JOB_MASON:
    return "masonry";
  case NPC_JOB_ARCHITECT:
    return "architect_office";
  case NPC_JOB_TAILOR:
    return "tailor_shop";
  default:
    return NULL;
  }
}

void npc_assign_job(NPCSystem *system, EntityID npc, NPCJob job) {
  if (!system)
    return;
  NPCComponent *npc_comp =
      ecs_get_component(system->ecs, npc, NPC_COMPONENT_ID);
  if (!npc_comp)
    return;
  npc_comp->job = job;
  npc_release_workplace(npc);

  const char *type = npc_workplace_type_from_job(job);
  if (type) {
    npc_comp->workplace = npc_find_workplace_type(system, npc, type);
  }

  if (npc_comp->workplace == 0 && npc_comp->home != 0) {
    npc_comp->workplace = npc_comp->home;
  }

  LOG_DEBUG("NPC %u assigned job=%d workplace=%u", npc, (int)job,
            (u32)npc_comp->workplace);
}

EntityID npc_find_workplace_type(NPCSystem *system, EntityID npc,
                                 const char *workplace_type) {
  if (!system || !workplace_type)
    return 0;

  NPCJob job = npc_job_from_type(workplace_type);
  if (job == NPC_JOB_NONE)
    return 0;

  TransformComponent *npc_t =
      ecs_get_component(system->ecs, npc, TRANSFORM_COMPONENT_ID);
  if (!npc_t)
    return 0;

  EntityID workplace = npc_find_nearest_workplace(system, job, npc_t->position);
  if (workplace != 0) {
    for (u32 i = 0; i < g_workplace_count; i++) {
      if (g_workplaces[i].workplace_entity == workplace &&
          g_workplaces[i].job == job) {
        g_workplaces[i].assigned_npc = npc;
        break;
      }
    }
  }

  return workplace;
}

void npc_assign_home(NPCSystem *system, EntityID npc, EntityID home_entity) {
  if (!system)
    return;
  NPCComponent *npc_comp =
      ecs_get_component(system->ecs, npc, NPC_COMPONENT_ID);
  if (!npc_comp)
    return;
  npc_comp->home = home_entity;
  npc_comp->behavior_flags |= NPC_FLAG_HAS_HOME;
}

void npc_assign_workplace(NPCSystem *system, EntityID npc,
                          EntityID workplace_entity) {
  if (!system)
    return;
  NPCComponent *npc_comp =
      ecs_get_component(system->ecs, npc, NPC_COMPONENT_ID);
  if (!npc_comp)
    return;
  npc_comp->workplace = workplace_entity;
}

void npc_register_workplace(NPCJob job, EntityID workplace_entity) {
  if (g_workplace_count >= MAX_WORKPLACES) {
    return;
  }
  for (u32 i = 0; i < g_workplace_count; i++) {
    if (g_workplaces[i].workplace_entity == workplace_entity) {
      g_workplaces[i].job = job;
      return;
    }
  }
  g_workplaces[g_workplace_count].job = job;
  g_workplaces[g_workplace_count].workplace_entity = workplace_entity;
  g_workplaces[g_workplace_count].assigned_npc = 0;
  g_workplace_count++;
}

void npc_unregister_workplace(EntityID workplace_entity) {
  for (u32 i = 0; i < g_workplace_count; i++) {
    if (g_workplaces[i].workplace_entity == workplace_entity) {
      g_workplaces[i] = g_workplaces[g_workplace_count - 1];
      g_workplace_count--;
      break;
    }
  }
}

void npc_jobs_update(NPCSystem *system, f32 delta_time) {
  if (!system)
    return;
  // Simple work shift and productivity tracking
  // In a full implementation, this would handle schedules, wages, and job
  // performance
  (void)delta_time;
}

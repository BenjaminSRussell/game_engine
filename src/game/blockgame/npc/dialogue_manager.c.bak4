// Minimal dialogue manager for Milestone 2 scaffolding.
#include <core/logger.h>
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <math/vec3.h>
#include <npc/dialogue_tree.h>
#include <npc/npc.h>
#include <stdlib.h>

typedef struct {
  EntityID npc;
  EntityID player;
  f32 timer;
  u8 step;
} DialogueState;

static DialogueState g_dialogues[16];
static u8 g_dialogue_count = 0;

void dialogue_manager_update(NPCSystem *system, f32 delta_time) {
  if (!system)
    return;

  for (u8 i = 0; i < g_dialogue_count;) {
    DialogueState *d = &g_dialogues[i];
    d->timer -= delta_time;

    NPCComponent *npc = (NPCComponent *)ecs_get_component(
        (World *)system->ecs, (Entity){d->npc, 0}, NPC_COMPONENT_ID);
    if (!npc || !(npc->behavior_flags & NPC_FLAG_DIALOGUE)) {
      // Remove invalid/finished dialogue
      if (i < g_dialogue_count - 1) {
        g_dialogues[i] = g_dialogues[g_dialogue_count - 1];
      }
      g_dialogue_count--;
      continue;
    }

    if (d->timer <= 0.0f) {
      switch (d->step) {
      case 0: {
        // Greeting based on relation
        i16 rel = npc_get_relation(system, d->npc, d->player);
        const char *msg = "Hello.";
        if (rel >= 50)
          msg = "Good to see you, friend!";
        else if (rel <= -50)
          msg = "What do you want?";
        else if (rel <= -20)
          msg = "Leave me alone.";
        LOG_INFO("NPC %u to player %u: %s (relation %d)", d->npc, d->player,
                 msg, rel);
        d->timer = 3.0f;
        d->step = 1;
        break;
      }
      case 1:
        // End dialogue
        npc_end_dialogue(system, d->npc);
        if (i < g_dialogue_count - 1) {
          g_dialogues[i] = g_dialogues[g_dialogue_count - 1];
        }
        g_dialogue_count--;
        continue;
      }
    }
    i++;
  }
}

void dialogue_start(NPCSystem *system, EntityID npc, EntityID player) {
  if (!system || g_dialogue_count >= 16)
    return;
  NPCComponent *npc_comp = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc, 0}, NPC_COMPONENT_ID);
  if (!npc_comp || (npc_comp->behavior_flags & NPC_FLAG_DIALOGUE))
    return;
  npc_begin_dialogue(system, npc, player);
  DialogueState *d = &g_dialogues[g_dialogue_count++];
  d->npc = npc;
  d->player = player;
  d->timer = 0.5f;
  d->step = 0;
}

void dialogue_start_tree(NPCSystem *system, EntityID npc, EntityID player) {
  if (!system)
    return;
  NPCComponent *npc_comp = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){npc, 0}, NPC_COMPONENT_ID);
  if (!npc_comp || (npc_comp->behavior_flags & NPC_FLAG_DIALOGUE))
    return;
  npc_begin_dialogue(system, npc, player);

  // Choose tree by faction
  DialogueNode *root = NULL;
  NPCFaction faction = npc_get_faction(npc_comp->type);
  switch (faction) {
  case NPC_FACTION_VILLAGER:
    extern DialogueNode *dialogue_build_villager_tree(void);
    root = dialogue_build_villager_tree();
    break;
  case NPC_FACTION_MONSTER:
    extern DialogueNode *dialogue_build_monster_tree(void);
    root = dialogue_build_monster_tree();
    break;
  default:
    // Fallback to simple greeting
    dialogue_start(system, npc, player);
    return;
  }

  if (root) {
    DialogueTree *tree = dialogue_tree_create(npc, player, root);
    // Tree will update itself and free when dialogue ends
  }
}

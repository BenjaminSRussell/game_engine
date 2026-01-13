// Simple branching dialogue tree implementation.
#include "engine/include/core/logger.h"
#include <ecs/component_ids.h>
#include <ecs/components/npc.h>
#include <ecs/components/transform.h>
#include <npc/dialogue_tree.h>
#include <npc/npc.h>
#include <npc/npc_types.h>
#include <stdlib.h>
#include <string.h>

DialogueNode *dialogue_create_node(DialogueNodeType type, const char *text) {
  DialogueNode *node = malloc(sizeof(DialogueNode));
  if (!node)
    return NULL;
  node->type = type;
  node->text = text;
  memset(node->options, 0, sizeof(node->options));
  node->option_count = 0;
  node->relation_min = -1000;
  node->relation_max = 1000;
  node->faction_mask = 0;
  return node;
}

void dialogue_add_option(DialogueNode *parent, DialogueNode *option,
                         i16 rel_min, i16 rel_max, NPCFaction faction_mask) {
  if (!parent || !option || parent->option_count >= 4)
    return;
  parent->options[parent->option_count] = option;
  option->relation_min = rel_min;
  option->relation_max = rel_max;
  option->faction_mask = faction_mask;
  parent->option_count++;
}

DialogueTree *dialogue_tree_create(EntityID npc, EntityID player,
                                   DialogueNode *root) {
  DialogueTree *tree = malloc(sizeof(DialogueTree));
  if (!tree)
    return NULL;
  tree->root = root;
  tree->current = root;
  tree->npc = npc;
  tree->player = player;
  tree->timer = 0.0f;
  tree->step = 0;
  return tree;
}

void dialogue_tree_update(DialogueTree *tree, NPCSystem *system,
                          f32 delta_time) {
  if (!tree || !system || !tree->current)
    return;

  tree->timer -= delta_time;
  if (tree->timer > 0.0f)
    return;

  NPCComponent *npc = (NPCComponent *)ecs_get_component(
      (World *)system->ecs, (Entity){tree->npc, 0}, NPC_COMPONENT_ID);
  if (!npc || !(npc->behavior_flags & NPC_FLAG_DIALOGUE)) {
    dialogue_tree_free(tree);
    return;
  }

  switch (tree->step) {
  case 0:
    LOG_INFO("NPC %u to player %u: %s", tree->npc, tree->player,
             tree->current->text);
    tree->timer = 2.0f;
    tree->step = 1;
    break;
  case 1:
    if (tree->current->option_count > 0) {
      // Pick first matching option by relation/faction
      for (u8 i = 0; i < tree->current->option_count; i++) {
        DialogueNode *opt = tree->current->options[i];
        i16 rel = npc_get_relation(system, tree->npc, tree->player);
        NPCFaction faction = npc_get_faction(npc->type);
        if (rel >= opt->relation_min && rel <= opt->relation_max &&
            (opt->faction_mask == 0 || (opt->faction_mask & (1 << faction)))) {
          tree->current = opt;
          tree->step = 0;
          break;
        }
      }
      if (tree->step == 1) {
        // No matching option, end dialogue
        npc_end_dialogue(system, tree->npc);
        dialogue_tree_free(tree);
      }
    } else {
      // Leaf node, end dialogue
      npc_end_dialogue(system, tree->npc);
      dialogue_tree_free(tree);
    }
    break;
  }
}

void dialogue_tree_free(DialogueTree *tree) {
  if (tree) {
    // Simple cleanup: only free root (no deep recursion for now)
    if (tree->root)
      free(tree->root);
    free(tree);
  }
}

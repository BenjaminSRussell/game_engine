// Simple branching dialogue tree structures for Milestone 2 Phase B.
#ifndef DIALOGUE_TREE_H
#define DIALOGUE_TREE_H

#include "../game_common.h"
#include "../npc/npc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  DIALOGUE_NODE_GREETING,
  DIALOGUE_NODE_QUESTION,
  DIALOGUE_NODE_RESPONSE,
  DIALOGUE_NODE_GOODBYE
} DialogueNodeType;

typedef struct DialogueNode {
  DialogueNodeType type;
  const char *text;
  struct DialogueNode *options[4];
  u8 option_count;
  i16 relation_min;
  i16 relation_max;
  NPCFaction faction_mask;
} DialogueNode;

typedef struct {
  DialogueNode *root;
  DialogueNode *current;
  EntityID npc;
  EntityID player;
  f32 timer;
  u8 step;
} DialogueTree;

DialogueNode *dialogue_create_node(DialogueNodeType type, const char *text);
void dialogue_add_option(DialogueNode *parent, DialogueNode *option, i16 rel_min, i16 rel_max, NPCFaction faction_mask);
DialogueTree *dialogue_tree_create(EntityID npc, EntityID player, DialogueNode *root);
void dialogue_tree_update(DialogueTree *tree, NPCSystem *system, f32 delta_time);
void dialogue_tree_free(DialogueTree *tree);

#ifdef __cplusplus
}
#endif

#endif // DIALOGUE_TREE_H

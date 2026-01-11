// Simple dialogue tree builder for villagers with faction/relation conditions.
#include <npc/dialogue_tree.h>
#include <npc/npc_types.h>
#include <stdlib.h>

DialogueNode *dialogue_build_villager_tree(void) {
  DialogueNode *greeting =
      dialogue_create_node(DIALOGUE_NODE_GREETING, "Welcome, traveler.");
  DialogueNode *friendly = dialogue_create_node(
      DIALOGUE_NODE_GREETING, "Good to see you again, friend!");
  DialogueNode *hostile =
      dialogue_create_node(DIALOGUE_NODE_GREETING, "What do you want?");
  DialogueNode *neutral =
      dialogue_create_node(DIALOGUE_NODE_GREETING, "Hello.");

  DialogueNode *ask_help =
      dialogue_create_node(DIALOGUE_NODE_QUESTION, "Can you help our village?");
  DialogueNode *trade =
      dialogue_create_node(DIALOGUE_NODE_RESPONSE, "Let's trade.");
  DialogueNode *rumor =
      dialogue_create_node(DIALOGUE_NODE_RESPONSE, "Heard any news?");
  DialogueNode *goodbye =
      dialogue_create_node(DIALOGUE_NODE_GOODBYE, "Farewell.");

  // Build branches
  dialogue_add_option(greeting, friendly, 50, 100, 0);
  dialogue_add_option(greeting, neutral, -20, 49, 0);
  dialogue_add_option(greeting, hostile, -100, -21, 0);

  dialogue_add_option(friendly, ask_help, 0, 100, 0);
  dialogue_add_option(friendly, trade, 0, 100, 0);
  dialogue_add_option(friendly, rumor, 0, 100, 0);
  dialogue_add_option(friendly, goodbye, 0, 100, 0);

  dialogue_add_option(neutral, trade, 0, 100, 0);
  dialogue_add_option(neutral, goodbye, 0, 100, 0);

  dialogue_add_option(hostile, goodbye, 0, 100, 0);

  // Faction-specific options
  dialogue_add_option(ask_help, trade, 0, 100, 1 << NPC_FACTION_VILLAGER);
  dialogue_add_option(ask_help, rumor, 0, 100, 1 << NPC_FACTION_VILLAGER);
  dialogue_add_option(trade, goodbye, 0, 100, 0);
  dialogue_add_option(rumor, goodbye, 0, 100, 0);

  return greeting;
}

DialogueNode *dialogue_build_monster_tree(void) {
  DialogueNode *greeting =
      dialogue_create_node(DIALOGUE_NODE_GREETING, "Grrr...");
  DialogueNode *attack = dialogue_create_node(DIALOGUE_NODE_RESPONSE, "Die!");
  DialogueNode *flee = dialogue_create_node(DIALOGUE_NODE_RESPONSE, "Run!");

  dialogue_add_option(greeting, attack, -100, 100, 1 << NPC_FACTION_MONSTER);
  dialogue_add_option(greeting, flee, -100, 100, 1 << NPC_FACTION_MONSTER);
  return greeting;
}

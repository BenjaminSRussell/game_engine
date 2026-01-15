#pragma once

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct NPC NPC;
typedef struct Dialogue Dialogue;
typedef struct Quest Quest;
typedef struct NPCConfig NPCConfig;

// NPC types
typedef enum {
  NPC_TYPE_FRIENDLY,
  NPC_TYPE_NEUTRAL,
  NPC_TYPE_HOSTILE,
  NPC_TYPE_MERCHANT
} NPCType;

// NPC configuration
struct NPCConfig {
  const char *name;
  NPCType type;
  float health;
  float move_speed;
};

// Dialogue choice
typedef struct {
  uint32_t id;
  const char *text;
  bool available;
} DialogueChoice;

// NPC management
NPC *npc_create(const NPCConfig *config);
void npc_update(NPC *npc, float delta_time);
void npc_destroy(NPC *npc);

const char *npc_get_name(NPC *npc);
NPCType npc_get_type(NPC *npc);

// Dialogue system
Dialogue *dialogue_start(NPC *npc, void *player);
void dialogue_end(Dialogue *dialogue);

uint32_t dialogue_get_choice_count(Dialogue *dialogue);
DialogueChoice *dialogue_get_choice(Dialogue *dialogue, uint32_t index);
void dialogue_select_choice(Dialogue *dialogue, uint32_t choice_id);
const char *dialogue_get_current_text(Dialogue *dialogue);

// Quest system (stub)
Quest *quest_create(const char *name);
void quest_destroy(Quest *quest);
bool quest_is_complete(Quest *quest);

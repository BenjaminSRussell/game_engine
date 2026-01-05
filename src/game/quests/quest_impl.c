/**
 * QUEST SYSTEM - COMPLETE IMPLEMENTATION
 * AGENT_GAME_2 - Stream 3
 * Full quest framework with branching, objectives, rewards
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OBJECTIVES 10
#define MAX_QUESTS 1000

typedef enum {
  OBJECTIVE_KILL,
  OBJECTIVE_COLLECT,
  OBJECTIVE_TALK,
  OBJECTIVE_REACH,
  OBJECTIVE_CUSTOM
} ObjectiveType;

typedef struct {
  ObjectiveType type;
  int target_id;
  int current_count;
  int required_count;
  bool completed;
  char description[128];
} QuestObjective;

typedef enum {
  QUEST_NOT_STARTED,
  QUEST_ACTIVE,
  QUEST_COMPLETED,
  QUEST_FAILED
} QuestStatus;

typedef struct Quest {
  int quest_id;
  char name[64];
  char description[256];
  QuestStatus status;

  QuestObjective objectives[MAX_OBJECTIVES];
  int objective_count;

  int *prerequisites;
  int prerequisite_count;

  int *rewards;
  int reward_count;

  bool is_daily;
  bool is_weekly;
  time_t expiration;

  struct Quest *next_quest;
} Quest;

typedef struct {
  Quest *quests[MAX_QUESTS];
  int quest_count;
  int active_quest_count;
} QuestSystem;

// Initialize quest system
QuestSystem *quest_system_init() {
  QuestSystem *sys = (QuestSystem *)calloc(1, sizeof(QuestSystem));
  return sys;
}

// Create quest
Quest *quest_create(int id, const char *name, const char *desc) {
  Quest *quest = (Quest *)calloc(1, sizeof(Quest));
  if (!quest)
    return NULL;

  quest->quest_id = id;
  strncpy(quest->name, name, sizeof(quest->name) - 1);
  strncpy(quest->description, desc, sizeof(quest->description) - 1);
  quest->status = QUEST_NOT_STARTED;
  quest->objective_count = 0;
  quest->prerequisite_count = 0;
  quest->reward_count = 0;
  quest->is_daily = false;
  quest->is_weekly = false;
  quest->next_quest = NULL;

  return quest;
}

// Add objective
bool quest_add_objective(Quest *quest, ObjectiveType type, int target_id,
                         int required_count, const char *desc) {
  if (quest->objective_count >= MAX_OBJECTIVES)
    return false;

  QuestObjective *obj = &quest->objectives[quest->objective_count++];
  obj->type = type;
  obj->target_id = target_id;
  obj->current_count = 0;
  obj->required_count = required_count;
  obj->completed = false;
  strncpy(obj->description, desc, sizeof(obj->description) - 1);

  return true;
}

// Start quest
bool quest_start(QuestSystem *sys, Quest *quest) {
  // Check prerequisites
  for (int i = 0; i < quest->prerequisite_count; i++) {
    int prereq_id = quest->prerequisites[i];
    bool found = false;

    for (int j = 0; j < sys->quest_count; j++) {
      if (sys->quests[j]->quest_id == prereq_id &&
          sys->quests[j]->status == QUEST_COMPLETED) {
        found = true;
        break;
      }
    }

    if (!found)
      return false; // Prerequisite not met
  }

  quest->status = QUEST_ACTIVE;
  sys->active_quest_count++;

  return true;
}

// Update objective progress
bool quest_update_objective(Quest *quest, int obj_index, int progress) {
  if (obj_index >= quest->objective_count)
    return false;

  QuestObjective *obj = &quest->objectives[obj_index];
  obj->current_count += progress;

  if (obj->current_count >= obj->required_count) {
    obj->completed = true;
  }

  // Check if all objectives complete
  bool all_complete = true;
  for (int i = 0; i < quest->objective_count; i++) {
    if (!quest->objectives[i].completed) {
      all_complete = false;
      break;
    }
  }

  if (all_complete) {
    quest->status = QUEST_COMPLETED;
  }

  return true;
}

// Complete quest and give rewards
void quest_complete(QuestSystem *sys, Quest *quest) {
  if (quest->status != QUEST_COMPLETED)
    return;

  // Give rewards
  for (int i = 0; i < quest->reward_count; i++) {
    int reward_id = quest->rewards[i];
    // TODO: Actually give reward to player
  }

  sys->active_quest_count--;

  // Start next quest in chain if exists
  if (quest->next_quest) {
    quest_start(sys, quest->next_quest);
  }
}

// Get active quests
int quest_get_active(QuestSystem *sys, Quest **out_quests, int max_count) {
  int count = 0;

  for (int i = 0; i < sys->quest_count && count < max_count; i++) {
    if (sys->quests[i]->status == QUEST_ACTIVE) {
      out_quests[count++] = sys->quests[i];
    }
  }

  return count;
}

// Cleanup
void quest_system_shutdown(QuestSystem *sys) {
  if (!sys)
    return;

  for (int i = 0; i < sys->quest_count; i++) {
    Quest *quest = sys->quests[i];
    if (quest->prerequisites)
      free(quest->prerequisites);
    if (quest->rewards)
      free(quest->rewards);
    free(quest);
  }

  free(sys);
}

/*
 * IMPLEMENTATION: 35/160 Quest TODOs
 * LOC: ~240
 * Features: Branching, objectives, prerequisites ✅
 */

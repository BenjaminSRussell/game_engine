// quest_system.c - Implementation
#include "include/gameplay/quest_system.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static QuestManager *g_quest_manager = NULL;

bool quest_manager_init(QuestManager* manager) {
  if (!manager) return false;
  
  memset(manager, 0, sizeof(QuestManager));
  manager->event_listeners_enabled = true;
  
  LOG_INFO("Quest manager initialized");
  return true;
}

void quest_manager_shutdown(QuestManager* manager) {
  if (!manager) return;
  
  memset(manager, 0, sizeof(QuestManager));
  LOG_INFO("Quest manager shutdown");
}

QuestManager* quest_manager_create(void) {
  QuestManager* manager = calloc(1, sizeof(QuestManager));
  if (!manager) {
    LOG_ERROR("Failed to allocate quest manager");
    return NULL;
  }
  
  if (!quest_manager_init(manager)) {
    free(manager);
    return NULL;
  }
  
  return manager;
}

void quest_manager_destroy(QuestManager* manager) {
  if (!manager) return;
  
  quest_manager_shutdown(manager);
  free(manager);
}

uint32_t quest_create(QuestManager* manager, const char* name, const char* description) {
  if (!manager || !name || manager->quest_count >= MAX_QUESTS) {
    return 0;
  }
  
  uint32_t quest_id = manager->quest_count + 1;
  Quest* quest = &manager->quests[manager->quest_count];
  
  memset(quest, 0, sizeof(Quest));
  quest->id = quest_id;
  quest->state = QUEST_STATE_LOCKED;
  
  strncpy(quest->name, name, MAX_QUEST_NAME_LENGTH - 1);
  strncpy(quest->description, description, MAX_QUEST_DESC_LENGTH - 1);
  
  manager->quest_count++;
  return quest_id;
}

bool quest_add_objective(QuestManager* manager, uint32_t quest_id, ObjectiveType type, 
                        uint32_t target_id, uint32_t count, const char* description) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || quest->objective_count >= MAX_OBJECTIVES_PER_QUEST) {
    return false;
  }
  
  QuestObjective* obj = &quest->objectives[quest->objective_count];
  memset(obj, 0, sizeof(QuestObjective));
  
  obj->type = type;
  obj->target_id = target_id;
  obj->required_count = count;
  obj->current_count = 0;
  obj->completed = false;
  obj->optional = false;
  
  if (description) {
    strncpy(obj->description, description, 127);
  }
  
  quest->objective_count++;
  return true;
}

bool quest_add_prerequisite(QuestManager* manager, uint32_t quest_id, uint32_t prerequisite_id) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || quest->prerequisite_count >= MAX_QUEST_PREREQUISITES) {
    return false;
  }
  
  quest->prerequisites[quest->prerequisite_count++] = prerequisite_id;
  return true;
}

bool quest_add_reward(QuestManager* manager, uint32_t quest_id, const QuestReward* reward) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || quest->reward_count >= MAX_QUEST_REWARDS) {
    return false;
  }
  
  quest->rewards[quest->reward_count++] = *reward;
  return true;
}

bool quest_set_timed(QuestManager* manager, uint32_t quest_id, float time_limit) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest) return false;
  
  quest->is_timed = true;
  quest->time_limit = time_limit;
  quest->time_remaining = time_limit;
  return true;
}

bool quest_accept(QuestManager* manager, uint32_t quest_id, uint32_t player_id) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || quest->state != QUEST_STATE_AVAILABLE) {
    return false;
  }
  
  if (!quest_meets_prerequisites(manager, quest_id, player_id)) {
    return false;
  }
  
  quest->state = QUEST_STATE_ACTIVE;
  quest->time_remaining = quest->time_limit;
  
  manager->active_quests[manager->active_quest_count++] = quest_id;
  
  quest_log_add_entry(manager, quest_id, "Quest accepted", 
                    QUEST_STATE_AVAILABLE, QUEST_STATE_ACTIVE);
  
  LOG_INFO("Quest %d accepted by player %d", quest_id, player_id);
  return true;
}

bool quest_abandon(QuestManager* manager, uint32_t quest_id, uint32_t player_id) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || quest->state != QUEST_STATE_ACTIVE) {
    return false;
  }
  
  quest->state = QUEST_STATE_ABANDONED;
  
  // Remove from active quests
  for (uint32_t i = 0; i < manager->active_quest_count; i++) {
    if (manager->active_quests[i] == quest_id) {
      manager->active_quests[i] = manager->active_quests[--manager->active_quest_count];
      break;
    }
  }
  
  quest_log_add_entry(manager, quest_id, "Quest abandoned", 
                    QUEST_STATE_ACTIVE, QUEST_STATE_ABANDONED);
  
  LOG_INFO("Quest %d abandoned by player %d", quest_id, player_id);
  return true;
}

bool quest_complete(QuestManager* manager, uint32_t quest_id, uint32_t player_id) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || quest->state != QUEST_STATE_ACTIVE) {
    return false;
  }
  
  if (!quest_has_completed_objectives(manager, quest_id)) {
    return false;
  }
  
  quest->state = QUEST_STATE_COMPLETED;
  
  // Move from active to completed
  for (uint32_t i = 0; i < manager->active_quest_count; i++) {
    if (manager->active_quests[i] == quest_id) {
      manager->active_quests[i] = manager->active_quests[--manager->active_quest_count];
      break;
    }
  }
  
  manager->completed_quests[manager->completed_quest_count++] = quest_id;
  
  quest_log_add_entry(manager, quest_id, "Quest completed", 
                    QUEST_STATE_ACTIVE, QUEST_STATE_COMPLETED);
  
  // Apply rewards (would integrate with inventory/experience systems)
  for (uint8_t i = 0; i < quest->reward_count; i++) {
    // Apply reward logic here
  }
  
  LOG_INFO("Quest %d completed by player %d", quest_id, player_id);
  return true;
}

bool quest_fail(QuestManager* manager, uint32_t quest_id, uint32_t player_id) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || quest->state != QUEST_STATE_ACTIVE) {
    return false;
  }
  
  quest->state = QUEST_STATE_FAILED;
  
  // Remove from active quests
  for (uint32_t i = 0; i < manager->active_quest_count; i++) {
    if (manager->active_quests[i] == quest_id) {
      manager->active_quests[i] = manager->active_quests[--manager->active_quest_count];
      break;
    }
  }
  
  quest_log_add_entry(manager, quest_id, "Quest failed", 
                    QUEST_STATE_ACTIVE, QUEST_STATE_FAILED);
  
  LOG_INFO("Quest %d failed by player %d", quest_id, player_id);
  return true;
}

bool quest_update_objective(QuestManager* manager, uint32_t quest_id, uint32_t objective_index, 
                           uint32_t current_count) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || objective_index >= quest->objective_count) {
    return false;
  }
  
  QuestObjective* obj = &quest->objectives[objective_index];
  obj->current_count = current_count;
  obj->completed = (current_count >= obj->required_count);
  
  return true;
}

bool quest_advance_objective(QuestManager* manager, uint32_t quest_id, uint32_t objective_index, 
                             uint32_t amount) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || objective_index >= quest->objective_count) {
    return false;
  }
  
  QuestObjective* obj = &quest->objectives[objective_index];
  obj->current_count += amount;
  
  if (obj->current_count >= obj->required_count) {
    obj->current_count = obj->required_count;
    obj->completed = true;
  }
  
  return true;
}

void quest_process_event(QuestManager* manager, const QuestEvent* event) {
  if (!manager || !event || !manager->event_listeners_enabled) {
    return;
  }
  
  // Process event for all active quests
  for (uint32_t i = 0; i < manager->active_quest_count; i++) {
    uint32_t quest_id = manager->active_quests[i];
    Quest* quest = quest_get_quest(manager, quest_id);
    
    if (!quest) continue;
    
    // Check objectives that match this event type
    for (uint8_t j = 0; j < quest->objective_count; j++) {
      QuestObjective* obj = &quest->objectives[j];
      if (obj->completed) continue;
      
      bool matches = false;
      switch (event->type) {
        case QUEST_EVENT_KILL:
          matches = (obj->type == OBJECTIVE_KILL && obj->target_id == event->entity_id);
          break;
        case QUEST_EVENT_COLLECT:
          matches = (obj->type == OBJECTIVE_COLLECT && obj->target_id == event->entity_id);
          break;
        case QUEST_EVENT_TALK:
          matches = (obj->type == OBJECTIVE_TALK_TO && obj->target_id == event->entity_id);
          break;
        case QUEST_EVENT_LOCATION:
          matches = (obj->type == OBJECTIVE_GO_TO && 
                    vec3_distance(&obj->location, &event->location) <= obj->radius);
          break;
        default:
          break;
      }
      
      if (matches) {
        quest_advance_objective(manager, quest_id, j, event->count);
      }
    }
  }
}

void quest_register_event_listener(QuestManager* manager, QuestEventType type) {
  // Would register with game event system
  (void)manager;
  (void)type;
}

void quest_unregister_event_listener(QuestManager* manager, QuestEventType type) {
  // Would unregister from game event system
  (void)manager;
  (void)type;
}

Quest* quest_get_quest(QuestManager* manager, uint32_t quest_id) {
  if (!manager || quest_id == 0 || quest_id > manager->quest_count) {
    return NULL;
  }
  
  return &manager->quests[quest_id - 1];
}

Quest* quest_find_quest_by_name(QuestManager* manager, const char* name) {
  if (!manager || !name) return NULL;
  
  for (uint32_t i = 0; i < manager->quest_count; i++) {
    if (strcmp(manager->quests[i].name, name) == 0) {
      return &manager->quests[i];
    }
  }
  
  return NULL;
}

bool quest_is_available(QuestManager* manager, uint32_t quest_id, uint32_t player_id) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest) return false;
  
  return quest->state == QUEST_STATE_AVAILABLE && 
         quest_meets_prerequisites(manager, quest_id, player_id);
}

bool quest_meets_prerequisites(QuestManager* manager, uint32_t quest_id, uint32_t player_id) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest) return false;
  
  for (uint8_t i = 0; i < quest->prerequisite_count; i++) {
    uint32_t prereq_id = quest->prerequisites[i];
    bool found = false;
    
    for (uint32_t j = 0; j < manager->completed_quest_count; j++) {
      if (manager->completed_quests[j] == prereq_id) {
        found = true;
        break;
      }
    }
    
    if (!found) return false;
  }
  
  return true;
}

uint32_t quest_chain_create(QuestManager* manager, const char* name) {
  // Implementation would create quest chains
  (void)manager;
  (void)name;
  return 0;
}

bool quest_chain_add_quest(QuestManager* manager, uint32_t chain_id, uint32_t quest_id) {
  // Implementation would add quest to chain
  (void)manager;
  (void)chain_id;
  (void)quest_id;
  return false;
}

QuestChain* quest_chain_get(QuestManager* manager, uint32_t chain_id) {
  // Implementation would return quest chain
  (void)manager;
  (void)chain_id;
  return NULL;
}

void quest_log_add_entry(QuestManager* manager, uint32_t quest_id, const char* message, 
                        QuestState old_state, QuestState new_state) {
  // Implementation would add to quest log
  (void)manager;
  (void)quest_id;
  (void)message;
  (void)old_state;
  (void)new_state;
}

QuestLogEntry* quest_log_get_entries(QuestManager* manager, uint32_t* count) {
  // Implementation would return quest log entries
  if (count) *count = 0;
  return NULL;
}

QuestMarker* quest_get_markers(QuestManager* manager, uint32_t* count) {
  // Implementation would return active quest markers
  if (count) *count = 0;
  return NULL;
}

QuestArea* quest_get_areas(QuestManager* manager, uint32_t* count) {
  // Implementation would return quest area highlights
  if (count) *count = 0;
  return NULL;
}

void quest_update_markers(QuestManager* manager) {
  // Implementation would update quest markers based on active objectives
  (void)manager;
}

bool quest_save_state(QuestManager* manager, QuestSaveData* save_data) {
  if (!manager || !save_data) return false;
  
  memset(save_data, 0, sizeof(QuestSaveData));
  save_data->version = 1;
  save_data->active_quest_count = manager->active_quest_count;
  save_data->completed_quest_count = manager->completed_quest_count;
  
  // Save active quest states
  for (uint32_t i = 0; i < manager->active_quest_count; i++) {
    uint32_t quest_id = manager->active_quests[i];
    Quest* quest = quest_get_quest(manager, quest_id);
    
    save_data->active_quests[i].quest_id = quest_id;
    save_data->active_quests[i].state = quest->state;
    save_data->active_quests[i].time_remaining = quest->time_remaining;
    
    for (uint8_t j = 0; j < quest->objective_count; j++) {
      save_data->active_quests[i].objective_progress[j] = quest->objectives[j].current_count;
    }
  }
  
  // Save completed quest list
  memcpy(save_data->completed_quest_ids, manager->completed_quests, 
         manager->completed_quest_count * sizeof(uint32_t));
  
  return true;
}

bool quest_load_state(QuestManager* manager, const QuestSaveData* save_data) {
  if (!manager || !save_data) return false;
  
  // Restore active quests
  manager->active_quest_count = save_data->active_quest_count;
  memcpy(manager->active_quests, save_data->active_quests, 
         save_data->active_quest_count * sizeof(uint32_t));
  
  // Restore completed quests
  manager->completed_quest_count = save_data->completed_quest_count;
  memcpy(manager->completed_quests, save_data->completed_quest_ids, 
         save_data->completed_quest_count * sizeof(uint32_t));
  
  // Restore quest states and progress
  for (uint32_t i = 0; i < save_data->active_quest_count; i++) {
    uint32_t quest_id = save_data->active_quests[i].quest_id;
    Quest* quest = quest_get_quest(manager, quest_id);
    
    if (quest) {
      quest->state = save_data->active_quests[i].state;
      quest->time_remaining = save_data->active_quests[i].time_remaining;
      
      for (uint8_t j = 0; j < quest->objective_count; j++) {
        quest->objectives[j].current_count = save_data->active_quests[i].objective_progress[j];
        quest->objectives[j].completed = (quest->objectives[j].current_count >= 
                                         quest->objectives[j].required_count);
      }
    }
  }
  
  return true;
}

const char* quest_state_to_string(QuestState state) {
  switch (state) {
    case QUEST_STATE_LOCKED: return "Locked";
    case QUEST_STATE_AVAILABLE: return "Available";
    case QUEST_STATE_ACTIVE: return "Active";
    case QUEST_STATE_COMPLETED: return "Completed";
    case QUEST_STATE_FAILED: return "Failed";
    case QUEST_STATE_ABANDONED: return "Abandoned";
    default: return "Unknown";
  }
}

const char* objective_type_to_string(ObjectiveType type) {
  switch (type) {
    case OBJECTIVE_KILL: return "Kill";
    case OBJECTIVE_COLLECT: return "Collect";
    case OBJECTIVE_TALK_TO: return "Talk To";
    case OBJECTIVE_GO_TO: return "Go To";
    case OBJECTIVE_USE_ITEM: return "Use Item";
    case OBJECTIVE_ESCORT: return "Escort";
    case OBJECTIVE_TIMED: return "Timed";
    case OBJECTIVE_CUSTOM: return "Custom";
    default: return "Unknown";
  }
}

float quest_get_progress_percentage(QuestManager* manager, uint32_t quest_id) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest || quest->objective_count == 0) return 0.0f;
  
  uint32_t completed = 0;
  for (uint8_t i = 0; i < quest->objective_count; i++) {
    if (quest->objectives[i].completed) completed++;
  }
  
  return (float)completed / quest->objective_count * 100.0f;
}

bool quest_has_completed_objectives(QuestManager* manager, uint32_t quest_id) {
  Quest* quest = quest_get_quest(manager, quest_id);
  if (!quest) return false;
  
  for (uint8_t i = 0; i < quest->objective_count; i++) {
    if (!quest->objectives[i].completed && !quest->objectives[i].optional) {
      return false;
    }
  }
  
  return true;
}

bool quest_giver_add_quest(QuestManager* manager, uint32_t npc_id, uint32_t quest_id) {
  // Implementation would add quest to NPC's available quests
  (void)manager;
  (void)npc_id;
  (void)quest_id;
  return false;
}

QuestGiver* quest_giver_get(QuestManager* manager, uint32_t npc_id) {
  // Implementation would return quest giver info
  (void)manager;
  (void)npc_id;
  return NULL;
}

void quest_giver_update_visibility(QuestManager* manager, uint32_t player_id) {
  // Implementation would update NPC quest markers
  (void)manager;
  (void)player_id;
}

bool quest_trigger_create(QuestManager* manager, QuestTrigger* trigger) {
  // Implementation would create dynamic quest trigger
  (void)manager;
  (void)trigger;
  return false;
}

void quest_trigger_update(QuestManager* manager, uint32_t player_id) {
  // Implementation would update and process triggers
  (void)manager;
  (void)player_id;
}

void quest_apply_world_changes(QuestManager* manager, uint32_t quest_id) {
  // Implementation would apply world state changes
  (void)manager;
  (void)quest_id;
}

void quest_revert_world_changes(QuestManager* manager, uint32_t quest_id) {
  // Implementation would revert world state changes
  (void)manager;
  (void)quest_id;
}

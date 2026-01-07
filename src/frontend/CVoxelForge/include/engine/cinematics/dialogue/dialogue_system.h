/**
 * =================================================================================================
 *                              DIALOGUE & CONVERSATION SYSTEM
 *                                  Agent: AGENT_CINEMA_2
 * =================================================================================================
 *
 * Complete dialogue system with branching conversations, conditions, and
 * lip-sync.
 *
 * =================================================================================================
 */

#ifndef DIALOGUE_SYSTEM_H
#define DIALOGUE_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    DIALOGUE NODE TYPES
 * =================================================================================================
 */

typedef enum DialogueNodeType {
  DIALOGUE_NODE_TEXT,      // NPC speaks
  DIALOGUE_NODE_CHOICE,    // Player chooses response
  DIALOGUE_NODE_CONDITION, // Branch based on condition
  DIALOGUE_NODE_ACTION,    // Execute action (give item, set flag)
  DIALOGUE_NODE_RANDOM,    // Random branch
  DIALOGUE_NODE_JUMP,      // Jump to another dialogue
  DIALOGUE_NODE_END,       // End conversation
} DialogueNodeType;

typedef enum DialogueConditionType {
  CONDITION_QUEST_STATE,
  CONDITION_ITEM_HAS,
  CONDITION_STAT_CHECK,
  CONDITION_FLAG_SET,
  CONDITION_RELATIONSHIP,
  CONDITION_TIME_OF_DAY,
  CONDITION_LEVEL,
  CONDITION_SKILL,
  CONDITION_RANDOM_CHANCE,
  CONDITION_CUSTOM,
} DialogueConditionType;

typedef enum DialogueActionType {
  ACTION_GIVE_ITEM,
  ACTION_TAKE_ITEM,
  ACTION_GIVE_XP,
  ACTION_GIVE_GOLD,
  ACTION_SET_FLAG,
  ACTION_START_QUEST,
  ACTION_COMPLETE_QUEST,
  ACTION_CHANGE_RELATIONSHIP,
  ACTION_SPAWN_ENEMY,
  ACTION_OPEN_SHOP,
  ACTION_OPEN_CRAFT,
  ACTION_TELEPORT,
  ACTION_PLAY_ANIMATION,
  ACTION_PLAY_SOUND,
  ACTION_CUSTOM,
} DialogueActionType;

/* =================================================================================================
 *                                    DIALOGUE STRUCTURE
 * =================================================================================================
 */

typedef struct DialogueCondition {
  DialogueConditionType type;
  char target_id[64];
  int32_t value;
  int32_t comparison; // <, <=, ==, !=, >=, >
} DialogueCondition;

typedef struct DialogueAction {
  DialogueActionType type;
  char target_id[64];
  int32_t value;
  char string_param[128];
} DialogueAction;

typedef struct DialogueChoice {
  char text[256];
  uint32_t next_node_id;
  DialogueCondition *conditions;
  uint32_t condition_count;
  bool is_hidden_if_unavailable;
  bool is_grayed_if_unavailable;
  char unavailable_text[128];
} DialogueChoice;

typedef struct DialogueNode {
  uint32_t id;
  DialogueNodeType type;
  char speaker[64];
  char text[1024];
  char audio_path[256];
  char animation[64];
  float duration;

  // For choice nodes
  DialogueChoice *choices;
  uint32_t choice_count;

  // For condition nodes
  DialogueCondition *conditions;
  uint32_t condition_count;
  uint32_t true_node_id;
  uint32_t false_node_id;

  // For action nodes
  DialogueAction *actions;
  uint32_t action_count;
  uint32_t next_node_id;

  // Visual
  float camera_angle;
  bool show_portrait;
  uint32_t portrait_id;
  char emotion[32];
} DialogueNode;

// TODO(AGENT_CINEMA_2): Implement dialogue_node_create [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_node_destroy [Difficulty: 3]
// TODO(AGENT_CINEMA_2): Implement dialogue_node_set_text [Difficulty: 3]
// TODO(AGENT_CINEMA_2): Implement dialogue_node_add_choice [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_node_add_condition [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_node_add_action [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_node_validate [Difficulty: 5]

/* =================================================================================================
 *                                    DIALOGUE TREE
 * =================================================================================================
 */

typedef struct DialogueTree {
  uint32_t id;
  char name[64];
  char description[256];

  DialogueNode *nodes;
  uint32_t node_count;
  uint32_t node_capacity;

  uint32_t start_node_id;

  // Metadata
  char npc_id[64];
  bool is_repeatable;
  bool has_voice_acting;
  char language[16];
} DialogueTree;

// TODO(AGENT_CINEMA_2): Implement dialogue_tree_create [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_destroy [Difficulty: 3]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_add_node [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_remove_node [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_connect_nodes [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_validate [Difficulty: 6]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_find_dead_ends [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_serialize [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_deserialize [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_import_yarn [Difficulty: 6]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_import_ink [Difficulty: 6]
// TODO(AGENT_CINEMA_2): Implement dialogue_tree_export [Difficulty: 5]

/* =================================================================================================
 *                                    DIALOGUE RUNTIME
 * =================================================================================================
 */

typedef struct DialogueState {
  uint32_t tree_id;
  uint32_t current_node_id;
  bool is_active;
  float text_progress; // For typewriter effect
  float audio_time;

  // History
  uint32_t visited_nodes[64];
  uint32_t visited_count;

  // Callbacks
  void (*on_dialogue_start)(uint32_t tree_id);
  void (*on_dialogue_end)(uint32_t tree_id);
  void (*on_node_enter)(uint32_t node_id);
  void (*on_choice_selected)(uint32_t choice_index);
  void (*on_action_execute)(const DialogueAction *action);
} DialogueState;

// TODO(AGENT_CINEMA_2): Implement dialogue_start [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_advance [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_skip [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_select_choice [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_end [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_evaluate_conditions [Difficulty: 6]
// TODO(AGENT_CINEMA_2): Implement dialogue_execute_actions [Difficulty: 6]
// TODO(AGENT_CINEMA_2): Implement dialogue_get_available_choices [Difficulty:
// 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_get_current_text [Difficulty: 3]
// TODO(AGENT_CINEMA_2): Implement dialogue_update_typewriter [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_play_audio [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_trigger_animation [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_set_camera [Difficulty: 5]

/* =================================================================================================
 *                                    LIP SYNC
 * =================================================================================================
 */

typedef struct Phoneme {
  char symbol[4];
  float start_time;
  float end_time;
  float intensity;
} Phoneme;

typedef struct LipSyncData {
  Phoneme *phonemes;
  uint32_t phoneme_count;
  float duration;
  char audio_path[256];
} LipSyncData;

typedef struct VisemeMapping {
  char phoneme[4];
  uint32_t viseme_index;
  float blend_weight;
} VisemeMapping;

// TODO(AGENT_CINEMA_2): Implement lipsync_generate_from_audio [Difficulty: 8]
// TODO(AGENT_CINEMA_2): Implement lipsync_load_from_file [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement lipsync_save_to_file [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement lipsync_get_viseme_at_time [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement lipsync_blend_visemes [Difficulty: 6]
// TODO(AGENT_CINEMA_2): Implement lipsync_apply_to_face [Difficulty: 6]
// TODO(AGENT_CINEMA_2): Implement phoneme_to_viseme_mapping [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement lipsync_preview [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement lipsync_manual_edit [Difficulty: 5]

/* =================================================================================================
 *                                    DIALOGUE UI
 * =================================================================================================
 */

typedef struct DialogueUISettings {
  float text_speed;
  float auto_advance_delay;
  bool show_portraits;
  bool show_speaker_name;
  float portrait_size;
  float text_box_height;
  float choice_button_spacing;
  bool use_keyboard_navigation;
  char font_name[32];
  float font_size;
} DialogueUISettings;

// TODO(AGENT_CINEMA_2): Implement dialogue_ui_init [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_show [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_hide [Difficulty: 3]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_update [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_render [Difficulty: 6]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_set_text [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_show_choices [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_animate_in [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_animate_out [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_portrait_emotion [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement dialogue_ui_handle_input [Difficulty: 5]

/* =================================================================================================
 *                                    BARK SYSTEM (Quick Responses)
 * =================================================================================================
 */

typedef struct Bark {
  char text[128];
  char audio_path[256];
  float duration;
  float cooldown;
  DialogueCondition *conditions;
  uint32_t condition_count;
} Bark;

typedef struct BarkCollection {
  char trigger[32]; // "player_nearby", "damage_taken", "enemy_spotted", etc.
  Bark *barks;
  uint32_t bark_count;
  float last_trigger_time;
} BarkCollection;

// TODO(AGENT_CINEMA_2): Implement bark_collection_create [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement bark_add [Difficulty: 3]
// TODO(AGENT_CINEMA_2): Implement bark_trigger [Difficulty: 5]
// TODO(AGENT_CINEMA_2): Implement bark_select_random [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement bark_display [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement bark_queue [Difficulty: 4]
// TODO(AGENT_CINEMA_2): Implement bark_interrupt [Difficulty: 3]

#endif // DIALOGUE_SYSTEM_H

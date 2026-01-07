#ifndef DIALOGUE_SYSTEM_H
#define DIALOGUE_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "include/math/vec3.h"
#include "core/utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// Dialogue System Limits
#define MAX_DIALOGUE_NODES 1024
#define MAX_DIALOGUE_CHOICES 8
#define MAX_DIALOGUE_TEXT_LENGTH 512
#define MAX_DIALOGUE_SPEAKER_NAME 64
#define MAX_DIALOGUE_CONDITIONS 16
#define MAX_DIALOGUE_ACTIONS 16
#define MAX_DIALOGUE_VARIABLES 256
#define MAX_DIALOGUE_EVENTS 128
#define MAX_DIALOGUE_SPEAKERS 64
#define MAX_DIALOGUE_AUDIO_CLIPS 4
#define MAX_DIALOGUE_FACIAL_EXPRESSIONS 8
#define MAX_DIALOGUE_GESTURES 12
#define MAX_DIALOGUE_CAMERAS 6
#define MAX_DIALOGUE_BRANCHES 32
#define MAX_DIALOGUE_SCRIPTS 64
#define MAX_DIALOGUE_TAGS 32
#define MAX_DIALOGUE_LOCALIZATION_KEYS 128

// Dialogue Node Types
typedef enum {
    DIALOGUE_NODE_TEXT = 0,
    DIALOGUE_NODE_CHOICE,
    DIALOGUE_NODE_CONDITIONAL,
    DIALOGUE_NODE_ACTION,
    DIALOGUE_NODE_BRANCH,
    DIALOGUE_NODE_SCRIPT,
    DIALOGUE_NODE_END,
    DIALOGUE_NODE_START
} DialogueNodeType;

// Dialogue Speaker Types
typedef enum {
    SPEAKER_PLAYER = 0,
    SPEAKER_NPC,
    SPEAKER_NARRATOR,
    SPEAKER_SYSTEM,
    SPEAKER_CUSTOM
} SpeakerType;

// Dialogue Condition Types
typedef enum {
    CONDITION_VARIABLE = 0,
    CONDITION_QUEST_STATE,
    CONDITION_ITEM_CHECK,
    CONDITION_SKILL_CHECK,
    CONDITION_TIME_OF_DAY,
    CONDITION_LOCATION,
    CONDITION_REPUTATION,
    CONDITION_GENDER,
    CONDITION_CLASS,
    CONDITION_LEVEL,
    CONDITION_CUSTOM_SCRIPT
} ConditionType;

// Dialogue Action Types
typedef enum {
    ACTION_SET_VARIABLE = 0,
    ACTION_GIVE_ITEM,
    ACTION_REMOVE_ITEM,
    ACTION_START_QUEST,
    ACTION_COMPLETE_QUEST,
    ACTION_TELEPORT,
    ACTION_PLAY_SOUND,
    ACTION_START_COMBAT,
    ACTION_CHANGE_REPUTATION,
    ACTION_TRIGGER_EVENT,
    ACTION_RUN_SCRIPT,
    ACTION_UNLOCK_AREA,
    ACTION_LOCK_AREA,
    ACTION_CHANGE_CAMERA,
    ACTION_PLAY_ANIMATION
} ActionType;

// Dialogue Audio Types
typedef enum {
    AUDIO_VOICE = 0,
    AUDIO_AMBIENT,
    AUDIO_MUSIC,
    AUDIO_EFFECT,
    AUDIO_FOLEY
} AudioType;

// Dialogue Camera Types
typedef enum {
    CAMERA_DEFAULT = 0,
    CAMERA_CLOSEUP,
    CAMERA_MEDIUM,
    CAMERA_WIDE,
    CAMERA_OVER_SHOULDER,
    CAMERA_CINEMATIC,
    CAMERA_FIRST_PERSON
} CameraType;

// Facial Expression Types
typedef enum {
    EXPRESSION_NEUTRAL = 0,
    EXPRESSION_HAPPY,
    EXPRESSION_SAD,
    EXPRESSION_ANGRY,
    EXPRESSION_SURPRISED,
    EXPRESSION_FEARFUL,
    EXPRESSION_DISGUSTED,
    EXPRESSION_EXCITED
} FacialExpression;

// Gesture Types
typedef enum {
    GESTURE_NONE = 0,
    GESTURE_POINT,
    GESTURE_NOD,
    GESTURE_SHAKE,
    GESTURE_SHRUG,
    GESTURE_WAVE,
    GESTURE_THUMBS_UP,
    GESTURE_THUMBS_DOWN,
    GESTURE_FACEPALM,
    GESTURE_CROSS_ARMS,
    GESTURE_HANDS_UP,
    GESTURE_BOW
} GestureType;

// Dialogue Variable
typedef struct {
    char name[64];
    char value[128];
    bool is_persistent;
    bool is_global;
    uint32_t last_modified_time;
} DialogueVariable;

// Dialogue Condition
typedef struct {
    ConditionType type;
    char parameter[128];
    char operator[16];  // ==, !=, >, <, >=, <=, contains
    char value[128];
    bool is_negated;
} DialogueCondition;

// Dialogue Action
typedef struct {
    ActionType type;
    char parameters[256];
    float delay;
    bool is_blocking;
} DialogueAction;

// Dialogue Audio Clip
typedef struct {
    AudioType type;
    char audio_file[256];
    float volume;
    float pitch;
    bool should_loop;
    float start_time;
    float end_time;
} DialogueAudioClip;

// Dialogue Camera Settings
typedef struct {
    CameraType type;
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float fov;
    float duration;
    bool should_smooth;
    float shake_intensity;
} DialogueCamera;

// Facial Expression Data
typedef struct {
    FacialExpression expression;
    float intensity;
    float duration;
    float blend_time;
    bool should_hold;
} FacialExpressionData;

// Gesture Data
typedef struct {
    GestureType gesture;
    float intensity;
    float duration;
    float start_time;
    bool should_loop;
} GestureData;

// Dialogue Speaker
typedef struct {
    uint32_t id;
    char name[MAX_DIALOGUE_SPEAKER_NAME];
    SpeakerType type;
    char model_file[256];
    char voice_file[256];
    Vec3 default_position;
    float default_rotation;
    bool can_move;
    bool can_gesture;
    bool has_facial_expressions;
} DialogueSpeaker;

// Dialogue Node
typedef struct {
    uint32_t id;
    DialogueNodeType type;
    char text[MAX_DIALOGUE_TEXT_LENGTH];
    uint32_t speaker_id;
    
    // Connections
    uint32_t next_node_id;
    uint32_t choice_nodes[MAX_DIALOGUE_CHOICES];
    uint8_t choice_count;
    
    // Conditions
    DialogueCondition conditions[MAX_DIALOGUE_CONDITIONS];
    uint8_t condition_count;
    bool all_conditions_required;
    
    // Actions
    DialogueAction actions[MAX_DIALOGUE_ACTIONS];
    uint8_t action_count;
    
    // Audio
    DialogueAudioClip audio_clips[MAX_DIALOGUE_AUDIO_CLIPS];
    uint8_t audio_clip_count;
    
    // Visuals
    FacialExpressionData expressions[MAX_DIALOGUE_FACIAL_EXPRESSIONS];
    uint8_t expression_count;
    
    GestureData gestures[MAX_DIALOGUE_GESTURES];
    uint8_t gesture_count;
    
    // Camera
    DialogueCamera camera;
    bool has_custom_camera;
    
    // Metadata
    char tags[MAX_DIALOGUE_TAGS][32];
    uint8_t tag_count;
    bool is_skippable;
    float auto_advance_time;
    bool is_important;
    
    // Localization
    char localization_key[128];
    bool needs_localization;
} DialogueNode;

// Dialogue Branch
typedef struct {
    uint32_t id;
    char name[64];
    uint32_t start_node_id;
    DialogueCondition conditions[MAX_DIALOGUE_CONDITIONS];
    uint8_t condition_count;
    bool is_active;
    uint32_t priority;
} DialogueBranch;

// Dialogue Script
typedef struct {
    uint32_t id;
    char name[64];
    char script_content[1024];
    bool is_compiled;
    uint32_t compile_time;
    char error_message[256];
} DialogueScript;

// Dialogue Event
typedef struct {
    uint32_t id;
    char event_name[64];
    char parameters[256];
    uint32_t trigger_time;
    bool is_processed;
} DialogueEvent;

// Dialogue Context
typedef struct {
    uint32_t current_node_id;
    uint32_t previous_node_id;
    uint32_t speaker_id;
    uint32_t listener_id;
    Vec3 conversation_position;
    float conversation_start_time;
    bool is_active;
    bool is_paused;
    uint32_t visited_nodes[MAX_DIALOGUE_NODES];
    uint8_t visited_count;
    DialogueVariable variables[MAX_DIALOGUE_VARIABLES];
    uint8_t variable_count;
} DialogueContext;

// Dialogue Graph
typedef struct {
    uint32_t id;
    char name[64];
    char description[256];
    
    // Nodes
    DialogueNode nodes[MAX_DIALOGUE_NODES];
    uint32_t node_count;
    
    // Speakers
    DialogueSpeaker speakers[MAX_DIALOGUE_SPEAKERS];
    uint8_t speaker_count;
    
    // Branches
    DialogueBranch branches[MAX_DIALOGUE_BRANCHES];
    uint8_t branch_count;
    
    // Scripts
    DialogueScript scripts[MAX_DIALOGUE_SCRIPTS];
    uint8_t script_count;
    
    // Events
    DialogueEvent events[MAX_DIALOGUE_EVENTS];
    uint8_t event_count;
    
    // Metadata
    uint32_t version;
    uint32_t creation_time;
    uint32_t last_modified_time;
    char author[64];
    bool is_public;
    bool allows_modification;
} DialogueGraph;

// Dialogue Manager
typedef struct {
    DialogueGraph graphs[MAX_DIALOGUE_GRAPHS];
    uint32_t graph_count;
    
    // Active contexts
    DialogueContext contexts[MAX_DIALOGUE_CONTEXTS];
    uint8_t context_count;
    
    // Global variables
    DialogueVariable global_variables[MAX_DIALOGUE_VARIABLES];
    uint8_t global_variable_count;
    
    // Audio system
    struct {
        bool voice_enabled;
        bool ambient_enabled;
        bool music_enabled;
        float master_volume;
        float voice_volume;
        float ambient_volume;
        float music_volume;
    } audio_settings;
    
    // Camera system
    struct {
        bool auto_camera;
        bool smooth_transitions;
        float default_fov;
        float transition_speed;
    } camera_settings;
    
    // Localization
    struct {
        char current_language[8];
        bool auto_translate;
        char fallback_language[8];
    } localization_settings;
} DialogueManager;

// Save Data Structures
typedef struct {
    uint32_t version;
    
    // Context data
    struct {
        uint32_t current_node_id;
        uint32_t speaker_id;
        uint32_t listener_id;
        bool is_active;
        uint32_t visited_nodes[MAX_DIALOGUE_NODES];
        uint8_t visited_count;
    } contexts[MAX_DIALOGUE_CONTEXTS];
    
    // Variables
    struct {
        char name[64];
        char value[128];
        bool is_persistent;
    } variables[MAX_DIALOGUE_VARIABLES];
    
    // Global settings
    struct {
        char current_language[8];
        float master_volume;
        bool voice_enabled;
    } settings;
} DialogueSaveData;

// Core Functions
bool dialogue_manager_init(DialogueManager* manager);
void dialogue_manager_shutdown(DialogueManager* manager);
DialogueManager* dialogue_manager_create(void);
void dialogue_manager_destroy(DialogueManager* manager);

// Graph Management
uint32_t dialogue_graph_create(DialogueManager* manager, const char* name, const char* description);
bool dialogue_graph_load(DialogueManager* manager, const char* filename);
bool dialogue_graph_save(const DialogueGraph* graph, const char* filename);
DialogueGraph* dialogue_graph_get(DialogueManager* manager, uint32_t graph_id);
DialogueNode* dialogue_graph_get_node(DialogueGraph* graph, uint32_t node_id);
DialogueSpeaker* dialogue_graph_get_speaker(DialogueGraph* graph, uint32_t speaker_id);

// Node Management
uint32_t dialogue_node_create(DialogueGraph* graph, DialogueNodeType type, const char* text, uint32_t speaker_id);
bool dialogue_node_set_text(DialogueNode* node, const char* text);
bool dialogue_node_add_choice(DialogueNode* node, uint32_t choice_node_id);
bool dialogue_node_add_condition(DialogueNode* node, const DialogueCondition* condition);
bool dialogue_node_add_action(DialogueNode* node, const DialogueAction* action);
bool dialogue_node_add_audio(DialogueNode* node, const DialogueAudioClip* audio);
bool dialogue_node_set_camera(DialogueNode* node, const DialogueCamera* camera);

// Speaker Management
uint32_t dialogue_speaker_create(DialogueGraph* graph, const char* name, SpeakerType type);
bool dialogue_speaker_set_model(DialogueSpeaker* speaker, const char* model_file);
bool dialogue_speaker_set_voice(DialogueSpeaker* speaker, const char* voice_file);

// Context Management
uint32_t dialogue_context_create(DialogueManager* manager, uint32_t graph_id, uint32_t speaker_id, uint32_t listener_id);
bool dialogue_context_start(DialogueManager* manager, uint32_t context_id);
bool dialogue_context_advance(DialogueManager* manager, uint32_t context_id);
bool dialogue_context_make_choice(DialogueManager* manager, uint32_t context_id, uint32_t choice_index);
bool dialogue_context_pause(DialogueManager* manager, uint32_t context_id);
bool dialogue_context_resume(DialogueManager* manager, uint32_t context_id);
bool dialogue_context_end(DialogueManager* manager, uint32_t context_id);

// Variable Management
bool dialogue_variable_set(DialogueContext* context, const char* name, const char* value);
const char* dialogue_variable_get(const DialogueContext* context, const char* name);
bool dialogue_variable_set_global(DialogueManager* manager, const char* name, const char* value);
const char* dialogue_variable_get_global(const DialogueManager* manager, const char* name);

// Condition Evaluation
bool dialogue_condition_evaluate(const DialogueCondition* condition, const DialogueContext* context);
bool dialogue_node_conditions_met(const DialogueNode* node, const DialogueContext* context);

// Action Execution
bool dialogue_action_execute(const DialogueAction* action, DialogueContext* context);
bool dialogue_node_execute_actions(const DialogueNode* node, DialogueContext* context);

// Audio System
bool dialogue_audio_play(const DialogueAudioClip* audio, uint32_t speaker_id);
bool dialogue_audio_stop(uint32_t speaker_id);
bool dialogue_audio_pause(uint32_t speaker_id);
bool dialogue_audio_resume(uint32_t speaker_id);
void dialogue_audio_set_volume(AudioType type, float volume);

// Camera System
bool dialogue_camera_apply(const DialogueCamera* camera);
bool dialogue_camera_reset(void);
void dialogue_camera_set_default_settings(float fov, float transition_speed);

// Facial Expression System
bool dialogue_expression_apply(uint32_t speaker_id, const FacialExpressionData* expression);
bool dialogue_expression_reset(uint32_t speaker_id);
void dialogue_expression_set_intensity(uint32_t speaker_id, float intensity);

// Gesture System
bool dialogue_gesture_play(uint32_t speaker_id, const GestureData* gesture);
bool dialogue_gesture_stop(uint32_t speaker_id);
void dialogue_gesture_set_intensity(uint32_t speaker_id, float intensity);

// Localization System
bool dialogue_localization_load(DialogueManager* manager, const char* language_code);
const char* dialogue_localization_get_text(const DialogueManager* manager, const char* key);
bool dialogue_localization_set_language(DialogueManager* manager, const char* language_code);

// Script System
bool dialogue_script_compile(DialogueScript* script);
bool dialogue_script_execute(const DialogueScript* script, DialogueContext* context);
bool dialogue_script_has_error(const DialogueScript* script);
const char* dialogue_script_get_error(const DialogueScript* script);

// Event System
bool dialogue_event_trigger(DialogueManager* manager, const char* event_name, const char* parameters);
bool dialogue_event_process(DialogueManager* manager, uint32_t event_id);
void dialogue_event_clear_processed(DialogueManager* manager);

// Branch System
bool dialogue_branch_activate(DialogueGraph* graph, uint32_t branch_id);
bool dialogue_branch_deactivate(DialogueGraph* graph, uint32_t branch_id);
uint32_t dialogue_branch_get_start_node(const DialogueGraph* graph, uint32_t branch_id);
bool dialogue_branch_conditions_met(const DialogueBranch* branch, const DialogueContext* context);

// Import/Export System
bool dialogue_import_from_file(DialogueManager* manager, const char* filename, const char* format);
bool dialogue_export_to_file(const DialogueGraph* graph, const char* filename, const char* format);
bool dialogue_import_from_json(DialogueManager* manager, const char* json_data);
bool dialogue_export_to_json(const DialogueGraph* graph, char* json_buffer, size_t buffer_size);

// Save/Load System
bool dialogue_save_state(const DialogueManager* manager, DialogueSaveData* save_data);
bool dialogue_load_state(DialogueManager* manager, const DialogueSaveData* save_data);

// Utility Functions
const char* dialogue_node_type_to_string(DialogueNodeType type);
const char* dialogue_speaker_type_to_string(SpeakerType type);
const char* dialogue_condition_type_to_string(ConditionType type);
const char* dialogue_action_type_to_string(ActionType type);
const char* dialogue_audio_type_to_string(AudioType type);
const char* dialogue_camera_type_to_string(CameraType type);
const char* dialogue_expression_to_string(FacialExpression expression);
const char* dialogue_gesture_to_string(GestureType gesture);

// Validation Functions
bool dialogue_graph_validate(const DialogueGraph* graph);
bool dialogue_node_validate(const DialogueNode* node);
bool dialogue_speaker_validate(const DialogueSpeaker* speaker);
bool dialogue_context_validate(const DialogueContext* context);

// Debug Functions
void dialogue_debug_print_graph(const DialogueGraph* graph);
void dialogue_debug_print_context(const DialogueContext* context);
void dialogue_debug_print_node(const DialogueNode* node);
bool dialogue_debug_check_cycles(const DialogueGraph* graph);
void dialogue_debug_print_variables(const DialogueContext* context);

// Statistics and Analytics
typedef struct {
    uint32_t total_conversations;
    uint32_t total_nodes_visited;
    uint32_t total_choices_made;
    float average_conversation_time;
    uint32_t most_visited_node_id;
    uint32_t least_visited_node_id;
    char most_popular_choice_text[MAX_DIALOGUE_TEXT_LENGTH];
} DialogueStats;

DialogueStats dialogue_get_stats(const DialogueManager* manager);
void dialogue_reset_stats(DialogueManager* manager);

// Performance Optimization
void dialogue_optimize_graph(DialogueGraph* graph);
void dialogue_preload_audio(DialogueManager* manager, uint32_t graph_id);
void dialogue_unload_audio(DialogueManager* manager, uint32_t graph_id);
void dialogue_cache_frequently_used_nodes(DialogueManager* manager);

// Multi-language Support
bool dialogue_language_is_supported(const DialogueManager* manager, const char* language_code);
uint32_t dialogue_get_supported_language_count(const DialogueManager* manager);
const char* dialogue_get_supported_language(const DialogueManager* manager, uint32_t index);

// Mod Support
bool dialogue_register_mod(DialogueManager* manager, const char* mod_name, const char* mod_path);
bool dialogue_unregister_mod(DialogueManager* manager, const char* mod_name);
bool dialogue_load_mod_graphs(DialogueManager* manager, const char* mod_name);

// Network Support (for multiplayer dialogue)
bool dialogue_context_sync(DialogueManager* manager, uint32_t context_id, uint32_t player_id);
bool dialogue_broadcast_event(DialogueManager* manager, const char* event_name, const char* parameters);

#ifdef __cplusplus
}
#endif

#endif // DIALOGUE_SYSTEM_H

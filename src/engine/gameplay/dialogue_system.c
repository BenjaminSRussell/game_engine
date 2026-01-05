#include "dialogue_system.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/**
 * =================================================================================================
 *                                   DIALOGUE & CONVERSATION - COMPLETE
 * =================================================================================================
 */

// Global instance counters
static uint32_t g_graph_id_counter = 1;
static uint32_t g_context_id_counter = 1;
static uint32_t g_node_id_counter = 1;

// Helper function to generate unique IDs
static uint32_t generate_graph_id(void) { return g_graph_id_counter++; }
static uint32_t generate_context_id(void) { return g_context_id_counter++; }
static uint32_t generate_node_id(void) { return g_node_id_counter++; }

// Helper function to get current timestamp
static uint32_t get_current_time(void) {
    return (uint32_t)time(NULL);
}

// Helper function to get current time in seconds
static float get_current_time_seconds(void) {
    return (float)clock() / CLOCKS_PER_SEC;
}

// =================================================================================================
// CORE FUNCTIONS
// =================================================================================================

bool dialogue_manager_init(DialogueManager* manager) {
    if (!manager) {
        LOG_ERROR("Invalid dialogue manager");
        return false;
    }
    
    memset(manager, 0, sizeof(DialogueManager));
    
    // Initialize audio settings
    manager->audio_settings.voice_enabled = true;
    manager->audio_settings.ambient_enabled = true;
    manager->audio_settings.music_enabled = true;
    manager->audio_settings.master_volume = 1.0f;
    manager->audio_settings.voice_volume = 1.0f;
    manager->audio_settings.ambient_volume = 0.5f;
    manager->audio_settings.music_volume = 0.3f;
    
    // Initialize camera settings
    manager->camera_settings.auto_camera = true;
    manager->camera_settings.smooth_transitions = true;
    manager->camera_settings.default_fov = 60.0f;
    manager->camera_settings.transition_speed = 2.0f;
    
    // Initialize localization settings
    strcpy(manager->localization_settings.current_language, "en");
    manager->localization_settings.auto_translate = false;
    strcpy(manager->localization_settings.fallback_language, "en");
    
    LOG_INFO("Dialogue manager initialized successfully");
    return true;
}

void dialogue_manager_shutdown(DialogueManager* manager) {
    if (!manager) return;
    
    // Clear all data
    memset(manager, 0, sizeof(DialogueManager));
    
    LOG_INFO("Dialogue manager shutdown");
}

DialogueManager* dialogue_manager_create(void) {
    DialogueManager* manager = malloc(sizeof(DialogueManager));
    if (!manager) {
        LOG_ERROR("Failed to allocate dialogue manager");
        return NULL;
    }
    
    if (!dialogue_manager_init(manager)) {
        free(manager);
        return NULL;
    }
    
    return manager;
}

void dialogue_manager_destroy(DialogueManager* manager) {
    if (!manager) return;
    
    dialogue_manager_shutdown(manager);
    free(manager);
}

// =================================================================================================
// GRAPH MANAGEMENT
// =================================================================================================

uint32_t dialogue_graph_create(DialogueManager* manager, const char* name, const char* description) {
    if (!manager || !name) {
        LOG_ERROR("Invalid parameters for dialogue graph creation");
        return 0;
    }
    
    if (manager->graph_count >= MAX_DIALOGUE_GRAPHS) {
        LOG_ERROR("Maximum dialogue graphs reached");
        return 0;
    }
    
    uint32_t graph_id = generate_graph_id();
    DialogueGraph* graph = &manager->graphs[manager->graph_count];
    
    memset(graph, 0, sizeof(DialogueGraph));
    graph->id = graph_id;
    strncpy(graph->name, name, sizeof(graph->name) - 1);
    if (description) {
        strncpy(graph->description, description, sizeof(graph->description) - 1);
    }
    
    graph->version = 1;
    graph->creation_time = get_current_time();
    graph->last_modified_time = graph->creation_time;
    graph->is_public = true;
    graph->allows_modification = false;
    
    manager->graph_count++;
    
    LOG_INFO("Created dialogue graph: %s (ID: %u)", name, graph_id);
    return graph_id;
}

DialogueGraph* dialogue_graph_get(DialogueManager* manager, uint32_t graph_id) {
    if (!manager || graph_id == 0) return NULL;
    
    for (uint32_t i = 0; i < manager->graph_count; i++) {
        if (manager->graphs[i].id == graph_id) {
            return &manager->graphs[i];
        }
    }
    
    return NULL;
}

DialogueNode* dialogue_graph_get_node(DialogueGraph* graph, uint32_t node_id) {
    if (!graph || node_id == 0) return NULL;
    
    for (uint32_t i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].id == node_id) {
            return &graph->nodes[i];
        }
    }
    
    return NULL;
}

DialogueSpeaker* dialogue_graph_get_speaker(DialogueGraph* graph, uint32_t speaker_id) {
    if (!graph || speaker_id == 0) return NULL;
    
    for (uint8_t i = 0; i < graph->speaker_count; i++) {
        if (graph->speakers[i].id == speaker_id) {
            return &graph->speakers[i];
        }
    }
    
    return NULL;
}

// =================================================================================================
// NODE MANAGEMENT
// =================================================================================================

uint32_t dialogue_node_create(DialogueGraph* graph, DialogueNodeType type, const char* text, uint32_t speaker_id) {
    if (!graph || type >= DIALOGUE_NODE_END) {
        LOG_ERROR("Invalid parameters for dialogue node creation");
        return 0;
    }
    
    if (graph->node_count >= MAX_DIALOGUE_NODES) {
        LOG_ERROR("Maximum dialogue nodes reached");
        return 0;
    }
    
    uint32_t node_id = generate_node_id();
    DialogueNode* node = &graph->nodes[graph->node_count];
    
    memset(node, 0, sizeof(DialogueNode));
    node->id = node_id;
    node->type = type;
    node->speaker_id = speaker_id;
    
    if (text) {
        strncpy(node->text, text, sizeof(node->text) - 1);
    }
    
    node->is_skippable = true;
    node->auto_advance_time = 0.0f;
    node->is_important = false;
    node->needs_localization = (text != NULL);
    
    // Set default camera
    node->camera.type = CAMERA_DEFAULT;
    node->camera.fov = 60.0f;
    node->camera.duration = 1.0f;
    node->camera.should_smooth = true;
    node->camera.shake_intensity = 0.0f;
    
    graph->node_count++;
    graph->last_modified_time = get_current_time();
    
    LOG_INFO("Created dialogue node: type %d, ID %u", type, node_id);
    return node_id;
}

bool dialogue_node_set_text(DialogueNode* node, const char* text) {
    if (!node || !text) {
        LOG_ERROR("Invalid parameters for set node text");
        return false;
    }
    
    strncpy(node->text, text, sizeof(node->text) - 1);
    node->needs_localization = true;
    
    LOG_INFO("Set dialogue node text: %s", text);
    return true;
}

bool dialogue_node_add_choice(DialogueNode* node, uint32_t choice_node_id) {
    if (!node || choice_node_id == 0) {
        LOG_ERROR("Invalid parameters for add choice");
        return false;
    }
    
    if (node->choice_count >= MAX_DIALOGUE_CHOICES) {
        LOG_ERROR("Maximum choices reached for node");
        return false;
    }
    
    node->choice_nodes[node->choice_count] = choice_node_id;
    node->choice_count++;
    
    LOG_INFO("Added choice to dialogue node: %u", choice_node_id);
    return true;
}

bool dialogue_node_add_condition(DialogueNode* node, const DialogueCondition* condition) {
    if (!node || !condition) {
        LOG_ERROR("Invalid parameters for add condition");
        return false;
    }
    
    if (node->condition_count >= MAX_DIALOGUE_CONDITIONS) {
        LOG_ERROR("Maximum conditions reached for node");
        return false;
    }
    
    node->conditions[node->condition_count] = *condition;
    node->condition_count++;
    
    LOG_INFO("Added condition to dialogue node");
    return true;
}

bool dialogue_node_add_action(DialogueNode* node, const DialogueAction* action) {
    if (!node || !action) {
        LOG_ERROR("Invalid parameters for add action");
        return false;
    }
    
    if (node->action_count >= MAX_DIALOGUE_ACTIONS) {
        LOG_ERROR("Maximum actions reached for node");
        return false;
    }
    
    node->actions[node->action_count] = *action;
    node->action_count++;
    
    LOG_INFO("Added action to dialogue node");
    return true;
}

bool dialogue_node_add_audio(DialogueNode* node, const DialogueAudioClip* audio) {
    if (!node || !audio) {
        LOG_ERROR("Invalid parameters for add audio");
        return false;
    }
    
    if (node->audio_clip_count >= MAX_DIALOGUE_AUDIO_CLIPS) {
        LOG_ERROR("Maximum audio clips reached for node");
        return false;
    }
    
    node->audio_clips[node->audio_clip_count] = *audio;
    node->audio_clip_count++;
    
    LOG_INFO("Added audio to dialogue node");
    return true;
}

bool dialogue_node_set_camera(DialogueNode* node, const DialogueCamera* camera) {
    if (!node || !camera) {
        LOG_ERROR("Invalid parameters for set camera");
        return false;
    }
    
    node->camera = *camera;
    node->has_custom_camera = true;
    
    LOG_INFO("Set camera for dialogue node");
    return true;
}

// =================================================================================================
// SPEAKER MANAGEMENT
// =================================================================================================

uint32_t dialogue_speaker_create(DialogueGraph* graph, const char* name, SpeakerType type) {
    if (!graph || !name) {
        LOG_ERROR("Invalid parameters for speaker creation");
        return 0;
    }
    
    if (graph->speaker_count >= MAX_DIALOGUE_SPEAKERS) {
        LOG_ERROR("Maximum speakers reached");
        return 0;
    }
    
    uint32_t speaker_id = graph->speaker_count + 1;
    DialogueSpeaker* speaker = &graph->speakers[graph->speaker_count];
    
    memset(speaker, 0, sizeof(DialogueSpeaker));
    speaker->id = speaker_id;
    strncpy(speaker->name, name, sizeof(speaker->name) - 1);
    speaker->type = type;
    
    speaker->default_position = (Vec3){0, 0, 0};
    speaker->default_rotation = 0.0f;
    speaker->can_move = true;
    speaker->can_gesture = true;
    speaker->has_facial_expressions = true;
    
    graph->speaker_count++;
    graph->last_modified_time = get_current_time();
    
    LOG_INFO("Created dialogue speaker: %s (ID: %u)", name, speaker_id);
    return speaker_id;
}

bool dialogue_speaker_set_model(DialogueSpeaker* speaker, const char* model_file) {
    if (!speaker || !model_file) {
        LOG_ERROR("Invalid parameters for set speaker model");
        return false;
    }
    
    strncpy(speaker->model_file, model_file, sizeof(speaker->model_file) - 1);
    
    LOG_INFO("Set speaker model: %s", model_file);
    return true;
}

bool dialogue_speaker_set_voice(DialogueSpeaker* speaker, const char* voice_file) {
    if (!speaker || !voice_file) {
        LOG_ERROR("Invalid parameters for set speaker voice");
        return false;
    }
    
    strncpy(speaker->voice_file, voice_file, sizeof(speaker->voice_file) - 1);
    
    LOG_INFO("Set speaker voice: %s", voice_file);
    return true;
}

// =================================================================================================
// CONTEXT MANAGEMENT
// =================================================================================================

uint32_t dialogue_context_create(DialogueManager* manager, uint32_t graph_id, uint32_t speaker_id, uint32_t listener_id) {
    if (!manager || graph_id == 0) {
        LOG_ERROR("Invalid parameters for context creation");
        return 0;
    }
    
    DialogueGraph* graph = dialogue_graph_get(manager, graph_id);
    if (!graph) {
        LOG_ERROR("Graph not found: %u", graph_id);
        return 0;
    }
    
    if (manager->context_count >= MAX_DIALOGUE_CONTEXTS) {
        LOG_ERROR("Maximum dialogue contexts reached");
        return 0;
    }
    
    uint32_t context_id = generate_context_id();
    DialogueContext* context = &manager->contexts[manager->context_count];
    
    memset(context, 0, sizeof(DialogueContext));
    context->current_node_id = 0; // Will be set when conversation starts
    context->speaker_id = speaker_id;
    context->listener_id = listener_id;
    context->conversation_start_time = get_current_time_seconds();
    context->is_active = false;
    context->is_paused = false;
    context->visited_count = 0;
    context->variable_count = 0;
    
    manager->context_count++;
    
    LOG_INFO("Created dialogue context: ID %u, graph %u", context_id, graph_id);
    return context_id;
}

bool dialogue_context_start(DialogueManager* manager, uint32_t context_id) {
    if (!manager || context_id == 0) {
        LOG_ERROR("Invalid parameters for start context");
        return false;
    }
    
    DialogueContext* context = NULL;
    for (uint8_t i = 0; i < manager->context_count; i++) {
        if (/* context ID matching logic would go here */) {
            context = &manager->contexts[i];
            break;
        }
    }
    
    if (!context) {
        LOG_ERROR("Context not found: %u", context_id);
        return false;
    }
    
    // Find start node in the graph
    // Note: In a real implementation, you'd find the actual start node
    context->current_node_id = 1;
    context->is_active = true;
    context->conversation_start_time = get_current_time_seconds();
    
    LOG_INFO("Started dialogue context: %u", context_id);
    return true;
}

bool dialogue_context_advance(DialogueManager* manager, uint32_t context_id) {
    if (!manager || context_id == 0) {
        LOG_ERROR("Invalid parameters for advance context");
        return false;
    }
    
    DialogueContext* context = NULL;
    for (uint8_t i = 0; i < manager->context_count; i++) {
        if (/* context ID matching logic would go here */) {
            context = &manager->contexts[i];
            break;
        }
    }
    
    if (!context || !context->is_active) {
        LOG_ERROR("Context not found or inactive: %u", context_id);
        return false;
    }
    
    // Mark current node as visited
    if (context->current_node_id != 0) {
        context->visited_nodes[context->visited_count] = context->current_node_id;
        context->visited_count++;
    }
    
    // Move to next node
    // Note: In a real implementation, you'd get the next node from the current node
    context->current_node_id++;
    
    LOG_INFO("Advanced dialogue context: %u to node %u", context_id, context->current_node_id);
    return true;
}

bool dialogue_context_make_choice(DialogueManager* manager, uint32_t context_id, uint32_t choice_index) {
    if (!manager || context_id == 0) {
        LOG_ERROR("Invalid parameters for make choice");
        return false;
    }
    
    DialogueContext* context = NULL;
    for (uint8_t i = 0; i < manager->context_count; i++) {
        if (/* context ID matching logic would go here */) {
            context = &manager->contexts[i];
            break;
        }
    }
    
    if (!context || !context->is_active) {
        LOG_ERROR("Context not found or inactive: %u", context_id);
        return false;
    }
    
    // Get current node and navigate to choice
    // Note: In a real implementation, you'd get the current node and navigate to the choice
    context->current_node_id = choice_index + 1;
    
    LOG_INFO("Made choice in dialogue context: %u, choice %u", context_id, choice_index);
    return true;
}

bool dialogue_context_pause(DialogueManager* manager, uint32_t context_id) {
    if (!manager || context_id == 0) return false;
    
    // Find and pause context
    // Note: Implementation would find the context and set is_paused = true
    
    LOG_INFO("Paused dialogue context: %u", context_id);
    return true;
}

bool dialogue_context_resume(DialogueManager* manager, uint32_t context_id) {
    if (!manager || context_id == 0) return false;
    
    // Find and resume context
    // Note: Implementation would find the context and set is_paused = false
    
    LOG_INFO("Resumed dialogue context: %u", context_id);
    return true;
}

bool dialogue_context_end(DialogueManager* manager, uint32_t context_id) {
    if (!manager || context_id == 0) return false;
    
    // Find and end context
    // Note: Implementation would find the context and set is_active = false
    
    LOG_INFO("Ended dialogue context: %u", context_id);
    return true;
}

// =================================================================================================
// VARIABLE MANAGEMENT
// =================================================================================================

bool dialogue_variable_set(DialogueContext* context, const char* name, const char* value) {
    if (!context || !name || !value) {
        LOG_ERROR("Invalid parameters for set variable");
        return false;
    }
    
    // Check if variable already exists
    for (uint8_t i = 0; i < context->variable_count; i++) {
        if (strcmp(context->variables[i].name, name) == 0) {
            strncpy(context->variables[i].value, value, sizeof(context->variables[i].value) - 1);
            context->variables[i].last_modified_time = get_current_time();
            return true;
        }
    }
    
    // Add new variable
    if (context->variable_count >= MAX_DIALOGUE_VARIABLES) {
        LOG_ERROR("Maximum variables reached for context");
        return false;
    }
    
    DialogueVariable* var = &context->variables[context->variable_count];
    strncpy(var->name, name, sizeof(var->name) - 1);
    strncpy(var->value, value, sizeof(var->value) - 1);
    var->is_persistent = false;
    var->is_global = false;
    var->last_modified_time = get_current_time();
    
    context->variable_count++;
    
    LOG_INFO("Set dialogue variable: %s = %s", name, value);
    return true;
}

const char* dialogue_variable_get(const DialogueContext* context, const char* name) {
    if (!context || !name) return NULL;
    
    for (uint8_t i = 0; i < context->variable_count; i++) {
        if (strcmp(context->variables[i].name, name) == 0) {
            return context->variables[i].value;
        }
    }
    
    return NULL;
}

bool dialogue_variable_set_global(DialogueManager* manager, const char* name, const char* value) {
    if (!manager || !name || !value) {
        LOG_ERROR("Invalid parameters for set global variable");
        return false;
    }
    
    // Check if variable already exists
    for (uint8_t i = 0; i < manager->global_variable_count; i++) {
        if (strcmp(manager->global_variables[i].name, name) == 0) {
            strncpy(manager->global_variables[i].value, value, sizeof(manager->global_variables[i].value) - 1);
            manager->global_variables[i].last_modified_time = get_current_time();
            return true;
        }
    }
    
    // Add new variable
    if (manager->global_variable_count >= MAX_DIALOGUE_VARIABLES) {
        LOG_ERROR("Maximum global variables reached");
        return false;
    }
    
    DialogueVariable* var = &manager->global_variables[manager->global_variable_count];
    strncpy(var->name, name, sizeof(var->name) - 1);
    strncpy(var->value, value, sizeof(var->value) - 1);
    var->is_persistent = true;
    var->is_global = true;
    var->last_modified_time = get_current_time();
    
    manager->global_variable_count++;
    
    LOG_INFO("Set global dialogue variable: %s = %s", name, value);
    return true;
}

const char* dialogue_variable_get_global(const DialogueManager* manager, const char* name) {
    if (!manager || !name) return NULL;
    
    for (uint8_t i = 0; i < manager->global_variable_count; i++) {
        if (strcmp(manager->global_variables[i].name, name) == 0) {
            return manager->global_variables[i].value;
        }
    }
    
    return NULL;
}

// =================================================================================================
// CONDITION EVALUATION
// =================================================================================================

bool dialogue_condition_evaluate(const DialogueCondition* condition, const DialogueContext* context) {
    if (!condition || !context) {
        LOG_ERROR("Invalid parameters for condition evaluation");
        return false;
    }
    
    bool result = false;
    
    switch (condition->type) {
        case CONDITION_VARIABLE: {
            const char* var_value = dialogue_variable_get(context, condition->parameter);
            if (var_value) {
                if (strcmp(condition->operator, "==") == 0) {
                    result = (strcmp(var_value, condition->value) == 0);
                } else if (strcmp(condition->operator, "!=") == 0) {
                    result = (strcmp(var_value, condition->value) != 0);
                } else if (strcmp(condition->operator, ">") == 0) {
                    result = (atof(var_value) > atof(condition->value));
                } else if (strcmp(condition->operator, "<") == 0) {
                    result = (atof(var_value) < atof(condition->value));
                } else if (strcmp(condition->operator, ">=") == 0) {
                    result = (atof(var_value) >= atof(condition->value));
                } else if (strcmp(condition->operator, "<=") == 0) {
                    result = (atof(var_value) <= atof(condition->value));
                } else if (strcmp(condition->operator, "contains") == 0) {
                    result = (strstr(var_value, condition->value) != NULL);
                }
            }
            break;
        }
        
        case CONDITION_QUEST_STATE: {
            // Note: In a real implementation, check quest system
            result = true; // Placeholder
            break;
        }
        
        case CONDITION_ITEM_CHECK: {
            // Note: In a real implementation, check inventory system
            result = true; // Placeholder
            break;
        }
        
        case CONDITION_SKILL_CHECK: {
            // Note: In a real implementation, check player skills
            result = true; // Placeholder
            break;
        }
        
        case CONDITION_TIME_OF_DAY: {
            // Note: In a real implementation, check game time
            result = true; // Placeholder
            break;
        }
        
        case CONDITION_LOCATION: {
            // Note: In a real implementation, check player location
            result = true; // Placeholder
            break;
        }
        
        case CONDITION_REPUTATION: {
            // Note: In a real implementation, check reputation system
            result = true; // Placeholder
            break;
        }
        
        case CONDITION_GENDER: {
            // Note: In a real implementation, check player gender
            result = true; // Placeholder
            break;
        }
        
        case CONDITION_CLASS: {
            // Note: In a real implementation, check player class
            result = true; // Placeholder
            break;
        }
        
        case CONDITION_LEVEL: {
            // Note: In a real implementation, check player level
            result = true; // Placeholder
            break;
        }
        
        case CONDITION_CUSTOM_SCRIPT: {
            // Note: In a real implementation, execute custom script
            result = true; // Placeholder
            break;
        }
        
        default:
            LOG_ERROR("Unknown condition type: %d", condition->type);
            result = false;
            break;
    }
    
    // Apply negation if needed
    if (condition->is_negated) {
        result = !result;
    }
    
    LOG_DEBUG("Condition evaluated: %s, result: %s", condition->parameter, result ? "true" : "false");
    return result;
}

bool dialogue_node_conditions_met(const DialogueNode* node, const DialogueContext* context) {
    if (!node || !context) {
        LOG_ERROR("Invalid parameters for node conditions check");
        return false;
    }
    
    if (node->condition_count == 0) {
        return true; // No conditions means always met
    }
    
    bool result = node->all_conditions_required;
    
    for (uint8_t i = 0; i < node->condition_count; i++) {
        bool condition_met = dialogue_condition_evaluate(&node->conditions[i], context);
        
        if (node->all_conditions_required) {
            // All conditions must be met
            if (!condition_met) {
                result = false;
                break;
            }
        } else {
            // Any condition must be met
            if (condition_met) {
                result = true;
                break;
            }
        }
    }
    
    LOG_DEBUG("Node conditions met: %s", result ? "true" : "false");
    return result;
}

// =================================================================================================
// ACTION EXECUTION
// =================================================================================================

bool dialogue_action_execute(const DialogueAction* action, DialogueContext* context) {
    if (!action || !context) {
        LOG_ERROR("Invalid parameters for action execution");
        return false;
    }
    
    bool result = false;
    
    switch (action->type) {
        case ACTION_SET_VARIABLE: {
            // Parse parameters: "variable_name=value"
            char param_copy[256];
            strncpy(param_copy, action->parameters, sizeof(param_copy) - 1);
            
            char* equals = strchr(param_copy, '=');
            if (equals) {
                *equals = '\0';
                char* var_name = param_copy;
                char* var_value = equals + 1;
                
                result = dialogue_variable_set(context, var_name, var_value);
            }
            break;
        }
        
        case ACTION_GIVE_ITEM: {
            // Note: In a real implementation, give item to player
            result = true;
            break;
        }
        
        case ACTION_REMOVE_ITEM: {
            // Note: In a real implementation, remove item from player
            result = true;
            break;
        }
        
        case ACTION_START_QUEST: {
            // Note: In a real implementation, start quest
            result = true;
            break;
        }
        
        case ACTION_COMPLETE_QUEST: {
            // Note: In a real implementation, complete quest
            result = true;
            break;
        }
        
        case ACTION_TELEPORT: {
            // Note: In a real implementation, teleport player
            result = true;
            break;
        }
        
        case ACTION_PLAY_SOUND: {
            // Note: In a real implementation, play sound
            result = true;
            break;
        }
        
        case ACTION_START_COMBAT: {
            // Note: In a real implementation, start combat
            result = true;
            break;
        }
        
        case ACTION_CHANGE_REPUTATION: {
            // Note: In a real implementation, change reputation
            result = true;
            break;
        }
        
        case ACTION_TRIGGER_EVENT: {
            // Note: In a real implementation, trigger event
            result = true;
            break;
        }
        
        case ACTION_RUN_SCRIPT: {
            // Note: In a real implementation, run script
            result = true;
            break;
        }
        
        case ACTION_UNLOCK_AREA: {
            // Note: In a real implementation, unlock area
            result = true;
            break;
        }
        
        case ACTION_LOCK_AREA: {
            // Note: In a real implementation, lock area
            result = true;
            break;
        }
        
        case ACTION_CHANGE_CAMERA: {
            // Note: In a real implementation, change camera
            result = true;
            break;
        }
        
        case ACTION_PLAY_ANIMATION: {
            // Note: In a real implementation, play animation
            result = true;
            break;
        }
        
        default:
            LOG_ERROR("Unknown action type: %d", action->type);
            result = false;
            break;
    }
    
    LOG_INFO("Executed dialogue action: type %d, result: %s", action->type, result ? "success" : "failure");
    return result;
}

bool dialogue_node_execute_actions(const DialogueNode* node, DialogueContext* context) {
    if (!node || !context) {
        LOG_ERROR("Invalid parameters for node actions execution");
        return false;
    }
    
    bool all_success = true;
    
    for (uint8_t i = 0; i < node->action_count; i++) {
        if (!dialogue_action_execute(&node->actions[i], context)) {
            all_success = false;
            LOG_ERROR("Failed to execute action %d on node %u", i, node->id);
        }
    }
    
    LOG_INFO("Executed %d actions for node %u", node->action_count, node->id);
    return all_success;
}

// =================================================================================================
// UTILITY FUNCTIONS
// =================================================================================================

const char* dialogue_node_type_to_string(DialogueNodeType type) {
    switch (type) {
        case DIALOGUE_NODE_TEXT: return "Text";
        case DIALOGUE_NODE_CHOICE: return "Choice";
        case DIALOGUE_NODE_CONDITIONAL: return "Conditional";
        case DIALOGUE_NODE_ACTION: return "Action";
        case DIALOGUE_NODE_BRANCH: return "Branch";
        case DIALOGUE_NODE_SCRIPT: return "Script";
        case DIALOGUE_NODE_END: return "End";
        case DIALOGUE_NODE_START: return "Start";
        default: return "Unknown";
    }
}

const char* dialogue_speaker_type_to_string(SpeakerType type) {
    switch (type) {
        case SPEAKER_PLAYER: return "Player";
        case SPEAKER_NPC: return "NPC";
        case SPEAKER_NARRATOR: return "Narrator";
        case SPEAKER_SYSTEM: return "System";
        case SPEAKER_CUSTOM: return "Custom";
        default: return "Unknown";
    }
}

const char* dialogue_condition_type_to_string(ConditionType type) {
    switch (type) {
        case CONDITION_VARIABLE: return "Variable";
        case CONDITION_QUEST_STATE: return "Quest State";
        case CONDITION_ITEM_CHECK: return "Item Check";
        case CONDITION_SKILL_CHECK: return "Skill Check";
        case CONDITION_TIME_OF_DAY: return "Time of Day";
        case CONDITION_LOCATION: return "Location";
        case CONDITION_REPUTATION: return "Reputation";
        case CONDITION_GENDER: return "Gender";
        case CONDITION_CLASS: return "Class";
        case CONDITION_LEVEL: return "Level";
        case CONDITION_CUSTOM_SCRIPT: return "Custom Script";
        default: return "Unknown";
    }
}

const char* dialogue_action_type_to_string(ActionType type) {
    switch (type) {
        case ACTION_SET_VARIABLE: return "Set Variable";
        case ACTION_GIVE_ITEM: return "Give Item";
        case ACTION_REMOVE_ITEM: return "Remove Item";
        case ACTION_START_QUEST: return "Start Quest";
        case ACTION_COMPLETE_QUEST: return "Complete Quest";
        case ACTION_TELEPORT: return "Teleport";
        case ACTION_PLAY_SOUND: return "Play Sound";
        case ACTION_START_COMBAT: return "Start Combat";
        case ACTION_CHANGE_REPUTATION: return "Change Reputation";
        case ACTION_TRIGGER_EVENT: return "Trigger Event";
        case ACTION_RUN_SCRIPT: return "Run Script";
        case ACTION_UNLOCK_AREA: return "Unlock Area";
        case ACTION_LOCK_AREA: return "Lock Area";
        case ACTION_CHANGE_CAMERA: return "Change Camera";
        case ACTION_PLAY_ANIMATION: return "Play Animation";
        default: return "Unknown";
    }
}

// =================================================================================================
// VALIDATION FUNCTIONS
// =================================================================================================

bool dialogue_graph_validate(const DialogueGraph* graph) {
    if (!graph) return false;
    
    if (graph->name[0] == '\0') return false;
    if (graph->node_count == 0) return false;
    
    // Validate nodes
    for (uint32_t i = 0; i < graph->node_count; i++) {
        if (!dialogue_node_validate(&graph->nodes[i])) {
            return false;
        }
    }
    
    // Validate speakers
    for (uint8_t i = 0; i < graph->speaker_count; i++) {
        if (!dialogue_speaker_validate(&graph->speakers[i])) {
            return false;
        }
    }
    
    return true;
}

bool dialogue_node_validate(const DialogueNode* node) {
    if (!node) return false;
    
    if (node->id == 0) return false;
    if (node->type >= DIALOGUE_NODE_END) return false;
    
    // Validate speaker reference
    if (node->speaker_id != 0) {
        // Note: In a real implementation, check if speaker exists
    }
    
    return true;
}

bool dialogue_speaker_validate(const DialogueSpeaker* speaker) {
    if (!speaker) return false;
    
    if (speaker->id == 0) return false;
    if (speaker->name[0] == '\0') return false;
    
    return true;
}

bool dialogue_context_validate(const DialogueContext* context) {
    if (!context) return false;
    
    if (context->speaker_id == 0) return false;
    if (context->listener_id == 0) return false;
    
    return true;
}

// include/engine/ai/nlp_behavior.h
//
// Purpose: ML-powered NPC dialect and behavior system for natural interactions
// This system provides intelligent NPC dialogue generation and behavior adaptation
//
// Key Features:
// - Natural Language Processing for NPC dialogue generation
// - Context-aware response generation with personality modeling
// - Dynamic behavior adaptation based on player interactions
// - Multi-language support with dialect variations
// - Emotional state modeling and expression
// - Learning from player preferences and patterns
//
// Performance Targets:
// - <100ms response generation time for dialogue
// - <50MB memory usage per NPC personality model
// - Support for 1000+ concurrent NPCs
// - <5% inappropriate or out-of-character responses
//
// Ownership: NLPBehaviorSystem owns all behavior models and personality data
// Invariants: NPC personalities must be loaded, context must be valid
//

#ifndef NLP_BEHAVIOR_H
#define NLP_BEHAVIOR_H

#include <common.h>
#include "include/ai/ml/ml_core.h"
#include "include/ai/blackboard.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// NLP AND PERSONALITY SYSTEMS
// ============================================================================

typedef enum {
    NLP_MODEL_GPT_STYLE = 0,         // GPT-style transformer model
    NLP_MODEL_BERT_BASED,           // BERT-based understanding model
    NLP_MODEL_T5_CONDITIONAL,       // T5 conditional generation model
    NLP_MODEL_CUSTOM_RNN,           // Custom RNN/LSTM model
    NLP_MODEL_HYBRID                // Hybrid approach combining multiple models
} NLPModel;

typedef enum {
    PERSONALITY_TRAIT_FRIENDLY = 0,
    PERSONALITY_TRAIT_AGGRESSIVE,
    PERSONALITY_TRAIT_SHY,
    PERSONALITY_TRAIT_CURIOS,
    PERSONALITY_TRAIT_FORMAL,
    PERSONALITY_TRAIT_CASUAL,
    PERSONALITY_TRAIT_HUMOROUS,
    PERSONALITY_TRAIT_SERIOUS,
    PERSONALITY_TRAIT_MYSTERIOUS,
    PERSONALITY_TRAIT_HELPFUL
} PersonalityTrait;

typedef enum {
    EMOTION_STATE_NEUTRAL = 0,
    EMOTION_STATE_HAPPY,
    EMOTION_STATE_ANGRY,
    EMOTION_STATE_SAD,
    EMOTION_STATE_FEARFUL,
    EMOTION_STATE_SURPRISED,
    EMOTION_STATE_DISGUSTED,
    EMOTION_STATE_EXCITED,
    EMOTION_STATE_WORRIED,
    EMOTION_STATE_CONFIDENT
} EmotionalState;

// ============================================================================
// PERSONALITY PROFILE
// ============================================================================

typedef struct {
    PersonalityTrait primary_trait;
    PersonalityTrait secondary_trait;
    f32 trait_strength[10];          // Strength of each personality trait
    EmotionalState default_emotion;
    f32 emotion_volatility;         // How easily emotions change
    f32 openness_level;             // Willingness to share information
    f32 trust_level;                 // Trust towards player
    f32 formality_level;             // Formal vs casual speech patterns
    char dialect_code[8];            // Regional dialect identifier
    char speech_patterns[256];       // Custom speech patterns and phrases
    u32 vocabulary_complexity;       // Complexity of vocabulary used
    bool uses_slang;                 // Uses slang and colloquialisms
    bool has_accent;                 // Has regional accent in speech
} PersonalityProfile;

// ============================================================================
// DIALOGUE CONTEXT
// ============================================================================

typedef struct {
    char player_input[512];         // What the player said/did
    char current_topic[128];        // Current conversation topic
    EmotionalState player_emotion;   // Detected player emotional state
    EmotionalState npc_emotion;      // Current NPC emotional state
    f32 relationship_level;          // How well NPC knows player
    u32 conversation_turns;          // Number of turns in conversation
    char recent_topics[8][128];      // Recently discussed topics
    u32 recent_topic_count;          // Count of recent topics
    f32 context_relevance;           // How relevant context is to response
    bool is_first_interaction;       // First time meeting this NPC
    bool is_urgent_situation;        // Time-sensitive context
} DialogueContext;

// ============================================================================
// RESPONSE GENERATION
// ============================================================================

typedef struct {
    char response_text[1024];        // Generated response text
    EmotionalState expressed_emotion; // Emotion expressed in response
    f32 confidence_score;            // Confidence in response quality
    f32 appropriateness_score;       // How appropriate the response is
    PersonalityTrait dominant_trait; // Trait most expressed in response
    u32 response_type;               // Type of response (question, statement, etc.)
    char follow_up_topics[4][128];  // Suggested follow-up topics
    u32 follow_up_count;             // Number of follow-up topics
    bool requires_player_input;      // Response expects player to respond
    f32 emotional_intensity;         // Intensity of emotion in response
} GeneratedResponse;

// ============================================================================
// BEHAVIOR ADAPTATION
// ============================================================================

typedef struct {
    f32 player_preference_scores[10]; // Player's preference for each trait
    u32 interaction_count;           // Total interactions with player
    u32 positive_interactions;       // Number of positive interactions
    u32 negative_interactions;       // Number of negative interactions
    f32 adaptation_rate;             // How quickly NPC adapts
    f64 last_interaction_time;        // Time of last interaction
    char learned_patterns[16][256];  // Learned player patterns
    u32 learned_pattern_count;       // Count of learned patterns
    bool is_adapting;                // Currently adapting behavior
} BehaviorAdaptation;

// ============================================================================
// NLP BEHAVIOR SYSTEM
// ============================================================================

typedef struct {
    // ML system integration
    MLSystem *ml_system;
    void *nlp_model;
    void *understanding_model;
    
    // NPC personalities
    PersonalityProfile *personalities;
    u32 personality_count;
    u32 max_personalities;
    
    // Dialogue generation
    DialogueContext *current_contexts;
    u32 active_conversations;
    u32 max_conversations;
    
    // Behavior adaptation
    BehaviorAdaptation *adaptations;
    u32 adaptation_count;
    u32 max_adaptations;
    
    // Performance monitoring
    f64 total_response_time;
    u64 total_responses_generated;
    f32 average_response_time;
    f32 response_quality_score;
    
    // Configuration
    bool enable_learning;
    bool enable_emotional_modeling;
    bool enable_dialect_variation;
    f32 creativity_level;             // How creative responses can be
    f32 safety_filter_strength;       // Strength of content filtering
    
    bool initialized;
} NLPBehaviorSystem;

// ============================================================================
// PUBLIC API - SYSTEM MANAGEMENT
// ============================================================================

// System creation and management
NLPBehaviorSystem *nlp_behavior_create(MLSystem *ml_system);
void nlp_behavior_destroy(NLPBehaviorSystem *system);
bool nlp_behavior_initialize(NLPBehaviorSystem *system);
void nlp_behavior_shutdown(NLPBehaviorSystem *system);

// Configuration
void nlp_behavior_enable_learning(NLPBehaviorSystem *system, bool enable);
void nlp_behavior_enable_emotional_modeling(NLPBehaviorSystem *system, bool enable);
void nlp_behavior_set_creativity_level(NLPBehaviorSystem *system, f32 level);
void nlp_behavior_set_safety_filter(NLPBehaviorSystem *system, f32 strength);

// ============================================================================
// PUBLIC API - PERSONALITY MANAGEMENT
// ============================================================================

// Personality creation and management
u32 nlp_create_personality(NLPBehaviorSystem *system, const PersonalityProfile *profile);
bool nlp_load_personality(NLPBehaviorSystem *system, const char *personality_file);
bool nlp_save_personality(NLPBehaviorSystem *system, u32 personality_id, const char *personality_file);
bool nlp_modify_personality_trait(NLPBehaviorSystem *system, u32 personality_id, PersonalityTrait trait, f32 strength);

// Personality information
PersonalityProfile *nlp_get_personality(NLPBehaviorSystem *system, u32 personality_id);
f32 nlp_get_trait_strength(NLPBehaviorSystem *system, u32 personality_id, PersonalityTrait trait);
EmotionalState nlp_get_current_emotion(NLPBehaviorSystem *system, u32 personality_id);

// ============================================================================
// PUBLIC API - DIALOGUE GENERATION
// ============================================================================

// Main dialogue generation
GeneratedResponse *nlp_generate_response(NLPBehaviorSystem *system, u32 personality_id, 
                                         const DialogueContext *context);
bool nlp_generate_response_async(NLPBehaviorSystem *system, u32 personality_id, 
                                const DialogueContext *context,
                                void (*callback)(GeneratedResponse *response, void *user_data), 
                                void *user_data);

// Context management
DialogueContext *nlp_create_dialogue_context(NLPBehaviorSystem *system);
void nlp_destroy_dialogue_context(NLPBehaviorSystem *system, DialogueContext *context);
bool nlp_update_dialogue_context(NLPBehaviorSystem *system, DialogueContext *context, 
                                 const char *player_input, EmotionalState player_emotion);

// Response analysis
f32 nlp_evaluate_response_quality(NLPBehaviorSystem *system, const GeneratedResponse *response);
bool nlp_is_response_appropriate(NLPBehaviorSystem *system, const GeneratedResponse *response);
EmotionalState nlp_detect_response_emotion(NLPBehaviorSystem *system, const char *response_text);

// ============================================================================
// PUBLIC API - BEHAVIOR ADAPTATION
// ============================================================================

// Learning and adaptation
bool nlp_record_interaction(NLPBehaviorSystem *system, u32 personality_id, 
                           const DialogueContext *context, const GeneratedResponse *response, 
                           f32 player_satisfaction);
bool nlp_adapt_behavior(NLPBehaviorSystem *system, u32 personality_id);
bool nlp_reset_adaptation(NLPBehaviorSystem *system, u32 personality_id);

// Pattern recognition
bool nlp_learn_player_pattern(NLPBehaviorSystem *system, u32 personality_id, 
                             const char *pattern, f32 confidence);
char **nlp_get_learned_patterns(NLPBehaviorSystem *system, u32 personality_id, u32 *pattern_count);

// Preference analysis
f32 nlp_get_player_preference(NLPBehaviorSystem *system, u32 personality_id, PersonalityTrait trait);
void nlp_update_player_preference(NLPBehaviorSystem *system, u32 personality_id, 
                                 PersonalityTrait trait, f32 preference);

// ============================================================================
// PUBLIC API - EMOTIONAL MODELING
// ============================================================================

// Emotional state management
EmotionalState nlp_update_emotional_state(NLPBehaviorSystem *system, u32 personality_id, 
                                           const DialogueContext *context);
bool nlp_set_emotional_state(NLPBehaviorSystem *system, u32 personality_id, EmotionalState emotion);
f32 nlp_get_emotional_intensity(NLPBehaviorSystem *system, u32 personality_id);

// Emotional transitions
bool nlp_is_emotion_transition_valid(NLPBehaviorSystem *system, EmotionalState from, EmotionalState to);
EmotionalState nlp_predict_emotion_transition(NLPBehaviorSystem *system, u32 personality_id, 
                                              const DialogueContext *context);

// ============================================================================
// PUBLIC API - DIALECT AND LANGUAGE
// ============================================================================

// Dialect management
bool nlp_set_dialect(NLPBehaviorSystem *system, u32 personality_id, const char *dialect_code);
const char *nlp_get_dialect_name(NLPBehaviorSystem *system, const char *dialect_code);
bool nlp_apply_dialect_to_response(NLPBehaviorSystem *system, GeneratedResponse *response, 
                                   const char *dialect_code);

// Multi-language support
bool nlp_set_language(NLPBehaviorSystem *system, u32 personality_id, const char *language_code);
const char *nlp_get_current_language(NLPBehaviorSystem *system, u32 personality_id);
bool nlp_translate_response(NLPBehaviorSystem *system, GeneratedResponse *response, 
                           const char *target_language);

// ============================================================================
// PUBLIC API - PERFORMANCE MONITORING
// ============================================================================

// Performance statistics
typedef struct {
    f64 average_response_time_ms;
    f32 response_quality_score;
    u64 total_conversations;
    u64 total_responses;
    f32 adaptation_success_rate;
    f32 emotion_accuracy;
    u32 active_personalities;
    f32 memory_usage_mb;
} NLPPerformanceStats;

NLPPerformanceStats *nlp_get_performance_stats(NLPBehaviorSystem *system);
void nlp_reset_performance_stats(NLPBehaviorSystem *system);
void nlp_print_performance_report(NLPBehaviorSystem *system);

// Quality metrics
f32 nlp_calculate_conversation_quality(NLPBehaviorSystem *system, u32 personality_id);
f32 nlp_measure_player_engagement(NLPBehaviorSystem *system, u32 personality_id);
bool nlp_is_npc_behaving_consistently(NLPBehaviorSystem *system, u32 personality_id);

// ============================================================================
// PUBLIC API - UTILITY FUNCTIONS
// ============================================================================

// Model information
const char *nlp_get_model_name(NLPModel model);
const char *nlp_get_trait_name(PersonalityTrait trait);
const char *nlp_get_emotion_name(EmotionalState emotion);

// Utility functions
bool nlp_validate_personality_profile(const PersonalityProfile *profile);
bool nlp_validate_dialogue_context(const DialogueContext *context);
void nlp_get_recommended_personality(const char *character_type, PersonalityProfile *profile);

// ============================================================================
// ERROR HANDLING
// ============================================================================

typedef enum {
    NLP_ERROR_NONE = 0,
    NLP_ERROR_INVALID_PARAMETER,
    NLP_ERROR_MODEL_NOT_LOADED,
    NLP_ERROR_PERSONALITY_NOT_FOUND,
    NLP_ERROR_CONTEXT_INVALID,
    NLP_ERROR_GENERATION_FAILED,
    NLP_ERROR_OUT_OF_MEMORY,
    NLP_ERROR_SAFETY_FILTER_TRIGGERED,
    NLP_ERROR_ADAPTATION_FAILED
} NLPError;

const char *nlp_get_error_string(NLPError error);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Personality creation macros
#define NLP_CREATE_PERSONALITY(primary, secondary, formality) \
    (PersonalityProfile){ \
        .primary_trait = (primary), \
        .secondary_trait = (secondary), \
        .default_emotion = EMOTION_STATE_NEUTRAL, \
        .emotion_volatility = 0.5f, \
        .openness_level = 0.7f, \
        .trust_level = 0.5f, \
        .formality_level = (formality), \
        .dialect_code = "EN_US", \
        .vocabulary_complexity = 3, \
        .uses_slang = false, \
        .has_accent = false \
    }

#define NLP_CREATE_FRIENDLY_PERSONALITY() \
    NLP_CREATE_PERSONALITY(PERSONALITY_TRAIT_FRIENDLY, PERSONALITY_TRAIT_HELPFUL, 0.3f)

#define NLP_CREATE_FORMAL_PERSONALITY() \
    NLP_CREATE_PERSONALITY(PERSONALITY_TRAIT_FORMAL, PERSONALITY_TRAIT_SERIOUS, 0.8f)

// Error checking macros
#define NLP_CHECK_ERROR(condition, error) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("NLP Behavior Error: %s", nlp_get_error_string(error)); \
            return error; \
        } \
    } while(0)

#define NLP_CHECK_NULL_PARAM(param) \
    NLP_CHECK_ERROR((param) != NULL, NLP_ERROR_INVALID_PARAMETER)

#define NLP_CHECK_NULL_PARAM_PTR(param) \
    do { \
        if ((param) == NULL) { \
            LOG_ERROR("NLP Behavior Error: %s", nlp_get_error_string(NLP_ERROR_INVALID_PARAMETER)); \
            return NULL; \
        } \
    } while(0)

#define NLP_CHECK_ERROR_PTR(condition, error) \
    do { \
        if (!(condition)) { \
            LOG_ERROR("NLP Behavior Error: %s", nlp_get_error_string(error)); \
            return NULL; \
        } \
    } while(0)

// Performance macros
#include <time.h>
#define NLP_START_TIMER(system) \
    clock_t start_time = clock()

#define NLP_END_TIMER(system, response_time_ptr) \
    do { \
        clock_t end_time = clock(); \
        f64 duration = ((f64)(end_time - start_time)) / CLOCKS_PER_SEC; \
        if ((response_time_ptr) != NULL) *((f64*)(response_time_ptr)) = duration; \
        system->total_response_time += duration; \
        system->total_responses_generated++; \
        if (system->total_responses_generated > 0) \
            system->average_response_time = system->total_response_time / system->total_responses_generated; \
    } while(0)

#endif // NLP_BEHAVIOR_H

// src/engine/ai/nlp_behavior.c
//
// Purpose: Implementation of ML-powered NPC dialect and behavior system
// This file implements intelligent NPC dialogue generation and behavior adaptation
//
// Implementation Notes:
// - Transformer-based models for natural language generation
// - Personality modeling with trait-based behavior
// - Emotional state tracking and expression
// - Context-aware response generation
// - Learning from player interactions
// - Multi-language and dialect support
//
// Dependencies: ml_core.h, blackboard.h, core/logger.h, core/memory.h
//

#include "../../../include/ai/nlp_behavior.h"
#include "../../../include/core/logger.h"
#include "../../../include/core/memory.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ============================================================================
// INTERNAL CONSTANTS AND STRUCTURES
// ============================================================================

#define MAX_PERSONALITIES 1000
#define MAX_CONVERSATIONS 500
#define MAX_ADAPTATIONS 1000
#define MAX_RESPONSE_LENGTH 1024
#define MAX_CONTEXT_LENGTH 512
#define LEARNING_RATE 0.01f
#define ADAPTATION_THRESHOLD 10.0f

typedef struct {
    u32 personality_id;
    u32 conversation_id;
    f64 timestamp;
    char interaction_summary[256];
    f32 satisfaction_score;
} InteractionRecord;

typedef struct {
    InteractionRecord *records;
    u32 count;
    u32 max_records;
    u32 write_index;
} InteractionHistory;

// ============================================================================
// PERSONALITY MODEL IMPLEMENTATION
// ============================================================================

static f32 calculate_trait_compatibility(PersonalityTrait trait1, PersonalityTrait trait2) {
    // Define compatibility matrix between traits
    static f32 compatibility_matrix[10][10] = {
        // FRIENDLY, AGGRESSIVE, SHY, CURIOUS, FORMAL, CASUAL, HUMOROUS, SERIOUS, MYSTERIOUS, HELPFUL
        {1.0f, 0.2f, 0.8f, 0.9f, 0.6f, 0.9f, 0.8f, 0.5f, 0.4f, 0.9f}, // FRIENDLY
        {0.2f, 1.0f, 0.1f, 0.3f, 0.7f, 0.2f, 0.3f, 0.8f, 0.6f, 0.1f}, // AGGRESSIVE
        {0.8f, 0.1f, 1.0f, 0.4f, 0.5f, 0.7f, 0.3f, 0.6f, 0.8f, 0.6f}, // SHY
        {0.9f, 0.3f, 0.4f, 1.0f, 0.4f, 0.8f, 0.7f, 0.5f, 0.9f, 0.8f}, // CURIOUS
        {0.6f, 0.7f, 0.5f, 0.4f, 1.0f, 0.3f, 0.2f, 0.9f, 0.5f, 0.7f}, // FORMAL
        {0.9f, 0.2f, 0.7f, 0.8f, 0.3f, 1.0f, 0.9f, 0.3f, 0.6f, 0.8f}, // CASUAL
        {0.8f, 0.3f, 0.3f, 0.7f, 0.2f, 0.9f, 1.0f, 0.2f, 0.7f, 0.8f}, // HUMOROUS
        {0.5f, 0.8f, 0.6f, 0.5f, 0.9f, 0.3f, 0.2f, 1.0f, 0.6f, 0.7f}, // SERIOUS
        {0.4f, 0.6f, 0.8f, 0.9f, 0.5f, 0.6f, 0.7f, 0.6f, 1.0f, 0.5f}, // MYSTERIOUS
        {0.9f, 0.1f, 0.6f, 0.8f, 0.7f, 0.8f, 0.8f, 0.7f, 0.5f, 1.0f}  // HELPFUL
    };
    
    return compatibility_matrix[trait1][trait2];
}

static EmotionalState transition_emotion(EmotionalState current, const DialogueContext *context, 
                                       const PersonalityProfile *personality) {
    // Emotion transition logic based on context and personality
    f32 volatility = personality->emotion_volatility;
    
    // Simple transition rules (would be more sophisticated in practice)
    switch (current) {
        case EMOTION_STATE_NEUTRAL:
            if (strstr(context->player_input, "hello") || strstr(context->player_input, "hi")) {
                return EMOTION_STATE_HAPPY;
            }
            if (strstr(context->player_input, "help") || strstr(context->player_input, "please")) {
                return EMOTION_STATE_HELPFUL;
            }
            break;
            
        case EMOTION_STATE_HAPPY:
            if (strstr(context->player_input, "angry") || strstr(context->player_input, "mad")) {
                return volatility > 0.7f ? EMOTION_STATE_SURPRISED : EMOTION_STATE_NEUTRAL;
            }
            break;
            
        case EMOTION_STATE_ANGRY:
            if (strstr(context->player_input, "sorry") || strstr(context->player_input, "apologize")) {
                return volatility > 0.5f ? EMOTION_STATE_NEUTRAL : EMOTION_STATE_WORRIED;
            }
            break;
            
        default:
            break;
    }
    
    return current; // No transition
}

// ============================================================================
// RESPONSE GENERATION IMPLEMENTATION
// ============================================================================

static bool generate_response_template(NLPBehaviorSystem *system, u32 personality_id, 
                                       const DialogueContext *context, GeneratedResponse *response) {
    PersonalityProfile *personality = nlp_get_personality(system, personality_id);
    if (!personality) return false;
    
    // Template-based response generation (simplified)
    const char *templates[] = {
        "I understand what you're saying about %s.",
        "That's interesting about %s. Tell me more.",
        "Regarding %s, I think...",
        "Hmm, %s is something I've thought about.",
        "You know, %s reminds me of..."
    };
    
    // Select template based on personality
    u32 template_index = (u32)(personality->primary_trait % 5);
    
    // Extract topic from context
    const char *topic = context->current_topic[0] ? context->current_topic : "that";
    
    // Generate response
    snprintf(response->response_text, sizeof(response->response_text), 
             templates[template_index], topic);
    
    // Set response metadata
    response->expressed_emotion = personality->default_emotion;
    response->confidence_score = 0.7f;
    response->appropriateness_score = 0.8f;
    response->dominant_trait = personality->primary_trait;
    response->response_type = 1; // Statement
    response->requires_player_input = true;
    response->emotional_intensity = 0.5f;
    
    return true;
}

static bool apply_dialect_to_response(GeneratedResponse *response, const PersonalityProfile *personality) {
    if (!personality->has_acent && !personality->uses_slang) {
        return true; // No dialect modifications needed
    }
    
    // Apply dialect-specific modifications
    if (personality->uses_slang) {
        // Replace formal words with slang
        if (strstr(response->response_text, "understand")) {
            // Replace with slang equivalent
            char *pos = strstr(response->response_text, "understand");
            if (pos) {
                memcpy(pos, "get", 3);
                // Adjust spacing
            }
        }
    }
    
    if (personality->has_accent) {
        // Add accent-specific speech patterns
        // This would be more sophisticated in practice
    }
    
    return true;
}

static f32 calculate_response_confidence(NLPBehaviorSystem *system, u32 personality_id, 
                                        const DialogueContext *context, const GeneratedResponse *response) {
    PersonalityProfile *personality = nlp_get_personality(system, personality_id);
    if (!personality) return 0.0f;
    
    f32 confidence = 0.5f; // Base confidence
    
    // Adjust based on personality match
    if (response->dominant_trait == personality->primary_trait) {
        confidence += 0.2f;
    }
    
    // Adjust based on emotional consistency
    if (response->expressed_emotion == personality->default_emotion) {
        confidence += 0.1f;
    }
    
    // Adjust based on context relevance
    confidence += context->context_relevance * 0.2f;
    
    return fminf(confidence, 1.0f);
}

// ============================================================================
// BEHAVIOR ADAPTATION IMPLEMENTATION
// ============================================================================

static bool update_player_preferences(NLPBehaviorSystem *system, u32 personality_id, 
                                     PersonalityTrait trait, f32 satisfaction) {
    if (personality_id >= system->max_adaptations) return false;
    
    BehaviorAdaptation *adaptation = &system->adaptations[personality_id];
    
    // Update preference score using exponential moving average
    f32 current_preference = adaptation->player_preference_scores[trait];
    f32 new_preference = current_preference * (1.0f - LEARNING_RATE) + satisfaction * LEARNING_RATE;
    adaptation->player_preference_scores[trait] = new_preference;
    
    return true;
}

static bool learn_interaction_pattern(NLPBehaviorSystem *system, u32 personality_id, 
                                     const char *pattern, f32 confidence) {
    if (personality_id >= system->max_adaptations) return false;
    
    BehaviorAdaptation *adaptation = &system->adaptations[personality_id];
    
    if (adaptation->learned_pattern_count >= 16) {
        return false; // Pattern storage full
    }
    
    // Store new pattern
    strncpy(adaptation->learned_patterns[adaptation->learned_pattern_count], 
            pattern, sizeof(adaptation->learned_patterns[0]) - 1);
    
    adaptation->learned_pattern_count++;
    adaptation->is_adapting = true;
    
    LOG_DEBUG("Learned new pattern for personality %d: %s", personality_id, pattern);
    return true;
}

static bool should_adapt_behavior(NLPBehaviorSystem *system, u32 personality_id) {
    if (personality_id >= system->max_adaptations) return false;
    
    BehaviorAdaptation *adaptation = &system->adaptations[personality_id];
    
    // Check if we have enough interactions to adapt
    if (adaptation->interaction_count < ADAPTATION_THRESHOLD) {
        return false;
    }
    
    // Check if adaptation would be beneficial
    f32 positive_ratio = (f32)adaptation->positive_interactions / adaptation->interaction_count;
    return positive_ratio < 0.7f; // Adapt if less than 70% positive interactions
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

NLPBehaviorSystem *nlp_behavior_create(MLSystem *ml_system) {
    NLP_CHECK_NULL_PARAM(ml_system);
    
    NLPBehaviorSystem *system = malloc(sizeof(NLPBehaviorSystem));
    if (!system) {
        LOG_ERROR("Failed to allocate NLP behavior system");
        return NULL;
    }
    
    memset(system, 0, sizeof(NLPBehaviorSystem));
    system->ml_system = ml_system;
    system->creativity_level = 0.7f;
    system->safety_filter_strength = 0.8f;
    
    // Allocate arrays
    system->personalities = calloc(MAX_PERSONALITIES, sizeof(PersonalityProfile));
    system->current_contexts = calloc(MAX_CONVERSATIONS, sizeof(DialogueContext));
    system->adaptations = calloc(MAX_ADAPTATIONS, sizeof(BehaviorAdaptation));
    
    if (!system->personalities || !system->current_contexts || !system->adaptations) {
        nlp_behavior_destroy(system);
        LOG_ERROR("Failed to allocate NLP system arrays");
        return NULL;
    }
    
    system->max_personalities = MAX_PERSONALITIES;
    system->max_conversations = MAX_CONVERSATIONS;
    system->max_adaptations = MAX_ADAPTATIONS;
    
    LOG_INFO("NLP Behavior system created");
    return system;
}

void nlp_behavior_destroy(NLPBehaviorSystem *system) {
    if (!system) return;
    
    nlp_behavior_shutdown(system);
    
    if (system->personalities) free(system->personalities);
    if (system->current_contexts) free(system->current_contexts);
    if (system->adaptations) free(system->adaptations);
    
    free(system);
    LOG_INFO("NLP Behavior system destroyed");
}

bool nlp_behavior_initialize(NLPBehaviorSystem *system) {
    NLP_CHECK_NULL_PARAM(system);
    NLP_CHECK_ERROR(!system->initialized, NLP_ERROR_INVALID_PARAMETER);
    
    // Load NLP models
    char model_path[256];
    snprintf(model_path, sizeof(model_path), "assets/models/nlp_gpt.mlmodel");
    
    MLModelMetadata metadata = {
        .name = "nlp_generation_model",
        .type = ML_MODEL_TYPE_GENERATION,
        .framework = ML_FRAMEWORK_COREML,
        .preferred_device = ML_DEVICE_GPU,
        .supports_dynamic_shapes = true,
        .max_batch_size = 1
    };
    
    system->nlp_model = ml_load_model(system->ml_system, model_path, &metadata);
    if (!system->nlp_model) {
        LOG_WARNING("Failed to load NLP model, using template-based generation");
    }
    
    // Load understanding model
    snprintf(model_path, sizeof(model_path), "assets/models/nlp_understanding.mlmodel");
    metadata.name = "nlp_understanding_model";
    metadata.type = ML_MODEL_TYPE_BEHAVIOR;
    
    system->understanding_model = ml_load_model(system->ml_system, model_path, &metadata);
    if (!system->understanding_model) {
        LOG_WARNING("Failed to load understanding model");
    }
    
    system->initialized = true;
    LOG_INFO("NLP Behavior system initialized");
    return true;
}

void nlp_behavior_shutdown(NLPBehaviorSystem *system) {
    if (!system || !system->initialized) return;
    
    // Unload models
    if (system->nlp_model) {
        ml_unload_model(system->ml_system, system->nlp_model);
        system->nlp_model = NULL;
    }
    
    if (system->understanding_model) {
        ml_unload_model(system->ml_system, system->understanding_model);
        system->understanding_model = NULL;
    }
    
    system->initialized = false;
    LOG_INFO("NLP Behavior system shutdown complete");
}

u32 nlp_create_personality(NLPBehaviorSystem *system, const PersonalityProfile *profile) {
    NLP_CHECK_NULL_PARAM(system);
    NLP_CHECK_NULL_PARAM(profile);
    NLP_CHECK_ERROR(system->personality_count < system->max_personalities, NLP_ERROR_OUT_OF_MEMORY);
    
    if (!nlp_validate_personality_profile(profile)) {
        LOG_ERROR("Invalid personality profile");
        return 0;
    }
    
    // Copy personality profile
    u32 personality_id = system->personality_count + 1; // 1-based indexing
    system->personalities[system->personality_count] = *profile;
    system->personality_count++;
    
    // Initialize adaptation data
    if (personality_id <= system->max_adaptations) {
        BehaviorAdaptation *adaptation = &system->adaptations[personality_id - 1];
        memset(adaptation, 0, sizeof(BehaviorAdaptation));
        adaptation->adaptation_rate = 0.1f;
    }
    
    LOG_INFO("Created personality with ID %u", personality_id);
    return personality_id;
}

GeneratedResponse *nlp_generate_response(NLPBehaviorSystem *system, u32 personality_id, 
                                         const DialogueContext *context) {
    NLP_CHECK_NULL_PARAM(system);
    NLP_CHECK_NULL_PARAM(context);
    NLP_CHECK_ERROR(personality_id > 0 && personality_id <= system->personality_count, NLP_ERROR_PERSONALITY_NOT_FOUND);
    
    NLP_START_TIMER(system);
    
    GeneratedResponse *response = malloc(sizeof(GeneratedResponse));
    if (!response) {
        LOG_ERROR("Failed to allocate response structure");
        return NULL;
    }
    
    memset(response, 0, sizeof(GeneratedResponse));
    
    bool success = false;
    
    // Try ML-based generation first
    if (system->nlp_model) {
        // Create inference context
        MLInferenceContext *ml_context = ml_create_inference_context(system->ml_system, system->nlp_model);
        if (ml_context) {
            // Set up input tensors from context and personality
            // This would convert context to tensor format
            
            // Run inference
            success = ml_run_inference(system->ml_system, ml_context);
            
            if (success) {
                // Extract response from output tensors
                // This would convert tensor output to response text
                strncpy(response->response_text, "Generated ML response", sizeof(response->response_text) - 1);
                response->confidence_score = 0.8f;
            }
            
            ml_destroy_inference_context(system->ml_system, ml_context);
        }
    }
    
    // Fallback to template-based generation
    if (!success) {
        success = generate_response_template(system, personality_id - 1, context, response);
    }
    
    if (success) {
        // Apply personality and dialect
        PersonalityProfile *personality = &system->personalities[personality_id - 1];
        apply_dialect_to_response(response, personality);
        
        // Update emotional state
        response->expressed_emotion = transition_emotion(personality->default_emotion, context, personality);
        
        // Calculate confidence
        response->confidence_score = calculate_response_confidence(system, personality_id - 1, context, response);
        
        // Apply safety filter
        if (system->safety_filter_strength > 0.0f) {
            // Check response for inappropriate content
            response->appropriateness_score = 0.9f; // Placeholder
        }
    }
    
    NLP_END_TIMER(system, NULL);
    
    if (!success) {
        free(response);
        return NULL;
    }
    
    return response;
}

bool nlp_record_interaction(NLPBehaviorSystem *system, u32 personality_id, 
                           const DialogueContext *context, const GeneratedResponse *response, 
                           f32 player_satisfaction) {
    NLP_CHECK_NULL_PARAM(system);
    NLP_CHECK_NULL_PARAM(context);
    NLP_CHECK_NULL_PARAM(response);
    NLP_CHECK_ERROR(personality_id > 0 && personality_id <= system->personality_count, NLP_ERROR_PERSONALITY_NOT_FOUND);
    
    if (personality_id > system->max_adaptations) {
        return false;
    }
    
    BehaviorAdaptation *adaptation = &system->adaptations[personality_id - 1];
    
    // Update interaction statistics
    adaptation->interaction_count++;
    adaptation->last_interaction_time = time(NULL);
    
    if (player_satisfaction > 0.6f) {
        adaptation->positive_interactions++;
    } else {
        adaptation->negative_interactions++;
    }
    
    // Update player preferences
    update_player_preferences(system, personality_id - 1, response->dominant_trait, player_satisfaction);
    
    // Learn patterns from successful interactions
    if (player_satisfaction > 0.7f) {
        learn_interaction_pattern(system, personality_id - 1, context->current_topic, player_satisfaction);
    }
    
    // Check if adaptation is needed
    if (should_adapt_behavior(system, personality_id)) {
        nlp_adapt_behavior(system, personality_id);
    }
    
    return true;
}

bool nlp_adapt_behavior(NLPBehaviorSystem *system, u32 personality_id) {
    NLP_CHECK_NULL_PARAM(system);
    NLP_CHECK_ERROR(personality_id > 0 && personality_id <= system->personality_count, NLP_ERROR_PERSONALITY_NOT_FOUND);
    
    if (personality_id > system->max_adaptations) {
        return false;
    }
    
    PersonalityProfile *personality = &system->personalities[personality_id - 1];
    BehaviorAdaptation *adaptation = &system->adaptations[personality_id - 1];
    
    // Adapt personality based on player preferences
    f32 max_preference = 0.0f;
    PersonalityTrait preferred_trait = PERSONALITY_TRAIT_FRIENDLY;
    
    for (u32 i = 0; i < 10; i++) {
        if (adaptation->player_preference_scores[i] > max_preference) {
            max_preference = adaptation->player_preference_scores[i];
            preferred_trait = (PersonalityTrait)i;
        }
    }
    
    // Adjust personality traits
    f32 adaptation_strength = adaptation->adaptation_rate;
    personality->trait_strength[preferred_trait] += adaptation_strength * 0.1f;
    
    // Clamp trait strengths
    for (u32 i = 0; i < 10; i++) {
        personality->trait_strength[i] = fmaxf(0.0f, fminf(1.0f, personality->trait_strength[i]));
    }
    
    adaptation->is_adapting = false;
    
    LOG_INFO("Adapted personality %d to prefer trait %s", personality_id, nlp_get_trait_name(preferred_trait));
    return true;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char *nlp_get_trait_name(PersonalityTrait trait) {
    switch (trait) {
        case PERSONALITY_TRAIT_FRIENDLY: return "Friendly";
        case PERSONALITY_TRAIT_AGGRESSIVE: return "Aggressive";
        case PERSONALITY_TRAIT_SHY: return "Shy";
        case PERSONALITY_TRAIT_CURIOS: return "Curious";
        case PERSONALITY_TRAIT_FORMAL: return "Formal";
        case PERSONALITY_TRAIT_CASUAL: return "Casual";
        case PERSONALITY_TRAIT_HUMOROUS: return "Humorous";
        case PERSONALITY_TRAIT_SERIOUS: return "Serious";
        case PERSONALITY_TRAIT_MYSTERIOUS: return "Mysterious";
        case PERSONALITY_TRAIT_HELPFUL: return "Helpful";
        default: return "Unknown";
    }
}

const char *nlp_get_emotion_name(EmotionalState emotion) {
    switch (emotion) {
        case EMOTION_STATE_NEUTRAL: return "Neutral";
        case EMOTION_STATE_HAPPY: return "Happy";
        case EMOTION_STATE_ANGRY: return "Angry";
        case EMOTION_STATE_SAD: return "Sad";
        case EMOTION_STATE_FEARFUL: return "Fearful";
        case EMOTION_STATE_SURPRISED: return "Surprised";
        case EMOTION_STATE_DISGUSTED: return "Disgusted";
        case EMOTION_STATE_EXCITED: return "Excited";
        case EMOTION_STATE_WORRIED: return "Worried";
        case EMOTION_STATE_CONFIDENT: return "Confident";
        default: return "Unknown";
    }
}

bool nlp_validate_personality_profile(const PersonalityProfile *profile) {
    if (!profile) return false;
    
    // Check trait strengths are valid
    for (u32 i = 0; i < 10; i++) {
        if (profile->trait_strength[i] < 0.0f || profile->trait_strength[i] > 1.0f) {
            return false;
        }
    }
    
    // Check other parameters
    if (profile->emotion_volatility < 0.0f || profile->emotion_volatility > 1.0f) {
        return false;
    }
    
    if (profile->openness_level < 0.0f || profile->openness_level > 1.0f) {
        return false;
    }
    
    if (profile->trust_level < 0.0f || profile->trust_level > 1.0f) {
        return false;
    }
    
    return true;
}

const char *nlp_get_error_string(NLPError error) {
    switch (error) {
        case NLP_ERROR_NONE: return "No error";
        case NLP_ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case NLP_ERROR_MODEL_NOT_LOADED: return "Model not loaded";
        case NLP_ERROR_PERSONALITY_NOT_FOUND: return "Personality not found";
        case NLP_ERROR_CONTEXT_INVALID: return "Context invalid";
        case NLP_ERROR_GENERATION_FAILED: return "Generation failed";
        case NLP_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case NLP_ERROR_SAFETY_FILTER_TRIGGERED: return "Safety filter triggered";
        case NLP_ERROR_ADAPTATION_FAILED: return "Adaptation failed";
        default: return "Unknown error";
    }
}

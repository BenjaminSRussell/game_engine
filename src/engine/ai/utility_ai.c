/**
 * UTILITY AI (IAUS) - Infinite Axis Utility System
 * Consolidated implementation from npc_advanced/utility_ai.c
 * 
 * PURPOSE: Fuzzy-logic based decision making with scoring curves and inertia.
 *
 * FEATURES:
 * - Phase 1: Core structures and initialization
 * - Phase 2: Mathematical scoring curves (linear, logistic, sine, exponential, clamped)
 * - Phase 3: Action selection with inertia/hysteresis
 * - Phase 4: Behavior tree integration (see behavior_tree.c)
 */

#include <float.h>
#include <string.h>
#include <math.h>

// Forward declarations for structures (would normally be in header)
typedef struct UtilityAgent UtilityAgent;
typedef struct UtilityAction UtilityAction;
typedef struct UtilityScorer UtilityScorer;
typedef struct UtilityConsideration UtilityConsideration;
typedef struct UtilityContext UtilityContext;
typedef struct UtilityCurve UtilityCurve;

// Enums
typedef enum {
    CURVE_LINEAR,
    CURVE_LOGISTIC,
    CURVE_SINE,
    CURVE_EXPONENTIAL,
    CURVE_CLAMPED,
    CURVE_INVERSE
} UtilityCurveType;

// Constants
#define UTILITY_MAX_ACTIONS 32
#define UTILITY_MAX_CONSIDERATIONS 16
#define MEMORY_TAG_AI 0x41490000

// Structures (simplified for consolidation)
struct UtilityCurve {
    UtilityCurveType type;
    float slope;
    float exponent;
    float x_shift;
    float y_shift;
    float min_value;
    float max_value;
};

struct UtilityConsideration {
    char name[64];
    UtilityCurve curve;
    float weight;
    float (*evaluate_input)(const UtilityContext *context, void *user_data);
    void *user_data;
};

struct UtilityScorer {
    char name[64];
    UtilityConsideration considerations[UTILITY_MAX_CONSIDERATIONS];
    int consideration_count;
    bool use_multiplication;
};

struct UtilityAction {
    char name[64];
    UtilityScorer scorer;
    bool (*can_execute)(UtilityAgent *agent, const UtilityContext *context);
    void (*on_start)(UtilityAgent *agent, const UtilityContext *context);
    void (*on_update)(UtilityAgent *agent, const UtilityContext *context, float delta_time);
    void (*on_end)(UtilityAgent *agent, const UtilityContext *context);
    
    // Runtime state
    float current_score;
    float last_score;
    bool is_active;
    int execution_count;
    float execution_time;
    float total_execution_time;
};

struct UtilityAgent {
    u32 id;
    char name[64];
    UtilityAction *actions[UTILITY_MAX_ACTIONS];
    int action_count;
    
    // Current state
    UtilityAction *current_action;
    UtilityAction *previous_action;
    float action_switch_cooldown;
    
    // Inertia system
    struct {
        bool enabled;
        float inertia_bonus;
        float min_score_diff;
        float cooldown_time;
    } inertia;
    
    // Statistics
    u32 total_decisions;
    u32 action_switches;
    float total_decision_time;
    
    // Debug
    bool debug_mode;
    char last_decision_reason[256];
};

struct UtilityContext {
    void *agent_entity;
    void *world_state;
    float delta_time;
    float current_time;
    // Additional context data as needed
};

// Memory macros (simplified)
#define MALLOC_TAGGED(size, tag) malloc(size)
#define FREE(ptr) free(ptr)

// Logging macros (simplified)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)

// ===========================================================================================
// PHASE 2: UTILITY SCORING CURVES
// ===========================================================================================

// Linear curve: f(x) = slope * (x + x_shift) + y_shift
float utility_curve_linear(float input, float slope, float x_shift, float y_shift) {
    return slope * (input + x_shift) + y_shift;
}

// Logistic (sigmoid) curve: f(x) = 1 / (1 + e^(-slope * (x + x_shift))) + y_shift
float utility_curve_logistic(float input, float slope, float x_shift, float y_shift) {
    float x = slope * (input + x_shift);
    return 1.0f / (1.0f + expf(-x)) + y_shift;
}

// Sine curve: f(x) = sin(slope * (x + x_shift)) + y_shift
float utility_curve_sine(float input, float slope, float x_shift, float y_shift) {
    return sinf(slope * (input + x_shift)) + y_shift;
}

// Exponential curve: f(x) = (x + x_shift)^exponent + y_shift
float utility_curve_exponential(float input, float exponent, float x_shift, float y_shift) {
    return powf(input + x_shift, exponent) + y_shift;
}

// Clamped curve: Hard min/max boundaries
float utility_curve_clamped(float input, float min_val, float max_val) {
    if (input < min_val) return min_val;
    if (input > max_val) return max_val;
    return input;
}

// Inverse curve: f(x) = 1 / (slope * (x + x_shift)) + y_shift
float utility_curve_inverse(float input, float slope, float x_shift, float y_shift) {
    float denominator = slope * (input + x_shift);
    if (fabsf(denominator) < 0.001f) denominator = 0.001f;
    return 1.0f / denominator + y_shift;
}

// Unified curve evaluation
float utility_evaluate_curve(const UtilityCurve *curve, float input) {
    float result = 0.0f;
    
    switch (curve->type) {
    case CURVE_LINEAR:
        result = utility_curve_linear(input, curve->slope, curve->x_shift, curve->y_shift);
        break;
    case CURVE_LOGISTIC:
        result = utility_curve_logistic(input, curve->slope, curve->x_shift, curve->y_shift);
        break;
    case CURVE_SINE:
        result = utility_curve_sine(input, curve->slope, curve->x_shift, curve->y_shift);
        break;
    case CURVE_EXPONENTIAL:
        result = utility_curve_exponential(input, curve->exponent, curve->x_shift, curve->y_shift);
        break;
    case CURVE_CLAMPED:
        result = utility_curve_clamped(input, curve->min_value, curve->max_value);
        break;
    case CURVE_INVERSE:
        result = utility_curve_inverse(input, curve->slope, curve->x_shift, curve->y_shift);
        break;
    default:
        result = input;
        break;
    }
    
    return utility_curve_clamped(result, curve->min_value, curve->max_value);
}

// ===========================================================================================
// PHASE 1: CORE AGENT & ACTION MANAGEMENT
// ===========================================================================================

// Create a new utility agent
UtilityAgent *utility_agent_create(u32 id, const char *name) {
    UtilityAgent *agent = (UtilityAgent *)MALLOC_TAGGED(sizeof(UtilityAgent), MEMORY_TAG_AI);
    if (!agent) {
        LOG_ERROR("Failed to allocate memory for utility agent");
        return NULL;
    }
    
    memset(agent, 0, sizeof(UtilityAgent));
    agent->id = id;
    strncpy(agent->name, name, sizeof(agent->name) - 1);
    
    // Default inertia configuration
    agent->inertia.enabled = true;
    agent->inertia.inertia_bonus = 0.1f;
    agent->inertia.min_score_diff = 0.05f;
    agent->inertia.cooldown_time = 0.5f;
    
    return agent;
}

// Destroy utility agent
void utility_agent_destroy(UtilityAgent *agent) {
    if (!agent) return;
    FREE(agent);
}

// Create a new utility action
UtilityAction *utility_action_create(const char *name) {
    UtilityAction *action = (UtilityAction *)MALLOC_TAGGED(sizeof(UtilityAction), MEMORY_TAG_AI);
    if (!action) {
        LOG_ERROR("Failed to allocate memory for utility action");
        return NULL;
    }
    
    memset(action, 0, sizeof(UtilityAction));
    strncpy(action->name, name, sizeof(action->name) - 1);
    strncpy(action->scorer.name, name, sizeof(action->scorer.name) - 1);
    action->scorer.use_multiplication = true;
    
    return action;
}

// Destroy utility action
void utility_action_destroy(UtilityAction *action) {
    if (!action) return;
    FREE(action);
}

// Add action to agent
bool utility_agent_add_action(UtilityAgent *agent, UtilityAction *action) {
    if (!agent || !action) return false;
    
    if (agent->action_count >= UTILITY_MAX_ACTIONS) {
        LOG_ERROR("Agent %s has reached maximum action count (%d)", agent->name, UTILITY_MAX_ACTIONS);
        return false;
    }
    
    agent->actions[agent->action_count++] = action;
    return true;
}

// ===========================================================================================
// PHASE 3: ACTION SELECTION WITH INERTIA
// ===========================================================================================

// Evaluate a single consideration
float utility_evaluate_consideration(const UtilityConsideration *consideration, const UtilityContext *context) {
    if (!consideration || !consideration->evaluate_input) return 0.0f;
    
    float raw_input = consideration->evaluate_input(context, consideration->user_data);
    raw_input = utility_curve_clamped(raw_input, 0.0f, 1.0f);
    float curve_output = utility_evaluate_curve(&consideration->curve, raw_input);
    return curve_output * consideration->weight;
}

// Evaluate a scorer (combines all considerations)
float utility_evaluate_scorer(const UtilityScorer *scorer, const UtilityContext *context) {
    if (!scorer || scorer->consideration_count == 0) return 0.0f;
    
    if (scorer->use_multiplication) {
        float score = 1.0f;
        for (int i = 0; i < scorer->consideration_count; i++) {
            float consideration_score = utility_evaluate_consideration(&scorer->considerations[i], context);
            score *= consideration_score;
        }
        return score;
    } else {
        float sum = 0.0f;
        for (int i = 0; i < scorer->consideration_count; i++) {
            sum += utility_evaluate_consideration(&scorer->considerations[i], context);
        }
        return sum / (float)scorer->consideration_count;
    }
}

// Evaluate an action's utility score
float utility_evaluate_action(const UtilityAction *action, const UtilityContext *context) {
    if (!action) return 0.0f;
    
    if (action->can_execute && !action->can_execute((UtilityAgent *)context->agent_entity, context)) {
        return 0.0f;
    }
    
    return utility_evaluate_scorer(&action->scorer, context);
}

// CORE FUNCTION: Select best action with inertia
UtilityAction *utility_select_best_action(UtilityAgent *agent, const UtilityContext *context) {
    if (!agent || !context || agent->action_count == 0) return NULL;
    
    agent->total_decisions++;
    
    // Update cooldown
    if (agent->action_switch_cooldown > 0.0f) {
        agent->action_switch_cooldown -= context->delta_time;
        if (agent->action_switch_cooldown < 0.0f) {
            agent->action_switch_cooldown = 0.0f;
        }
    }
    
    // Evaluate all actions
    float best_score = -FLT_MAX;
    UtilityAction *best_action = NULL;
    
    for (int i = 0; i < agent->action_count; i++) {
        UtilityAction *action = agent->actions[i];
        float score = utility_evaluate_action(action, context);
        
        // Apply inertia bonus to current action
        if (agent->inertia.enabled && action == agent->current_action) {
            score *= (1.0f + agent->inertia.inertia_bonus);
        }
        
        action->last_score = action->current_score;
        action->current_score = score;
        
        if (score > best_score) {
            best_score = score;
            best_action = action;
        }
    }
    
    // Check if we should switch actions
    bool should_switch = false;
    
    if (best_action != agent->current_action) {
        if (agent->current_action) {
            float current_score = agent->current_action->current_score;
            float score_diff = best_score - current_score;
            
            if (score_diff >= agent->inertia.min_score_diff) {
                should_switch = true;
            }
        } else {
            should_switch = true;
        }
        
        if (should_switch && agent->action_switch_cooldown > 0.0f) {
            should_switch = false;
        }
    }
    
    // Perform action switch if needed
    if (should_switch && best_action != agent->current_action) {
        if (agent->current_action && agent->current_action->on_end) {
            agent->current_action->on_end(agent, context);
            agent->current_action->is_active = false;
        }
        
        agent->previous_action = agent->current_action;
        agent->current_action = best_action;
        agent->action_switches++;
        agent->action_switch_cooldown = agent->inertia.cooldown_time;
        
        if (agent->current_action && agent->current_action->on_start) {
            agent->current_action->on_start(agent, context);
            agent->current_action->is_active = true;
            agent->current_action->execution_time = 0.0f;
        }
    }
    
    return agent->current_action;
}

// Update agent (tick current action)
void utility_update_agent(UtilityAgent *agent, const UtilityContext *context, float delta_time) {
    if (!agent || !context) return;
    
    utility_select_best_action(agent, context);
    
    if (agent->current_action && agent->current_action->on_update) {
        agent->current_action->on_update(agent, context, delta_time);
        agent->current_action->execution_time += delta_time;
        agent->current_action->total_execution_time += delta_time;
    }
}

// ===========================================================================================
// LEGACY COMPATIBILITY FUNCTIONS (from utility_ai_advanced_impl.c)
// ===========================================================================================

typedef struct {
    float (*score_func)(void *ctx);
    float weight;
    float curve_power;
} LegacyConsideration;

typedef struct {
    char name[32];
    LegacyConsideration *considerations;
    int count;
    void (*execute)(void *ctx);
} LegacyAction;

typedef struct {
    LegacyAction *actions;
    int count;
} LegacyUtilityBrain;

// Evaluate Action (legacy compatibility)
float ai_utility_score_action(LegacyAction *action, void *context) {
    float score = 1.0f;
    for (int i = 0; i < action->count; i++) {
        float val = action->considerations[i].score_func(context);
        val = powf(val, action->considerations[i].curve_power);
        score *= val;
    }
    return score + ((float)rand() / RAND_MAX) * 0.01f;
}

// Select Best (legacy compatibility)
LegacyAction *ai_utility_select(LegacyUtilityBrain *brain, void *context) {
    float best_score = -1.0f;
    LegacyAction *best_action = NULL;
    
    for (int i = 0; i < brain->count; i++) {
        float s = ai_utility_score_action(&brain->actions[i], context);
        if (s > best_score) {
            best_score = s;
            best_action = &brain->actions[i];
        }
    }
    return best_action;
}

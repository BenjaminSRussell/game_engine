#ifndef UTILITY_AI_H
#define UTILITY_AI_H

#include <core/types.h>
#include <math/vec3.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Utility AI System - Fuzzy-logic based decision making
// Enables NPCs to make context-aware choices based on multiple weighted factors

// Forward declarations
typedef struct UtilityAgent UtilityAgent;
typedef struct UtilityAction UtilityAction;
typedef struct UtilityScorer UtilityScorer;
typedef struct UtilityContext UtilityContext;

#define UTILITY_MAX_ACTIONS 32
#define UTILITY_MAX_SCORERS 16
#define UTILITY_MAX_CONSIDERATIONS 8

// Curve types for scoring functions
typedef enum {
  CURVE_LINEAR,      // Simple linear mapping
  CURVE_LOGISTIC,    // S-curve (sigmoid) - smooth transitions
  CURVE_SINE,        // Sine wave - periodic preferences
  CURVE_EXPONENTIAL, // Exponential growth/decay
  CURVE_CLAMPED,     // Hard min/max boundaries
  CURVE_INVERSE,     // Inverse relationship
  CURVE_COUNT
} UtilityCurveType;

// Curve evaluation parameters
typedef struct {
  UtilityCurveType type;
  float slope;     // Steepness of curve
  float exponent;  // Power for exponential curves
  float x_shift;   // Horizontal offset
  float y_shift;   // Vertical offset
  float min_value; // Minimum output value
  float max_value; // Maximum output value
} UtilityCurve;

// Input consideration for scoring
// Example: "Distance to target", "Ammo remaining", "Health percentage"
typedef struct {
  char name[64];
  UtilityCurve curve;
  float weight; // Importance multiplier

  // Function to evaluate the raw input value [0, 1]
  float (*evaluate_input)(const UtilityContext *context, void *user_data);
  void *user_data;
} UtilityConsideration;

// Scorer combines multiple considerations into a single score
struct UtilityScorer {
  char name[64];
  UtilityConsideration considerations[UTILITY_MAX_CONSIDERATIONS];
  int consideration_count;

  // Aggregation method (multiply vs average)
  bool use_multiplication; // true = multiply, false = average
};

// Action that can be selected by the utility system
struct UtilityAction {
  char name[64];
  UtilityScorer scorer;

  // Execution callbacks
  bool (*can_execute)(UtilityAgent *agent, const UtilityContext *context);
  void (*on_start)(UtilityAgent *agent, const UtilityContext *context);
  void (*on_update)(UtilityAgent *agent, const UtilityContext *context,
                    float delta_time);
  void (*on_end)(UtilityAgent *agent, const UtilityContext *context);

  // Runtime state
  float last_score;
  float current_score;
  bool is_active;
  float execution_time;

  // Debug info
  int execution_count;
  float total_execution_time;

  // User data
  void *action_data;
};

// Context provides world state for utility evaluation
struct UtilityContext {
  // Agent making the decision
  void *agent_entity;

  // World state
  float delta_time;
  u32 frame_number;

  // Common query results (cached for performance)
  struct {
    void *nearest_enemy;
    float distance_to_enemy;
    void *nearest_cover;
    float distance_to_cover;
    void *nearest_health_pack;
    float distance_to_health_pack;
  } cached_queries;

  // User-extensible data
  void *blackboard;
  void *user_data;
};

// Inertia configuration to prevent action thrashing
typedef struct {
  bool enabled;
  float inertia_bonus; // Percentage bonus to current action (default 0.1 = 10%)
  float min_score_diff; // Minimum score difference to switch actions
  float cooldown_time;  // Cooldown after switching actions
} UtilityInertiaConfig;

// The Utility Agent
struct UtilityAgent {
  u32 id;
  char name[64];

  // Available actions
  UtilityAction *actions[UTILITY_MAX_ACTIONS];
  int action_count;

  // Current state
  UtilityAction *current_action;
  UtilityAction *previous_action;
  float action_switch_cooldown;

  // Configuration
  UtilityInertiaConfig inertia;

  // Statistics
  u32 total_decisions;
  u32 action_switches;
  float total_decision_time;

  // Debug
  bool debug_mode;
  char last_decision_reason[256];

  // User data
  void *user_data;
};

// Core utility AI functions

// Agent management
UtilityAgent *utility_agent_create(u32 id, const char *name);
void utility_agent_destroy(UtilityAgent *agent);
void utility_agent_reset(UtilityAgent *agent);

// Action management
UtilityAction *utility_action_create(const char *name);
void utility_action_destroy(UtilityAction *action);
bool utility_agent_add_action(UtilityAgent *agent, UtilityAction *action);
bool utility_agent_remove_action(UtilityAgent *agent, UtilityAction *action);

// Scorer configuration
bool utility_scorer_add_consideration(
    UtilityScorer *scorer, const UtilityConsideration *consideration);
void utility_scorer_set_aggregation(UtilityScorer *scorer,
                                    bool use_multiplication);

// Curve evaluation
float utility_evaluate_curve(const UtilityCurve *curve, float input);
float utility_curve_linear(float input, float slope, float x_shift,
                           float y_shift);
float utility_curve_logistic(float input, float slope, float x_shift,
                             float y_shift);
float utility_curve_sine(float input, float slope, float x_shift,
                         float y_shift);
float utility_curve_exponential(float input, float exponent, float x_shift,
                                float y_shift);
float utility_curve_clamped(float input, float min_val, float max_val);
float utility_curve_inverse(float input, float slope, float x_shift,
                            float y_shift);

// Decision making (CORE FUNCTION)
UtilityAction *utility_select_best_action(UtilityAgent *agent,
                                          const UtilityContext *context);
void utility_update_agent(UtilityAgent *agent, const UtilityContext *context,
                          float delta_time);

// Score evaluation
float utility_evaluate_action(const UtilityAction *action,
                              const UtilityContext *context);
float utility_evaluate_scorer(const UtilityScorer *scorer,
                              const UtilityContext *context);
float utility_evaluate_consideration(const UtilityConsideration *consideration,
                                     const UtilityContext *context);

// Inertia configuration
void utility_set_inertia(UtilityAgent *agent, bool enabled, float bonus,
                         float min_diff);
void utility_set_cooldown(UtilityAgent *agent, float cooldown_time);

// Context management
UtilityContext *utility_context_create(void);
void utility_context_destroy(UtilityContext *context);
void utility_context_update_cache(UtilityContext *context);

// Debug and profiling
void utility_debug_print_scores(const UtilityAgent *agent,
                                const UtilityContext *context);
void utility_debug_print_action(const UtilityAction *action,
                                const UtilityContext *context);
const char *utility_get_curve_name(UtilityCurveType type);
void utility_print_agent_stats(const UtilityAgent *agent);

// Helper functions for common considerations
float utility_consideration_distance(const UtilityContext *context,
                                     void *user_data);
float utility_consideration_health(const UtilityContext *context,
                                   void *user_data);
float utility_consideration_ammo(const UtilityContext *context,
                                 void *user_data);
float utility_consideration_threat_level(const UtilityContext *context,
                                         void *user_data);

#ifdef __cplusplus
}
#endif

#endif // UTILITY_AI_H

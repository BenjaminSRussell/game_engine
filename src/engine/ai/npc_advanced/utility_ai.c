#include "ai/npc_advanced/utility_ai.h"
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <float.h>
#include <include/math/math_all.h>
#include <string.h>

/**
 * =================================================================================================
 *                          UTILITY AI SYSTEM - AGENT_AI_1 (Phase 1-4)
 * =================================================================================================
 *
 * PURPOSE: Fuzzy-logic based decision making with scoring curves and inertia.
 *
 * FEATURES:
 * - Phase 1: Core structures and initialization
 * - Phase 2: Mathematical scoring curves (linear, logistic, sine, exponential,
 * clamped)
 * - Phase 3: Action selection with inertia/hysteresis
 * - Phase 4: Behavior tree integration (see behavior_tree.c)
 * =================================================================================================
 */

// ===========================================================================================
// PHASE 2: UTILITY SCORING CURVES
// ===========================================================================================

// Linear curve: f(x) = slope * (x + x_shift) + y_shift
float utility_curve_linear(float input, float slope, float x_shift,
                           float y_shift) {
  return slope * (input + x_shift) + y_shift;
}

// Logistic (sigmoid) curve: f(x) = 1 / (1 + e^(-slope * (x + x_shift))) +
// y_shift Creates an S-shaped curve for smooth transitions
float utility_curve_logistic(float input, float slope, float x_shift,
                             float y_shift) {
  float x = slope * (input + x_shift);
  return 1.0f / (1.0f + expf(-x)) + y_shift;
}

// Sine curve: f(x) = sin(slope * (x + x_shift)) + y_shift
// Useful for periodic preferences
float utility_curve_sine(float input, float slope, float x_shift,
                         float y_shift) {
  return sinf(slope * (input + x_shift)) + y_shift;
}

// Exponential curve: f(x) = (x + x_shift)^exponent + y_shift
// Good for rapidly increasing/decreasing preferences
float utility_curve_exponential(float input, float exponent, float x_shift,
                                float y_shift) {
  return powf(input + x_shift, exponent) + y_shift;
}

// Clamped curve: Hard min/max boundaries
float utility_curve_clamped(float input, float min_val, float max_val) {
  if (input < min_val)
    return min_val;
  if (input > max_val)
    return max_val;
  return input;
}

// Inverse curve: f(x) = 1 / (slope * (x + x_shift)) + y_shift
// Good for "closer is better" scenarios
float utility_curve_inverse(float input, float slope, float x_shift,
                            float y_shift) {
  float denominator = slope * (input + x_shift);
  if (fabsf(denominator) < 0.001f)
    denominator = 0.001f; // Prevent division by zero
  return 1.0f / denominator + y_shift;
}

// Unified curve evaluation
float utility_evaluate_curve(const UtilityCurve *curve, float input) {
  float result = 0.0f;

  switch (curve->type) {
  case CURVE_LINEAR:
    result = utility_curve_linear(input, curve->slope, curve->x_shift,
                                  curve->y_shift);
    break;
  case CURVE_LOGISTIC:
    result = utility_curve_logistic(input, curve->slope, curve->x_shift,
                                    curve->y_shift);
    break;
  case CURVE_SINE:
    result =
        utility_curve_sine(input, curve->slope, curve->x_shift, curve->y_shift);
    break;
  case CURVE_EXPONENTIAL:
    result = utility_curve_exponential(input, curve->exponent, curve->x_shift,
                                       curve->y_shift);
    break;
  case CURVE_CLAMPED:
    result = utility_curve_clamped(input, curve->min_value, curve->max_value);
    break;
  case CURVE_INVERSE:
    result = utility_curve_inverse(input, curve->slope, curve->x_shift,
                                   curve->y_shift);
    break;
  default:
    result = input; // Passthrough
    break;
  }

  // Apply global min/max clamping
  return utility_curve_clamped(result, curve->min_value, curve->max_value);
}

// ===========================================================================================
// PHASE 1: CORE AGENT & ACTION MANAGEMENT
// ===========================================================================================

// Create a new utility agent
UtilityAgent *utility_agent_create(u32 id, const char *name) {
  UtilityAgent *agent =
      (UtilityAgent *)MALLOC_TAGGED(sizeof(UtilityAgent), MEMORY_TAG_AI);
  if (!agent) {
    LOG_ERROR("Failed to allocate memory for utility agent");
    return NULL;
  }

  memset(agent, 0, sizeof(UtilityAgent));
  agent->id = id;
  strncpy(agent->name, name, sizeof(agent->name) - 1);

  // Default inertia configuration (prevents action thrashing)
  agent->inertia.enabled = true;
  agent->inertia.inertia_bonus = 0.1f;   // 10% bonus to current action
  agent->inertia.min_score_diff = 0.05f; // Must be 5% better to switch
  agent->inertia.cooldown_time = 0.5f;   // 0.5 second cooldown after switching

  agent->current_action = NULL;
  agent->previous_action = NULL;
  agent->action_switch_cooldown = 0.0f;

  return agent;
}

// Destroy utility agent
void utility_agent_destroy(UtilityAgent *agent) {
  if (!agent)
    return;

  // Note: We don't destroy actions here, as they may be shared between agents
  // The user is responsible for destroying actions separately

  FREE(0);
}

// Reset agent state
void utility_agent_reset(UtilityAgent *agent) {
  if (!agent)
    return;

  agent->current_action = NULL;
  agent->previous_action = NULL;
  agent->action_switch_cooldown = 0.0f;
  agent->total_decisions = 0;
  agent->action_switches = 0;
  agent->total_decision_time = 0.0f;
}

// Create a new utility action
UtilityAction *utility_action_create(const char *name) {
  UtilityAction *action =
      (UtilityAction *)MALLOC_TAGGED(sizeof(UtilityAction), MEMORY_TAG_AI);
  if (!action) {
    LOG_ERROR("Failed to allocate memory for utility action");
    return NULL;
  }

  memset(action, 0, sizeof(UtilityAction));
  strncpy(action->name, name, sizeof(action->name) - 1);
  strncpy(action->scorer.name, name, sizeof(action->scorer.name) - 1);
  action->scorer.use_multiplication = true; // Default to multiplication

  return action;
}

// Destroy utility action
void utility_action_destroy(UtilityAction *action) {
  if (!action)
    return;
  FREE(0);
}

// Add action to agent
bool utility_agent_add_action(UtilityAgent *agent, UtilityAction *action) {
  if (!agent || !action)
    return false;

  if (agent->action_count >= UTILITY_MAX_ACTIONS) {
    LOG_ERROR("Agent %s has reached maximum action count (%d)", agent->name,
              UTILITY_MAX_ACTIONS);
    return false;
  }

  agent->actions[agent->action_count++] = action;
  return true;
}

// Remove action from agent
bool utility_agent_remove_action(UtilityAgent *agent, UtilityAction *action) {
  if (!agent || !action)
    return false;

  for (int i = 0; i < agent->action_count; i++) {
    if (agent->actions[i] == action) {
      // Shift remaining actions
      for (int j = i; j < agent->action_count - 1; j++) {
        agent->actions[j] = agent->actions[j + 1];
      }
      agent->action_count--;

      // Clear current action if it was removed
      if (agent->current_action == action) {
        agent->current_action = NULL;
      }

      return true;
    }
  }

  return false;
}

// Add consideration to scorer
bool utility_scorer_add_consideration(
    UtilityScorer *scorer, const UtilityConsideration *consideration) {
  if (!scorer || !consideration)
    return false;

  if (scorer->consideration_count >= UTILITY_MAX_CONSIDERATIONS) {
    LOG_ERROR("Scorer %s has reached maximum consideration count (%d)",
              scorer->name, UTILITY_MAX_CONSIDERATIONS);
    return false;
  }

  scorer->considerations[scorer->consideration_count++] = *consideration;
  return true;
}

// Set scorer aggregation method
void utility_scorer_set_aggregation(UtilityScorer *scorer,
                                    bool use_multiplication) {
  if (scorer) {
    scorer->use_multiplication = use_multiplication;
  }
}

// ===========================================================================================
// PHASE 3: ACTION SELECTION WITH INERTIA
// ===========================================================================================

// Evaluate a single consideration
float utility_evaluate_consideration(const UtilityConsideration *consideration,
                                     const UtilityContext *context) {
  if (!consideration || !consideration->evaluate_input) {
    return 0.0f;
  }

  // Get raw input value [0, 1]
  float raw_input =
      consideration->evaluate_input(context, consideration->user_data);

  // Clamp to [0, 1]
  raw_input = utility_curve_clamped(raw_input, 0.0f, 1.0f);

  // Apply curve transformation
  float curve_output = utility_evaluate_curve(&consideration->curve, raw_input);

  // Apply weight
  return curve_output * consideration->weight;
}

// Evaluate a scorer (combines all considerations)
float utility_evaluate_scorer(const UtilityScorer *scorer,
                              const UtilityContext *context) {
  if (!scorer || scorer->consideration_count == 0) {
    return 0.0f;
  }

  if (scorer->use_multiplication) {
    // Multiplicative aggregation (all factors matter)
    float score = 1.0f;
    for (int i = 0; i < scorer->consideration_count; i++) {
      float consideration_score =
          utility_evaluate_consideration(&scorer->considerations[i], context);
      score *= consideration_score;
    }
    return score;
  } else {
    // Averaging aggregation (compensatory)
    float sum = 0.0f;
    for (int i = 0; i < scorer->consideration_count; i++) {
      sum +=
          utility_evaluate_consideration(&scorer->considerations[i], context);
    }
    return sum / (float)scorer->consideration_count;
  }
}

// Evaluate an action's utility score
float utility_evaluate_action(const UtilityAction *action,
                              const UtilityContext *context) {
  if (!action)
    return 0.0f;

  // Check if action can execute
  if (action->can_execute &&
      !action->can_execute((UtilityAgent *)context->agent_entity, context)) {
    return 0.0f; // Can't execute, score is 0
  }

  // Evaluate scorer
  return utility_evaluate_scorer(&action->scorer, context);
}

// CORE FUNCTION: Select best action with inertia
UtilityAction *utility_select_best_action(UtilityAgent *agent,
                                          const UtilityContext *context) {
  if (!agent || !context || agent->action_count == 0) {
    return NULL;
  }

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

      if (agent->debug_mode) {
        LOG_DEBUG("Inertia bonus applied to %s: %.3f -> %.3f", action->name,
                  score / (1.0f + agent->inertia.inertia_bonus), score);
      }
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
    // Check minimum score difference threshold
    if (agent->current_action) {
      float current_score = agent->current_action->current_score;
      float score_diff = best_score - current_score;

      if (score_diff >= agent->inertia.min_score_diff) {
        should_switch = true;
      }

      if (agent->debug_mode) {
        LOG_DEBUG("Score diff: %.3f (threshold: %.3f) - %s", score_diff,
                  agent->inertia.min_score_diff,
                  should_switch ? "SWITCHING" : "STAYING");
      }
    } else {
      should_switch = true; // No current action, always switch
    }

    // Check cooldown
    if (should_switch && agent->action_switch_cooldown > 0.0f) {
      should_switch = false;
      if (agent->debug_mode) {
        LOG_DEBUG("Switch prevented by cooldown (%.2fs remaining)",
                  agent->action_switch_cooldown);
      }
    }
  }

  // Perform action switch if needed
  if (should_switch && best_action != agent->current_action) {
    // Call on_end for previous action
    if (agent->current_action && agent->current_action->on_end) {
      agent->current_action->on_end(agent, context);
      agent->current_action->is_active = false;
    }

    agent->previous_action = agent->current_action;
    agent->current_action = best_action;
    agent->action_switches++;
    agent->action_switch_cooldown = agent->inertia.cooldown_time;

    // Call on_start for new action
    if (agent->current_action && agent->current_action->on_start) {
      agent->current_action->on_start(agent, context);
      agent->current_action->is_active = true;
      agent->current_action->execution_time = 0.0f;
    }

    if (agent->debug_mode) {
      snprintf(agent->last_decision_reason, sizeof(agent->last_decision_reason),
               "Switched from '%s' (%.3f) to '%s' (%.3f)",
               agent->previous_action ? agent->previous_action->name : "None",
               agent->previous_action ? agent->previous_action->current_score
                                      : 0.0f,
               agent->current_action->name, best_score);
      LOG_DEBUG("%s", agent->last_decision_reason);
    }
  }

  return agent->current_action;
}

// Update agent (tick current action)
void utility_update_agent(UtilityAgent *agent, const UtilityContext *context,
                          float delta_time) {
  if (!agent || !context)
    return;

  // Select best action
  utility_select_best_action(agent, context);

  // Update current action
  if (agent->current_action && agent->current_action->on_update) {
    agent->current_action->on_update(agent, context, delta_time);
    agent->current_action->execution_time += delta_time;
    agent->current_action->total_execution_time += delta_time;
  }
}

// ===========================================================================================
// INERTIA CONFIGURATION
// ===========================================================================================

void utility_set_inertia(UtilityAgent *agent, bool enabled, float bonus,
                         float min_diff) {
  if (!agent)
    return;
  agent->inertia.enabled = enabled;
  agent->inertia.inertia_bonus = bonus;
  agent->inertia.min_score_diff = min_diff;
}

void utility_set_cooldown(UtilityAgent *agent, float cooldown_time) {
  if (!agent)
    return;
  agent->inertia.cooldown_time = cooldown_time;
}

// ===========================================================================================
// CONTEXT MANAGEMENT
// ===========================================================================================

UtilityContext *utility_context_create(void) {
  UtilityContext *context =
      (UtilityContext *)MALLOC_TAGGED(sizeof(UtilityContext), MEMORY_TAG_AI);
  if (!context) {
    LOG_ERROR("Failed to allocate memory for utility context");
    return NULL;
  }

  memset(context, 0, sizeof(UtilityContext));
  return context;
}

void utility_context_destroy(UtilityContext *context) {
  if (!context)
    return;
  FREE(context);
}

void utility_context_update_cache(UtilityContext *context) {
  // User should implement caching logic based on game-specific queries
  // This is a placeholder for performance optimization
}

// ===========================================================================================
// DEBUG & PROFILING
// ===========================================================================================

const char *utility_get_curve_name(UtilityCurveType type) {
  switch (type) {
  case CURVE_LINEAR:
    return "Linear";
  case CURVE_LOGISTIC:
    return "Logistic";
  case CURVE_SINE:
    return "Sine";
  case CURVE_EXPONENTIAL:
    return "Exponential";
  case CURVE_CLAMPED:
    return "Clamped";
  case CURVE_INVERSE:
    return "Inverse";
  default:
    return "Unknown";
  }
}

void utility_debug_print_action(const UtilityAction *action,
                                const UtilityContext *context) {
  if (!action)
    return;

  LOG_INFO("Action: %s", action->name);
  LOG_INFO("  Score: %.3f (Last: %.3f)", action->current_score,
           action->last_score);
  LOG_INFO("  Active: %s", action->is_active ? "Yes" : "No");
  LOG_INFO("  Executions: %d", action->execution_count);
  LOG_INFO("  Total Time: %.2fs", action->total_execution_time);

  LOG_INFO("  Considerations:");
  for (int i = 0; i < action->scorer.consideration_count; i++) {
    const UtilityConsideration *c = &action->scorer.considerations[i];
    float score = utility_evaluate_consideration(c, context);
    LOG_INFO("    - %s: %.3f (weight: %.2f, curve: %s)", c->name, score,
             c->weight, utility_get_curve_name(c->curve.type));
  }
}

void utility_debug_print_scores(const UtilityAgent *agent,
                                const UtilityContext *context) {
  if (!agent)
    return;

  LOG_INFO("=== Utility Agent: %s ===", agent->name);
  LOG_INFO("Total Decisions: %u", agent->total_decisions);
  LOG_INFO("Action Switches: %u", agent->action_switches);
  LOG_INFO("Current Action: %s",
           agent->current_action ? agent->current_action->name : "None");

  LOG_INFO("\nAction Scores:");
  for (int i = 0; i < agent->action_count; i++) {
    const UtilityAction *action = agent->actions[i];
    LOG_INFO("  %s%s: %.3f", action == agent->current_action ? "[ACTIVE] " : "",
             action->name, action->current_score);
  }
}

void utility_print_agent_stats(const UtilityAgent *agent) {
  if (!agent)
    return;

  LOG_INFO("=== Utility Agent Stats: %s ===", agent->name);
  LOG_INFO("ID: %u", agent->id);
  LOG_INFO("Total Decisions: %u", agent->total_decisions);
  LOG_INFO("Action Switches: %u", agent->action_switches);
  LOG_INFO("Average Decision Time: %.3f ms",
           agent->total_decisions > 0
               ? (agent->total_decision_time * 1000.0f) / agent->total_decisions
               : 0.0f);

  float switch_rate =
      agent->total_decisions > 0
          ? (float)agent->action_switches / (float)agent->total_decisions
          : 0.0f;
  LOG_INFO("Switch Rate: %.1f%%", switch_rate * 100.0f);

  LOG_INFO("\nInertia Config:");
  LOG_INFO("  Enabled: %s", agent->inertia.enabled ? "Yes" : "No");
  LOG_INFO("  Bonus: %.1f%%", agent->inertia.inertia_bonus * 100.0f);
  LOG_INFO("  Min Score Diff: %.3f", agent->inertia.min_score_diff);
  LOG_INFO("  Cooldown: %.2fs", agent->inertia.cooldown_time);
}

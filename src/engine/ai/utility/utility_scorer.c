/**
 * =================================================================================================
 *                          UTILITY AI SCORER
 * =================================================================================================
 */

#include "utility_scorer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void utility_scorer_init(UtilityScorer *scorer) {
    scorer->actions = NULL;
    scorer->action_count = 0;
    scorer->selected_action = -1;
}

void utility_scorer_destroy(UtilityScorer *scorer) {
    for (int i = 0; i < scorer->action_count; i++) {
        free(scorer->actions[i].inputs);
    }
    free(scorer->actions);
}

void utility_add_action(UtilityScorer *scorer, const char *name) {
    scorer->action_count++;
    scorer->actions = realloc(scorer->actions, sizeof(UtilityAction) * scorer->action_count);
    
    UtilityAction *action = &scorer->actions[scorer->action_count - 1];
    action->action_name = name;
    action->inputs = NULL;
    action->input_count = 0;
    action->final_score = 0.0f;
}

void utility_add_input(UtilityScorer *scorer, int action_idx, const char *input_name, UtilityCurve curve) {
    if (action_idx < 0 || action_idx >= scorer->action_count) return;
    
    UtilityAction *action = &scorer->actions[action_idx];
    action->input_count++;
    action->inputs = realloc(action->inputs, sizeof(UtilityInput) * action->input_count);
    
    UtilityInput *input = &action->inputs[action->input_count - 1];
    input->name = input_name;
    input->curve = curve;
    input->min_value = 0.0f;
    input->max_value = 1.0f;
    input->exponent = 1.0f;
}

float utility_evaluate_curve(UtilityCurve curve, float value, float min_val, float max_val, float exponent) {
    // Normalize value to 0-1
    float normalized = (value - min_val) / (max_val - min_val);
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    
    float result = 0.0f;
    
    switch (curve) {
        case CURVE_LINEAR:
            result = normalized;
            break;
            
        case CURVE_EXPONENTIAL:
            result = powf(normalized, exponent);
            break;
            
        case CURVE_LOGISTIC:
            // S-curve
            result = 1.0f / (1.0f + expf(-10.0f * (normalized - 0.5f)));
            break;
            
        case CURVE_INVERSE_LINEAR:
            result = 1.0f - normalized;
            break;
    }
    
    return result;
}

void utility_score_all(UtilityScorer *scorer) {
    for (int i = 0; i < scorer->action_count; i++) {
        UtilityAction *action = &scorer->actions[i];
        
        // Multiply all input scores (geometric mean approach)
        float score = 1.0f;
        for (int j = 0; j < action->input_count; j++) {
            UtilityInput *input = &action->inputs[j];
            float input_score = utility_evaluate_curve(input->curve, input->min_value, 
                                                        input->min_value, input->max_value, input->exponent);
            score *= input_score;
        }
        
        action->final_score = score;
    }
}

int utility_select_best(UtilityScorer *scorer) {
    float best_score = -1.0f;
    int best_idx = -1;
    
    for (int i = 0; i < scorer->action_count; i++) {
        if (scorer->actions[i].final_score > best_score) {
            best_score = scorer->actions[i].final_score;
            best_idx = i;
        }
    }
    
    scorer->selected_action = best_idx;
    return best_idx;
}

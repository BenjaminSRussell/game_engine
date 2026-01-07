#pragma once

typedef enum {
    CURVE_LINEAR,
    CURVE_EXPONENTIAL,
    CURVE_LOGISTIC,
    CURVE_INVERSE_LINEAR
} UtilityCurve;

typedef struct {
    const char *name;
    UtilityCurve curve;
    float min_value;
    float max_value;
    float exponent; // For exponential curves
} UtilityInput;

typedef struct {
    const char *action_name;
    UtilityInput *inputs;
    int input_count;
    float final_score;
} UtilityAction;

typedef struct {
    UtilityAction *actions;
    int action_count;
    int selected_action;
} UtilityScorer;

void utility_scorer_init(UtilityScorer *scorer);
void utility_scorer_destroy(UtilityScorer *scorer);

void utility_add_action(UtilityScorer *scorer, const char *name);
void utility_add_input(UtilityScorer *scorer, int action_idx, const char *input_name, UtilityCurve curve);

float utility_evaluate_curve(UtilityCurve curve, float value, float min_val, float max_val, float exponent);
void utility_score_all(UtilityScorer *scorer);
int utility_select_best(UtilityScorer *scorer);

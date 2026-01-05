#include "core/core.h"
#include <math.h>

#define MAX_SETS 8

typedef enum { FUZZY_TRIANGLE, FUZZY_TRAPEZOID } FuzzySetType;

typedef struct {
    FuzzySetType type;
    float a, b, c, d; // Triangle uses a,b,c; Trapezoid uses all four
} FuzzySet;

typedef struct {
    char name[32];
    FuzzySet sets[MAX_SETS];
    int set_count;
} FuzzyVariable;

float membership_triangle(float x, float a, float b, float c) {
    if (x <= a || x >= c) return 0.0f;
    if (x <= b) return (x - a) / (b - a);
    return (c - x) / (c - b);
}

float membership_trapezoid(float x, float a, float b, float c, float d) {
    if (x <= a || x >= d) return 0.0f;
    if (x >= b && x <= c) return 1.0f;
    if (x < b) return (x - a) / (b - a);
    return (d - x) / (d - c);
}

float fuzzy_evaluate_set(FuzzySet *set, float value) {
    if (set->type == FUZZY_TRIANGLE) {
        return membership_triangle(value, set->a, set->b, set->c);
    } else {
        return membership_trapezoid(value, set->a, set->b, set->c, set->d);
    }
}

void fuzzy_variable_init(FuzzyVariable *var, const char *name) {
    strncpy(var->name, name, sizeof(var->name) - 1);
    var->set_count = 0;
}

void fuzzy_add_set_triangle(FuzzyVariable *var, float a, float b, float c) {
    if (var->set_count >= MAX_SETS) return;
    FuzzySet *set = &var->sets[var->set_count++];
    set->type = FUZZY_TRIANGLE;
    set->a = a;
    set->b = b;
    set->c = c;
}

float fuzzy_fuzzify(FuzzyVariable *var, float value, int set_index) {
    if (set_index >= var->set_count) return 0.0f;
    return fuzzy_evaluate_set(&var->sets[set_index], value);
}

float fuzzy_defuzzify_centroid(float *memberships, float *values, int count) {
    float numerator = 0.0f;
    float denominator = 0.0f;
    for (int i = 0; i < count; i++) {
        numerator += memberships[i] * values[i];
        denominator += memberships[i];
    }
    return (denominator > 0.001f) ? (numerator / denominator) : 0.0f;
}

float fuzzy_and(float a, float b) { return fminf(a, b); }
float fuzzy_or(float a, float b) { return fmaxf(a, b); }

/*
 * constraint_layout.c
 * Constraint-Based Layout System Implementation
 * Advanced constraint solving using modified Cassowary algorithm
 *
 * This implementation provides a flexible constraint-based layout system
 * that can solve complex layout problems using linear constraint equations.
 * The Cassowary algorithm is used for efficient constraint solving.
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "constraint_layout.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

/* Tableau entry for the simplex solver */
typedef struct {
    ConstraintVariable* var;
    float coefficient;
} TableauEntry;

/* Row in the simplex tableau */
typedef struct {
    TableauEntry* entries;
    uint32_t entry_count;
    uint32_t entry_capacity;
    float constant;
} TableauRow;

/* Simplex tableau for constraint solving */
typedef struct {
    TableauRow* rows;
    uint32_t row_count;
    uint32_t row_capacity;
} SimplexTableau;

/* Expression simplification cache */
typedef struct {
    Constraint* constraint;
    ConstraintExpression simplified_lhs;
    ConstraintExpression simplified_rhs;
    bool is_valid;
} ExpressionCacheEntry;

typedef struct {
    ExpressionCacheEntry* entries;
    uint32_t entry_count;
    uint32_t entry_capacity;
} ExpressionCache;

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static uint32_t g_next_variable_id = 1;
static uint32_t g_next_constraint_id = 1;
static uint32_t g_next_element_id = 1;

/* ============================================================================
 * HELPER FUNCTIONS - EXPRESSION OPERATIONS
 * ============================================================================ */

static ConstraintExpression expression_create(void) {
    ConstraintExpression expr = {0};
    expr.var_capacity = 8;
    expr.variables = malloc(sizeof(ConstraintVariable*) * expr.var_capacity);
    expr.coefficients = malloc(sizeof(float) * expr.var_capacity);
    expr.var_count = 0;
    expr.constant = 0.0f;
    return expr;
}

static void expression_destroy(ConstraintExpression* expr) {
    if (expr->variables) {
        free(expr->variables);
        expr->variables = NULL;
    }
    if (expr->coefficients) {
        free(expr->coefficients);
        expr->coefficients = NULL;
    }
    expr->var_count = 0;
    expr->var_capacity = 0;
}

static void expression_add_term(ConstraintExpression* expr, ConstraintVariable* var, float coeff) {
    assert(expr && var);

    /* Check if variable already exists */
    for (uint32_t i = 0; i < expr->var_count; i++) {
        if (expr->variables[i]->id == var->id) {
            expr->coefficients[i] += coeff;
            return;
        }
    }

    /* Add new variable */
    if (expr->var_count >= expr->var_capacity) {
        expr->var_capacity *= 2;
        expr->variables = realloc(expr->variables, sizeof(ConstraintVariable*) * expr->var_capacity);
        expr->coefficients = realloc(expr->coefficients, sizeof(float) * expr->var_capacity);
    }

    expr->variables[expr->var_count] = var;
    expr->coefficients[expr->var_count] = coeff;
    expr->var_count++;
}

static void expression_add_constant(ConstraintExpression* expr, float constant) {
    expr->constant += constant;
}

static ConstraintExpression expression_copy(const ConstraintExpression* src) {
    ConstraintExpression dest = expression_create();
    dest.constant = src->constant;
    dest.var_count = src->var_count;

    if (dest.var_count > dest.var_capacity) {
        dest.var_capacity = dest.var_count + 4;
        dest.variables = realloc(dest.variables, sizeof(ConstraintVariable*) * dest.var_capacity);
        dest.coefficients = realloc(dest.coefficients, sizeof(float) * dest.var_capacity);
    }

    memcpy(dest.variables, src->variables, sizeof(ConstraintVariable*) * src->var_count);
    memcpy(dest.coefficients, src->coefficients, sizeof(float) * src->var_count);

    return dest;
}

static void expression_normalize(ConstraintExpression* expr) {
    /* Remove zero coefficients and consolidate duplicate variables */
    uint32_t write_idx = 0;

    for (uint32_t i = 0; i < expr->var_count; i++) {
        if (fabsf(expr->coefficients[i]) > 1e-10f) {
            expr->variables[write_idx] = expr->variables[i];
            expr->coefficients[write_idx] = expr->coefficients[i];
            write_idx++;
        }
    }

    expr->var_count = write_idx;
}

/* ============================================================================
 * HELPER FUNCTIONS - VARIABLE MANAGEMENT
 * ============================================================================ */

static ConstraintVariable* variable_create(ConstraintVariableType type, uint32_t element_id) {
    ConstraintVariable* var = malloc(sizeof(ConstraintVariable));
    var->id = g_next_variable_id++;
    var->type = type;
    var->element_id = element_id;
    var->current_value = 0.0f;
    var->is_external = false;
    return var;
}

static void variable_destroy(ConstraintVariable* var) {
    free(var);
}

static ConstraintVariable* layouter_get_or_create_variable(ConstraintLayouter* layouter,
                                                           ConstraintVariableType type,
                                                           uint32_t element_id) {
    assert(layouter);

    /* Check if variable already exists */
    for (uint32_t i = 0; i < layouter->variable_count; i++) {
        ConstraintVariable* var = layouter->variables[i];
        if (var->type == type && var->element_id == element_id) {
            return var;
        }
    }

    /* Create new variable */
    ConstraintVariable* var = variable_create(type, element_id);

    if (layouter->variable_count >= layouter->variable_capacity) {
        layouter->variable_capacity = layouter->variable_capacity ? layouter->variable_capacity * 2 : 16;
        layouter->variables = realloc(layouter->variables,
                                     sizeof(ConstraintVariable*) * layouter->variable_capacity);
    }

    layouter->variables[layouter->variable_count++] = var;
    return var;
}

/* ============================================================================
 * TABLEAU OPERATIONS (SIMPLEX SOLVER)
 * ============================================================================ */

static SimplexTableau* tableau_create(void) {
    SimplexTableau* tableau = malloc(sizeof(SimplexTableau));
    tableau->row_capacity = 16;
    tableau->rows = malloc(sizeof(TableauRow) * tableau->row_capacity);
    tableau->row_count = 0;
    return tableau;
}

static void tableau_destroy(SimplexTableau* tableau) {
    if (!tableau) return;

    for (uint32_t i = 0; i < tableau->row_count; i++) {
        if (tableau->rows[i].entries) {
            free(tableau->rows[i].entries);
        }
    }
    free(tableau->rows);
    free(tableau);
}

static void tableau_add_row(SimplexTableau* tableau, const ConstraintExpression* expr, float constant) {
    if (tableau->row_count >= tableau->row_capacity) {
        tableau->row_capacity *= 2;
        tableau->rows = realloc(tableau->rows, sizeof(TableauRow) * tableau->row_capacity);
    }

    TableauRow* row = &tableau->rows[tableau->row_count++];
    row->entry_capacity = 8;
    row->entries = malloc(sizeof(TableauEntry) * row->entry_capacity);
    row->entry_count = expr->var_count;
    row->constant = constant;

    if (row->entry_count > row->entry_capacity) {
        row->entry_capacity = row->entry_count + 4;
        row->entries = realloc(row->entries, sizeof(TableauEntry) * row->entry_capacity);
    }

    for (uint32_t i = 0; i < expr->var_count; i++) {
        row->entries[i].var = expr->variables[i];
        row->entries[i].coefficient = expr->coefficients[i];
    }
}

static void tableau_normalize_rows(SimplexTableau* tableau) {
    /* Normalize each row so leading coefficient is 1 */
    for (uint32_t i = 0; i < tableau->row_count; i++) {
        TableauRow* row = &tableau->rows[i];
        if (row->entry_count > 0) {
            float leading_coeff = row->entries[0].coefficient;
            if (fabsf(leading_coeff) > 1e-10f) {
                for (uint32_t j = 0; j < row->entry_count; j++) {
                    row->entries[j].coefficient /= leading_coeff;
                }
                row->constant /= leading_coeff;
            }
        }
    }
}

/* ============================================================================
 * CONSTRAINT SOLVING
 * ============================================================================ */

static bool solve_constraints_cassowary(ConstraintLayouter* layouter) {
    assert(layouter);

    if (!layouter->tableau) {
        layouter->tableau = tableau_create();
    }

    SimplexTableau* tableau = (SimplexTableau*)layouter->tableau;

    /* Clear previous tableau */
    for (uint32_t i = 0; i < tableau->row_count; i++) {
        if (tableau->rows[i].entries) {
            free(tableau->rows[i].entries);
            tableau->rows[i].entries = NULL;
        }
    }
    tableau->row_count = 0;

    /* Build tableau from active constraints */
    uint32_t iteration = 0;
    bool converged = false;

    for (iteration = 0; iteration < layouter->max_iterations && !converged; iteration++) {
        converged = true;

        /* Process each constraint */
        for (uint32_t i = 0; i < layouter->constraint_count; i++) {
            Constraint* constraint = layouter->constraints[i];
            if (!constraint->is_active) continue;

            /* Build equation: lhs - rhs = 0 */
            ConstraintExpression equation = expression_create();

            /* Add LHS terms */
            for (uint32_t j = 0; j < constraint->lhs.var_count; j++) {
                expression_add_term(&equation, constraint->lhs.variables[j],
                                   constraint->lhs.coefficients[j]);
            }

            /* Subtract RHS terms */
            for (uint32_t j = 0; j < constraint->rhs.var_count; j++) {
                expression_add_term(&equation, constraint->rhs.variables[j],
                                   -constraint->rhs.coefficients[j]);
            }

            float constant = constraint->rhs.constant - constraint->lhs.constant;

            /* Handle different operators */
            switch (constraint->operator) {
                case CONSTRAINT_EQ:
                    tableau_add_row(tableau, &equation, constant);
                    break;
                case CONSTRAINT_LE:
                    /* c <= d becomes c - d <= 0 */
                    if (constant > layouter->epsilon) {
                        converged = false;
                    }
                    break;
                case CONSTRAINT_GE:
                    /* c >= d becomes c - d >= 0 */
                    if (constant < -layouter->epsilon) {
                        converged = false;
                    }
                    break;
            }

            expression_destroy(&equation);
        }

        if (converged) break;
    }

    /* Normalize tableau rows */
    tableau_normalize_rows(tableau);

    /* Back-substitute to compute variable values */
    for (uint32_t i = 0; i < layouter->variable_count; i++) {
        ConstraintVariable* var = layouter->variables[i];

        /* Look for equation solving for this variable */
        for (uint32_t j = 0; j < tableau->row_count; j++) {
            TableauRow* row = &tableau->rows[j];
            if (row->entry_count == 1 &&
                fabsf(row->entries[0].coefficient - 1.0f) < layouter->epsilon &&
                row->entries[0].var->id == var->id) {
                var->current_value = row->constant;
                break;
            }
        }
    }

    /* Store performance metrics */
    layouter->iteration_count = iteration;

    return converged || iteration >= layouter->max_iterations;
}

/* ============================================================================
 * ELEMENT CONSTRAINT SETUP
 * ============================================================================ */

static void setup_derived_constraints(ConstraintLayouter* layouter, ConstraintElement* element) {
    assert(layouter && element);

    /* Create derived variable constraints:
       right = x + width
       bottom = y + height
       center_x = x + width/2
       center_y = y + height/2
    */

    /* right = x + width */
    {
        ConstraintExpression lhs = expression_create();
        expression_add_term(&lhs, &element->var_right, 1.0f);

        ConstraintExpression rhs = expression_create();
        expression_add_term(&rhs, &element->var_x, 1.0f);
        expression_add_term(&rhs, &element->var_width, 1.0f);

        Constraint* constraint = malloc(sizeof(Constraint));
        constraint->id = g_next_constraint_id++;
        constraint->lhs = lhs;
        constraint->rhs = rhs;
        constraint->operator = CONSTRAINT_EQ;
        constraint->strength = CONSTRAINT_STRENGTH_REQUIRED;
        constraint->is_active = true;
        constraint->internal_data = NULL;

        if (element->constraint_count >= element->constraint_capacity) {
            element->constraint_capacity = element->constraint_capacity ? element->constraint_capacity * 2 : 4;
            element->constraints = realloc(element->constraints,
                                          sizeof(Constraint*) * element->constraint_capacity);
        }
        element->constraints[element->constraint_count++] = constraint;
    }

    /* bottom = y + height */
    {
        ConstraintExpression lhs = expression_create();
        expression_add_term(&lhs, &element->var_bottom, 1.0f);

        ConstraintExpression rhs = expression_create();
        expression_add_term(&rhs, &element->var_y, 1.0f);
        expression_add_term(&rhs, &element->var_height, 1.0f);

        Constraint* constraint = malloc(sizeof(Constraint));
        constraint->id = g_next_constraint_id++;
        constraint->lhs = lhs;
        constraint->rhs = rhs;
        constraint->operator = CONSTRAINT_EQ;
        constraint->strength = CONSTRAINT_STRENGTH_REQUIRED;
        constraint->is_active = true;
        constraint->internal_data = NULL;

        if (element->constraint_count >= element->constraint_capacity) {
            element->constraint_capacity *= 2;
            element->constraints = realloc(element->constraints,
                                          sizeof(Constraint*) * element->constraint_capacity);
        }
        element->constraints[element->constraint_count++] = constraint;
    }
}

/* ============================================================================
 * LAYOUTER MANAGEMENT API
 * ============================================================================ */

ConstraintLayouter* constraint_layouter_create(void) {
    ConstraintLayouter* layouter = malloc(sizeof(ConstraintLayouter));
    memset(layouter, 0, sizeof(ConstraintLayouter));

    layouter->variable_capacity = 32;
    layouter->variables = malloc(sizeof(ConstraintVariable*) * layouter->variable_capacity);

    layouter->constraint_capacity = 32;
    layouter->constraints = malloc(sizeof(Constraint*) * layouter->constraint_capacity);

    layouter->element_capacity = 16;
    layouter->elements = malloc(sizeof(ConstraintElement*) * layouter->element_capacity);

    layouter->layout_cache = malloc(sizeof(LayoutCache));
    layouter->layout_cache->entry_capacity = 32;
    layouter->layout_cache->entries = malloc(sizeof(LayoutCacheEntry) * layouter->layout_cache->entry_capacity);
    layouter->layout_cache->entry_count = 0;

    layouter->epsilon = 1e-6f;
    layouter->max_iterations = 1000;
    layouter->enable_debug = false;
    layouter->current_generation = 0;

    return layouter;
}

void constraint_layouter_destroy(ConstraintLayouter* layouter) {
    if (!layouter) return;

    /* Destroy all constraints */
    for (uint32_t i = 0; i < layouter->constraint_count; i++) {
        Constraint* constraint = layouter->constraints[i];
        expression_destroy(&constraint->lhs);
        expression_destroy(&constraint->rhs);
        free(constraint);
    }

    /* Destroy all variables */
    for (uint32_t i = 0; i < layouter->variable_count; i++) {
        variable_destroy(layouter->variables[i]);
    }

    /* Destroy all elements */
    for (uint32_t i = 0; i < layouter->element_count; i++) {
        ConstraintElement* element = layouter->elements[i];
        free(element->constraints);
        free(element->children);
        free(element);
    }

    /* Destroy tableau */
    if (layouter->tableau) {
        tableau_destroy((SimplexTableau*)layouter->tableau);
    }

    /* Destroy cache */
    if (layouter->layout_cache) {
        free(layouter->layout_cache->entries);
        free(layouter->layout_cache);
    }

    free(layouter->variables);
    free(layouter->constraints);
    free(layouter->elements);
    free(layouter);
}

void constraint_layouter_set_epsilon(ConstraintLayouter* layouter, float epsilon) {
    if (layouter) {
        layouter->epsilon = epsilon;
    }
}

void constraint_layouter_set_max_iterations(ConstraintLayouter* layouter, uint32_t max_iterations) {
    if (layouter) {
        layouter->max_iterations = max_iterations;
    }
}

void constraint_layouter_enable_debug(ConstraintLayouter* layouter, bool enable) {
    if (layouter) {
        layouter->enable_debug = enable;
    }
}

/* ============================================================================
 * ELEMENT MANAGEMENT API
 * ============================================================================ */

ConstraintElement* constraint_element_create(ConstraintLayouter* layouter, const char* name) {
    assert(layouter);

    ConstraintElement* element = malloc(sizeof(ConstraintElement));
    memset(element, 0, sizeof(ConstraintElement));

    element->id = g_next_element_id++;
    element->name = name ? strdup(name) : NULL;

    /* Create variables for this element */
    element->var_x = *layouter_get_or_create_variable(layouter, VAR_X, element->id);
    element->var_y = *layouter_get_or_create_variable(layouter, VAR_Y, element->id);
    element->var_width = *layouter_get_or_create_variable(layouter, VAR_WIDTH, element->id);
    element->var_height = *layouter_get_or_create_variable(layouter, VAR_HEIGHT, element->id);
    element->var_right = *layouter_get_or_create_variable(layouter, VAR_RIGHT, element->id);
    element->var_bottom = *layouter_get_or_create_variable(layouter, VAR_BOTTOM, element->id);
    element->var_center_x = *layouter_get_or_create_variable(layouter, VAR_CENTER_X, element->id);
    element->var_center_y = *layouter_get_or_create_variable(layouter, VAR_CENTER_Y, element->id);

    /* Initialize properties */
    element->min_width = 0.0f;
    element->min_height = 0.0f;
    element->max_width = 10000.0f;
    element->max_height = 10000.0f;
    element->preferred_width = 0.0f;
    element->preferred_height = 0.0f;
    element->intrinsic_width = 0.0f;
    element->intrinsic_height = 0.0f;
    element->is_visible = true;
    element->is_dirty = true;
    element->needs_layout = true;
    element->generation = 0;

    element->constraint_capacity = 4;
    element->constraints = malloc(sizeof(Constraint*) * element->constraint_capacity);
    element->constraint_count = 0;

    element->child_capacity = 8;
    element->children = malloc(sizeof(ConstraintElement*) * element->child_capacity);
    element->child_count = 0;

    /* Add element to layouter */
    if (layouter->element_count >= layouter->element_capacity) {
        layouter->element_capacity *= 2;
        layouter->elements = realloc(layouter->elements,
                                    sizeof(ConstraintElement*) * layouter->element_capacity);
    }
    layouter->elements[layouter->element_count++] = element;

    /* Set up derived constraints */
    setup_derived_constraints(layouter, element);

    layouter->needs_rebuild = true;

    return element;
}

void constraint_element_destroy(ConstraintLayouter* layouter, ConstraintElement* element) {
    if (!element) return;

    /* Remove from layouter's element list */
    if (layouter) {
        for (uint32_t i = 0; i < layouter->element_count; i++) {
            if (layouter->elements[i]->id == element->id) {
                layouter->elements[i] = layouter->elements[layouter->element_count - 1];
                layouter->element_count--;
                break;
            }
        }
    }

    /* Destroy constraints */
    for (uint32_t i = 0; i < element->constraint_count; i++) {
        expression_destroy(&element->constraints[i]->lhs);
        expression_destroy(&element->constraints[i]->rhs);
        free(element->constraints[i]);
    }

    free(element->constraints);
    free(element->children);
    free(element->name);
    free(element);
}

void constraint_element_add_child(ConstraintElement* parent, ConstraintElement* child) {
    assert(parent && child);

    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->children = realloc(parent->children,
                                  sizeof(ConstraintElement*) * parent->child_capacity);
    }

    parent->children[parent->child_count++] = child;
    child->parent = parent;
}

void constraint_element_remove_child(ConstraintElement* parent, ConstraintElement* child) {
    assert(parent && child);

    for (uint32_t i = 0; i < parent->child_count; i++) {
        if (parent->children[i]->id == child->id) {
            parent->children[i] = parent->children[parent->child_count - 1];
            parent->child_count--;
            break;
        }
    }

    child->parent = NULL;
}

void constraint_element_set_bounds(ConstraintElement* element,
                                   float min_width, float min_height,
                                   float max_width, float max_height) {
    assert(element);
    element->min_width = min_width;
    element->min_height = min_height;
    element->max_width = max_width;
    element->max_height = max_height;
    element->is_dirty = true;
}

void constraint_element_set_intrinsic_size(ConstraintElement* element,
                                          float width, float height) {
    assert(element);
    element->intrinsic_width = width;
    element->intrinsic_height = height;
    element->is_dirty = true;
}

void constraint_element_set_margin(ConstraintElement* element,
                                   float top, float right, float bottom, float left) {
    assert(element);
    element->margin_top = top;
    element->margin_right = right;
    element->margin_bottom = bottom;
    element->margin_left = left;
    element->is_dirty = true;
}

void constraint_element_set_padding(ConstraintElement* element,
                                    float top, float right, float bottom, float left) {
    assert(element);
    element->padding_top = top;
    element->padding_right = right;
    element->padding_bottom = bottom;
    element->padding_left = left;
    element->is_dirty = true;
}

void constraint_element_set_visible(ConstraintElement* element, bool visible) {
    assert(element);
    if (element->is_visible != visible) {
        element->is_visible = visible;
        element->needs_layout = true;
    }
}

/* ============================================================================
 * CONSTRAINT CREATION API
 * ============================================================================ */

static Constraint* constraint_add_internal(ConstraintLayouter* layouter,
                                          const ConstraintExpression* lhs,
                                          ConstraintOperator operator,
                                          const ConstraintExpression* rhs,
                                          ConstraintStrength strength) {
    assert(layouter && lhs && rhs);

    Constraint* constraint = malloc(sizeof(Constraint));
    constraint->id = g_next_constraint_id++;
    constraint->lhs = expression_copy(lhs);
    constraint->rhs = expression_copy(rhs);
    constraint->operator = operator;
    constraint->strength = strength;
    constraint->is_active = true;
    constraint->internal_data = NULL;

    if (layouter->constraint_count >= layouter->constraint_capacity) {
        layouter->constraint_capacity *= 2;
        layouter->constraints = realloc(layouter->constraints,
                                       sizeof(Constraint*) * layouter->constraint_capacity);
    }

    layouter->constraints[layouter->constraint_count++] = constraint;
    layouter->needs_rebuild = true;

    return constraint;
}

Constraint* constraint_add_equal(ConstraintLayouter* layouter,
                                ConstraintElement* element1, ConstraintVariableType var1,
                                ConstraintElement* element2, ConstraintVariableType var2,
                                ConstraintStrength strength) {
    assert(layouter && element1 && element2);

    ConstraintExpression lhs = expression_create();
    ConstraintVariable* v1 = layouter_get_or_create_variable(layouter, var1, element1->id);
    expression_add_term(&lhs, v1, 1.0f);

    ConstraintExpression rhs = expression_create();
    ConstraintVariable* v2 = layouter_get_or_create_variable(layouter, var2, element2->id);
    expression_add_term(&rhs, v2, 1.0f);

    Constraint* constraint = constraint_add_internal(layouter, &lhs, CONSTRAINT_EQ, &rhs, strength);

    expression_destroy(&lhs);
    expression_destroy(&rhs);

    return constraint;
}

Constraint* constraint_add_less_than_or_equal(ConstraintLayouter* layouter,
                                              ConstraintElement* element1, ConstraintVariableType var1,
                                              ConstraintElement* element2, ConstraintVariableType var2,
                                              ConstraintStrength strength) {
    assert(layouter && element1 && element2);

    ConstraintExpression lhs = expression_create();
    ConstraintVariable* v1 = layouter_get_or_create_variable(layouter, var1, element1->id);
    expression_add_term(&lhs, v1, 1.0f);

    ConstraintExpression rhs = expression_create();
    ConstraintVariable* v2 = layouter_get_or_create_variable(layouter, var2, element2->id);
    expression_add_term(&rhs, v2, 1.0f);

    Constraint* constraint = constraint_add_internal(layouter, &lhs, CONSTRAINT_LE, &rhs, strength);

    expression_destroy(&lhs);
    expression_destroy(&rhs);

    return constraint;
}

Constraint* constraint_add_greater_than_or_equal(ConstraintLayouter* layouter,
                                                 ConstraintElement* element1, ConstraintVariableType var1,
                                                 ConstraintElement* element2, ConstraintVariableType var2,
                                                 ConstraintStrength strength) {
    assert(layouter && element1 && element2);

    ConstraintExpression lhs = expression_create();
    ConstraintVariable* v1 = layouter_get_or_create_variable(layouter, var1, element1->id);
    expression_add_term(&lhs, v1, 1.0f);

    ConstraintExpression rhs = expression_create();
    ConstraintVariable* v2 = layouter_get_or_create_variable(layouter, var2, element2->id);
    expression_add_term(&rhs, v2, 1.0f);

    Constraint* constraint = constraint_add_internal(layouter, &lhs, CONSTRAINT_GE, &rhs, strength);

    expression_destroy(&lhs);
    expression_destroy(&rhs);

    return constraint;
}

Constraint* constraint_add_constant_equal(ConstraintLayouter* layouter,
                                         ConstraintElement* element, ConstraintVariableType var,
                                         float constant, ConstraintStrength strength) {
    assert(layouter && element);

    ConstraintExpression lhs = expression_create();
    ConstraintVariable* v = layouter_get_or_create_variable(layouter, var, element->id);
    expression_add_term(&lhs, v, 1.0f);

    ConstraintExpression rhs = expression_create();
    expression_add_constant(&rhs, constant);

    Constraint* constraint = constraint_add_internal(layouter, &lhs, CONSTRAINT_EQ, &rhs, strength);

    expression_destroy(&lhs);
    expression_destroy(&rhs);

    return constraint;
}

Constraint* constraint_add_constant_less_than_or_equal(ConstraintLayouter* layouter,
                                                       ConstraintElement* element, ConstraintVariableType var,
                                                       float constant, ConstraintStrength strength) {
    assert(layouter && element);

    ConstraintExpression lhs = expression_create();
    ConstraintVariable* v = layouter_get_or_create_variable(layouter, var, element->id);
    expression_add_term(&lhs, v, 1.0f);

    ConstraintExpression rhs = expression_create();
    expression_add_constant(&rhs, constant);

    Constraint* constraint = constraint_add_internal(layouter, &lhs, CONSTRAINT_LE, &rhs, strength);

    expression_destroy(&lhs);
    expression_destroy(&rhs);

    return constraint;
}

Constraint* constraint_add_constant_greater_than_or_equal(ConstraintLayouter* layouter,
                                                          ConstraintElement* element, ConstraintVariableType var,
                                                          float constant, ConstraintStrength strength) {
    assert(layouter && element);

    ConstraintExpression lhs = expression_create();
    ConstraintVariable* v = layouter_get_or_create_variable(layouter, var, element->id);
    expression_add_term(&lhs, v, 1.0f);

    ConstraintExpression rhs = expression_create();
    expression_add_constant(&rhs, constant);

    Constraint* constraint = constraint_add_internal(layouter, &lhs, CONSTRAINT_GE, &rhs, strength);

    expression_destroy(&lhs);
    expression_destroy(&rhs);

    return constraint;
}

Constraint* constraint_add_linear(ConstraintLayouter* layouter,
                                 const ConstraintVariable** variables,
                                 const float* coefficients,
                                 uint32_t var_count,
                                 ConstraintOperator operator,
                                 float constant,
                                 ConstraintStrength strength) {
    assert(layouter && variables && coefficients);

    ConstraintExpression lhs = expression_create();
    for (uint32_t i = 0; i < var_count; i++) {
        expression_add_term(&lhs, (ConstraintVariable*)variables[i], coefficients[i]);
    }

    ConstraintExpression rhs = expression_create();
    expression_add_constant(&rhs, constant);

    Constraint* constraint = constraint_add_internal(layouter, &lhs, operator, &rhs, strength);

    expression_destroy(&lhs);
    expression_destroy(&rhs);

    return constraint;
}

void constraint_remove(ConstraintLayouter* layouter, Constraint* constraint) {
    assert(layouter && constraint);

    for (uint32_t i = 0; i < layouter->constraint_count; i++) {
        if (layouter->constraints[i]->id == constraint->id) {
            expression_destroy(&layouter->constraints[i]->lhs);
            expression_destroy(&layouter->constraints[i]->rhs);
            free(layouter->constraints[i]);

            layouter->constraints[i] = layouter->constraints[layouter->constraint_count - 1];
            layouter->constraint_count--;
            layouter->needs_rebuild = true;
            break;
        }
    }
}

/* ============================================================================
 * LAYOUT SOLVING API
 * ============================================================================ */

bool constraint_layout_solve(ConstraintLayouter* layouter,
                            float available_width, float available_height) {
    assert(layouter);

    /* Add boundary constraints for root elements */
    for (uint32_t i = 0; i < layouter->element_count; i++) {
        ConstraintElement* element = layouter->elements[i];
        if (!element->parent && element->is_visible) {
            constraint_add_constant_equal(layouter, element, VAR_X, 0.0f, CONSTRAINT_STRENGTH_REQUIRED);
            constraint_add_constant_equal(layouter, element, VAR_Y, 0.0f, CONSTRAINT_STRENGTH_REQUIRED);
            constraint_add_constant_equal(layouter, element, VAR_WIDTH, available_width, CONSTRAINT_STRENGTH_REQUIRED);
            constraint_add_constant_equal(layouter, element, VAR_HEIGHT, available_height, CONSTRAINT_STRENGTH_REQUIRED);
        }
    }

    /* Solve all constraints */
    bool result = solve_constraints_cassowary(layouter);

    if (layouter->enable_debug) {
        printf("[Constraint Solver] Iterations: %u, Converged: %s\n",
               layouter->iteration_count, result ? "yes" : "no");
    }

    /* Mark cache as dirty */
    layouter->current_generation++;

    return result;
}

void constraint_element_get_layout(const ConstraintElement* element,
                                  float* out_x, float* out_y,
                                  float* out_width, float* out_height) {
    assert(element && out_x && out_y && out_width && out_height);

    *out_x = element->var_x.current_value;
    *out_y = element->var_y.current_value;
    *out_width = element->var_width.current_value;
    *out_height = element->var_height.current_value;
}

/* ============================================================================
 * INVALIDATION AND CACHING API
 * ============================================================================ */

void constraint_element_invalidate(ConstraintElement* element) {
    assert(element);

    element->is_dirty = true;
    element->needs_layout = true;
    element->generation++;

    /* Propagate to children */
    for (uint32_t i = 0; i < element->child_count; i++) {
        constraint_element_invalidate(element->children[i]);
    }
}

void constraint_layouter_invalidate_all(ConstraintLayouter* layouter) {
    assert(layouter);

    for (uint32_t i = 0; i < layouter->element_count; i++) {
        constraint_element_invalidate(layouter->elements[i]);
    }

    layouter->current_generation++;
}

bool constraint_layout_is_cached(const ConstraintLayouter* layouter,
                                const ConstraintElement* element) {
    assert(layouter && element);

    if (!layouter->layout_cache) return false;

    for (uint32_t i = 0; i < layouter->layout_cache->entry_count; i++) {
        LayoutCacheEntry* entry = &layouter->layout_cache->entries[i];
        if (entry->element_id == element->id &&
            entry->generation == element->generation &&
            entry->is_valid) {
            return true;
        }
    }

    return false;
}

void constraint_layout_cache_clear(ConstraintLayouter* layouter) {
    assert(layouter && layouter->layout_cache);
    layouter->layout_cache->entry_count = 0;
}

/* ============================================================================
 * PERFORMANCE AND DEBUGGING API
 * ============================================================================ */

void constraint_layouter_get_stats(const ConstraintLayouter* layouter,
                                  uint32_t* out_iterations,
                                  float* out_time_ms) {
    assert(layouter && out_iterations && out_time_ms);

    *out_iterations = layouter->iteration_count;
    *out_time_ms = layouter->solve_time_ms;
}

uint32_t constraint_layouter_get_constraint_count(const ConstraintLayouter* layouter) {
    return layouter ? layouter->constraint_count : 0;
}

uint32_t constraint_layouter_get_variable_count(const ConstraintLayouter* layouter) {
    return layouter ? layouter->variable_count : 0;
}

uint32_t constraint_layouter_get_element_count(const ConstraintLayouter* layouter) {
    return layouter ? layouter->element_count : 0;
}

void constraint_layouter_print_state(const ConstraintLayouter* layouter) {
    if (!layouter) return;

    printf("=== Constraint Layouter State ===\n");
    printf("Elements: %u\n", layouter->element_count);
    printf("Variables: %u\n", layouter->variable_count);
    printf("Constraints: %u\n", layouter->constraint_count);
    printf("Iterations: %u\n", layouter->iteration_count);
    printf("Generation: %u\n", layouter->current_generation);
    printf("Cache entries: %u\n", layouter->layout_cache ? layouter->layout_cache->entry_count : 0);
}

void constraint_element_print_layout(const ConstraintElement* element) {
    if (!element) return;

    printf("=== Element: %s (ID: %u) ===\n", element->name ? element->name : "unnamed", element->id);
    printf("Position: (%.2f, %.2f)\n", element->var_x.current_value, element->var_y.current_value);
    printf("Size: %.2f x %.2f\n", element->var_width.current_value, element->var_height.current_value);
    printf("Bounds: [%.2f-%.2f, %.2f-%.2f]\n",
           element->min_width, element->max_width,
           element->min_height, element->max_height);
    printf("Visible: %s\n", element->is_visible ? "yes" : "no");
    printf("Dirty: %s\n", element->is_dirty ? "yes" : "no");
}

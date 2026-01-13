/*
 * constraint_layout.h
 * Constraint-Based Layout System
 * Advanced constraint solving using modified Cassowary algorithm
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_CONSTRAINT_LAYOUT_H
#define UI_CONSTRAINT_LAYOUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "include/math/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES AND ENUMERATIONS
 * ============================================================================ */

/* Constraint strength levels - higher strength takes priority */
typedef enum {
    CONSTRAINT_STRENGTH_WEAK = 1,         /* Weakest - easily breakable */
    CONSTRAINT_STRENGTH_MEDIUM = 10,      /* Default for preferred values */
    CONSTRAINT_STRENGTH_STRONG = 100,     /* For important constraints */
    CONSTRAINT_STRENGTH_REQUIRED = 1000   /* Absolute constraints (must hold) */
} ConstraintStrength;

/* Constraint relationship operators */
typedef enum {
    CONSTRAINT_EQ,   /* Equal */
    CONSTRAINT_LE,   /* Less than or equal */
    CONSTRAINT_GE    /* Greater than or equal */
} ConstraintOperator;

/* Variable types in constraint system */
typedef enum {
    VAR_X,        /* Left position */
    VAR_Y,        /* Top position */
    VAR_WIDTH,    /* Width */
    VAR_HEIGHT,   /* Height */
    VAR_RIGHT,    /* Right edge (x + width) */
    VAR_BOTTOM,   /* Bottom edge (y + height) */
    VAR_CENTER_X, /* Center X position */
    VAR_CENTER_Y  /* Center Y position */
} ConstraintVariableType;

/* Forward declarations */
typedef struct ConstraintVariable ConstraintVariable;
typedef struct Constraint Constraint;
typedef struct ConstraintLayouter ConstraintLayouter;
typedef struct ConstraintElement ConstraintElement;

/* ============================================================================
 * CONSTRAINT VARIABLE
 * ============================================================================ */

/* Represents a variable that can be constrained (position, size, etc) */
typedef struct ConstraintVariable {
    uint32_t id;
    ConstraintVariableType type;
    uint32_t element_id;
    float current_value;
    bool is_external;      /* External variables are resolved by host */
} ConstraintVariable;

/* ============================================================================
 * CONSTRAINT EXPRESSION
 * ============================================================================ */

/* Linear expression: sum of (coefficient * variable) + constant */
typedef struct {
    ConstraintVariable** variables;
    float* coefficients;
    uint32_t var_count;
    uint32_t var_capacity;
    float constant;
} ConstraintExpression;

/* ============================================================================
 * CONSTRAINT
 * ============================================================================ */

/* Represents a single constraint in the solver */
typedef struct Constraint {
    uint32_t id;
    ConstraintExpression lhs;      /* Left-hand side */
    ConstraintOperator operator;   /* Constraint operator */
    ConstraintExpression rhs;      /* Right-hand side */
    ConstraintStrength strength;   /* Priority of constraint */
    bool is_active;
    void* internal_data;           /* Solver-specific data (tableau entries, etc) */
} Constraint;

/* ============================================================================
 * CONSTRAINT ELEMENT
 * ============================================================================ */

/* UI element with constraint layout properties */
typedef struct ConstraintElement {
    uint32_t id;
    const char* name;

    /* Position and size variables */
    ConstraintVariable var_x;
    ConstraintVariable var_y;
    ConstraintVariable var_width;
    ConstraintVariable var_height;

    /* Derived variables (right, bottom, center_x, center_y) */
    ConstraintVariable var_right;
    ConstraintVariable var_bottom;
    ConstraintVariable var_center_x;
    ConstraintVariable var_center_y;

    /* Constraints applied to this element */
    Constraint** constraints;
    uint32_t constraint_count;
    uint32_t constraint_capacity;

    /* Layout bounds */
    float min_width;
    float min_height;
    float max_width;
    float max_height;
    float preferred_width;
    float preferred_height;

    /* Spacing */
    float margin_top;
    float margin_right;
    float margin_bottom;
    float margin_left;
    float padding_top;
    float padding_right;
    float padding_bottom;
    float padding_left;

    /* Intrinsic size (content size) */
    float intrinsic_width;
    float intrinsic_height;

    /* Hierarchy */
    ConstraintElement* parent;
    ConstraintElement** children;
    uint32_t child_count;
    uint32_t child_capacity;

    /* Invalidation and caching */
    bool is_dirty;
    bool needs_layout;
    bool is_visible;
    uint32_t generation;           /* Incremented on invalidation */
} ConstraintElement;

/* ============================================================================
 * LAYOUT CACHE
 * ============================================================================ */

/* Caches computed layout to avoid redundant calculations */
typedef struct {
    uint32_t element_id;
    uint32_t generation;
    float x;
    float y;
    float width;
    float height;
    bool is_valid;
} LayoutCacheEntry;

typedef struct {
    LayoutCacheEntry* entries;
    uint32_t entry_count;
    uint32_t entry_capacity;
} LayoutCache;

/* ============================================================================
 * CONSTRAINT LAYOUTER (MAIN SOLVER)
 * ============================================================================ */

/* Main constraint layout solver object */
typedef struct ConstraintLayouter {
    /* Variables and constraints */
    ConstraintVariable** variables;
    uint32_t variable_count;
    uint32_t variable_capacity;

    Constraint** constraints;
    uint32_t constraint_count;
    uint32_t constraint_capacity;

    /* Elements being laid out */
    ConstraintElement** elements;
    uint32_t element_count;
    uint32_t element_capacity;

    /* Cassowary solver state */
    void* tableau;                 /* Simplex tableau for constraint solving */
    void* expression_cache;        /* Cache of simplified expressions */

    /* Layout cache and invalidation */
    LayoutCache* layout_cache;
    uint32_t current_generation;

    /* Solver state */
    bool needs_rebuild;            /* Set when constraints change */
    bool is_solving;
    uint32_t iteration_count;
    float solve_time_ms;

    /* Configuration */
    float epsilon;                 /* Tolerance for floating-point comparisons */
    uint32_t max_iterations;       /* Max iterations before giving up */
    bool enable_debug;
} ConstraintLayouter;

/* ============================================================================
 * API - LAYOUTER MANAGEMENT
 * ============================================================================ */

/* Create and destroy constraint layouter */
ConstraintLayouter* constraint_layouter_create(void);
void constraint_layouter_destroy(ConstraintLayouter* layouter);

/* Configure layouter */
void constraint_layouter_set_epsilon(ConstraintLayouter* layouter, float epsilon);
void constraint_layouter_set_max_iterations(ConstraintLayouter* layouter, uint32_t max_iterations);
void constraint_layouter_enable_debug(ConstraintLayouter* layouter, bool enable);

/* ============================================================================
 * API - ELEMENT MANAGEMENT
 * ============================================================================ */

/* Element creation and management */
ConstraintElement* constraint_element_create(ConstraintLayouter* layouter, const char* name);
void constraint_element_destroy(ConstraintLayouter* layouter, ConstraintElement* element);

/* Element hierarchy */
void constraint_element_add_child(ConstraintElement* parent, ConstraintElement* child);
void constraint_element_remove_child(ConstraintElement* parent, ConstraintElement* child);

/* Element properties */
void constraint_element_set_bounds(ConstraintElement* element,
                                   float min_width, float min_height,
                                   float max_width, float max_height);
void constraint_element_set_intrinsic_size(ConstraintElement* element,
                                          float width, float height);
void constraint_element_set_margin(ConstraintElement* element,
                                   float top, float right, float bottom, float left);
void constraint_element_set_padding(ConstraintElement* element,
                                    float top, float right, float bottom, float left);
void constraint_element_set_visible(ConstraintElement* element, bool visible);

/* ============================================================================
 * API - CONSTRAINT CREATION
 * ============================================================================ */

/* Simple constraint creation helpers */
Constraint* constraint_add_equal(ConstraintLayouter* layouter,
                                ConstraintElement* element1, ConstraintVariableType var1,
                                ConstraintElement* element2, ConstraintVariableType var2,
                                ConstraintStrength strength);

Constraint* constraint_add_less_than_or_equal(ConstraintLayouter* layouter,
                                              ConstraintElement* element1, ConstraintVariableType var1,
                                              ConstraintElement* element2, ConstraintVariableType var2,
                                              ConstraintStrength strength);

Constraint* constraint_add_greater_than_or_equal(ConstraintLayouter* layouter,
                                                 ConstraintElement* element1, ConstraintVariableType var1,
                                                 ConstraintElement* element2, ConstraintVariableType var2,
                                                 ConstraintStrength strength);

/* Constant value constraints */
Constraint* constraint_add_constant_equal(ConstraintLayouter* layouter,
                                         ConstraintElement* element, ConstraintVariableType var,
                                         float constant, ConstraintStrength strength);

Constraint* constraint_add_constant_less_than_or_equal(ConstraintLayouter* layouter,
                                                       ConstraintElement* element, ConstraintVariableType var,
                                                       float constant, ConstraintStrength strength);

Constraint* constraint_add_constant_greater_than_or_equal(ConstraintLayouter* layouter,
                                                          ConstraintElement* element, ConstraintVariableType var,
                                                          float constant, ConstraintStrength strength);

/* Advanced constraint: linear combination of variables */
Constraint* constraint_add_linear(ConstraintLayouter* layouter,
                                 const ConstraintVariable** variables,
                                 const float* coefficients,
                                 uint32_t var_count,
                                 ConstraintOperator operator,
                                 float constant,
                                 ConstraintStrength strength);

/* Remove constraint */
void constraint_remove(ConstraintLayouter* layouter, Constraint* constraint);

/* ============================================================================
 * API - LAYOUT SOLVING
 * ============================================================================ */

/* Solve all constraints and compute layout */
bool constraint_layout_solve(ConstraintLayouter* layouter,
                            float available_width, float available_height);

/* Get computed layout for element */
void constraint_element_get_layout(const ConstraintElement* element,
                                  float* out_x, float* out_y,
                                  float* out_width, float* out_height);

/* ============================================================================
 * API - INVALIDATION AND CACHING
 * ============================================================================ */

/* Mark element as needing layout recalculation */
void constraint_element_invalidate(ConstraintElement* element);

/* Mark entire layout as dirty (force full recalculation) */
void constraint_layouter_invalidate_all(ConstraintLayouter* layouter);

/* Check if cached layout is valid */
bool constraint_layout_is_cached(const ConstraintLayouter* layouter,
                                const ConstraintElement* element);

/* Clear layout cache */
void constraint_layout_cache_clear(ConstraintLayouter* layouter);

/* ============================================================================
 * API - PERFORMANCE AND DEBUGGING
 * ============================================================================ */

/* Get solver performance statistics */
void constraint_layouter_get_stats(const ConstraintLayouter* layouter,
                                  uint32_t* out_iterations,
                                  float* out_time_ms);

/* Get constraint information for debugging */
uint32_t constraint_layouter_get_constraint_count(const ConstraintLayouter* layouter);
uint32_t constraint_layouter_get_variable_count(const ConstraintLayouter* layouter);
uint32_t constraint_layouter_get_element_count(const ConstraintLayouter* layouter);

/* Print debug information */
void constraint_layouter_print_state(const ConstraintLayouter* layouter);
void constraint_element_print_layout(const ConstraintElement* element);

#ifdef __cplusplus
}
#endif

#endif /* UI_CONSTRAINT_LAYOUT_H */

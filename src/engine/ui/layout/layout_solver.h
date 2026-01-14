/*
 * layout_solver.h
 * Unified Layout Solver
 * Orchestrates layout passes (measure/arrange) across different layout systems
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_LAYOUT_SOLVER_H
#define UI_LAYOUT_SOLVER_H

#include "ui/ui_types.h"
#include "layout_invalidation.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {
    uint32_t total_elements_processed;
    uint32_t total_layouts_computed;
    uint32_t cache_hits;
    float total_time_ms;
} LayoutSolverStats;

typedef struct {
    LayoutInvalidationSystem* invalidation_system;
    LayoutSolverStats stats;
    bool debug_mode;
} LayoutSolver;

/* ============================================================================
 * API
 * ============================================================================ */

/* Lifecycle */
LayoutSolver* layout_solver_create(void);
void layout_solver_destroy(LayoutSolver* solver);

/* Configuration */
void layout_solver_set_invalidation_system(LayoutSolver* solver, LayoutInvalidationSystem* invalidation_system);
void layout_solver_set_debug(LayoutSolver* solver, bool enabled);

/* Solving */
/* Triggers a full layout pass starting from the root element */
void layout_solver_solve(LayoutSolver* solver, UIElement* root, float available_width, float available_height);

/* Partial update (if supported by invalidation system) */
void layout_solver_update(LayoutSolver* solver, UIElement* root);

/* Stats */
void layout_solver_reset_stats(LayoutSolver* solver);
LayoutSolverStats layout_solver_get_stats(const LayoutSolver* solver);

#ifdef __cplusplus
}
#endif

#endif /* UI_LAYOUT_SOLVER_H */

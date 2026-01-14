/*
 * layout_solver.c
 * Unified Layout Solver
 * Orchestrates layout passes (measure/arrange) across different layout systems
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "layout_solver.h"
#include "flexbox_layout.h"
#include "grid_layout.h"
#include "core/logger.h"
#include "core/memory.h"
#include "core/time_system.h"
#include <stdlib.h>
#include <string.h>

/* Use standard allocation to avoid conflicts with core headers during partial compilation */
#ifndef core_alloc
#define core_alloc(size) malloc(size)
#endif
#ifndef core_free
#define core_free(ptr) free(ptr)
#endif
#ifndef core_realloc
#define core_realloc(ptr, size) realloc(ptr, size)
#endif

/* ============================================================================
 * INTERNAL
 * ============================================================================
 */

static void solve_recursive(LayoutSolver* solver, UIElement* element, float available_width, float available_height) {
    if (!element || !element->visible) return;

    // Check invalidation system if available
    if (solver->invalidation_system) {
        // In a real implementation, we would check if we can skip this element
        // bool dirty = layout_invalidation_is_dirty(solver->invalidation_system, element->id);
        // if (!dirty && ...) return;

        // Register element if not tracked
        layout_invalidation_register_element(solver->invalidation_system, element->id);
    }

    solver->stats.total_elements_processed++;

    // 1. Layout this container (Measure & Arrange children relative to self)
    // The layout functions calculate the positions and sizes of children,
    // and also update the container's own size if auto.
    bool layout_computed = false;
    uint64_t start_time = get_time_nanos();

    switch (element->layout_type) {
        case LAYOUT_TYPE_FLEX:
            flexbox_layout((FlexboxContainer*)element, available_width, available_height);
            layout_computed = true;
            break;

        case LAYOUT_TYPE_GRID:
            grid_layout((GridContainer*)element, available_width, available_height);
            layout_computed = true;
            break;

        case LAYOUT_TYPE_CONSTRAINT:
            // TODO: Integrate constraint layout
            break;

        case LAYOUT_TYPE_NONE:
        default:
            // Leaf element or simple container.
            // Size and position are typically determined by the parent layout (Flex/Grid).
            // We do NOT overwrite size here, as it would undo the parent's work.
            // If this element needs to enforce its own size (e.g. strict preferred size),
            // it should have been handled by the parent's measurement phase.
            break;
    }

    if (layout_computed) {
        solver->stats.total_layouts_computed++;
    }

    // 2. Recursively solve children
    // Now that children have been sized and positioned by the parent's layout algorithm,
    // we need to ask them to layout their internal content.
    for (uint32_t i = 0; i < element->child_count; i++) {
        UIElement* child = element->children[i];

        // Pass the size allocated by the parent
        float child_width = child->layout.size.width;
        float child_height = child->layout.size.height;

        solve_recursive(solver, child, child_width, child_height);
    }

    // Cache result in invalidation system
    if (solver->invalidation_system) {
        layout_invalidation_cache_layout(solver->invalidation_system,
                                       element->id,
                                       element->layout.position.x,
                                       element->layout.position.y,
                                       element->layout.size.width,
                                       element->layout.size.height);
    }

    uint64_t end_time = get_time_nanos();
    solver->stats.total_time_ms += (float)(end_time - start_time) / 1000000.0f;
}

/* ============================================================================
 * API
 * ============================================================================
 */

LayoutSolver* layout_solver_create(void) {
    LayoutSolver* solver = core_alloc(sizeof(LayoutSolver));
    if (!solver) return NULL;

    memset(solver, 0, sizeof(LayoutSolver));
    return solver;
}

void layout_solver_destroy(LayoutSolver* solver) {
    if (solver) {
        core_free(solver);
    }
}

void layout_solver_set_invalidation_system(LayoutSolver* solver, LayoutInvalidationSystem* invalidation_system) {
    if (solver) {
        solver->invalidation_system = invalidation_system;
    }
}

void layout_solver_set_debug(LayoutSolver* solver, bool enabled) {
    if (solver) {
        solver->debug_mode = enabled;
    }
}

void layout_solver_solve(LayoutSolver* solver, UIElement* root, float available_width, float available_height) {
    if (!solver || !root) return;

    if (solver->debug_mode) {
        LOGI("Starting layout solve for root %s (%.2fx%.2f)",
             root->name ? root->name : "unnamed", available_width, available_height);
    }

    solve_recursive(solver, root, available_width, available_height);

    if (solver->debug_mode) {
        LOGI("Layout solve completed. Processed %u elements.", solver->stats.total_elements_processed);
    }
}

void layout_solver_update(LayoutSolver* solver, UIElement* root) {
    // For now, same as solve but assumes root has size set or we use current size
    if (!solver || !root) return;

    float w = root->layout.size.width;
    float h = root->layout.size.height;

    layout_solver_solve(solver, root, w, h);
}

void layout_solver_reset_stats(LayoutSolver* solver) {
    if (solver) {
        memset(&solver->stats, 0, sizeof(LayoutSolverStats));
    }
}

LayoutSolverStats layout_solver_get_stats(const LayoutSolver* solver) {
    if (solver) return solver->stats;
    LayoutSolverStats empty = {0};
    return empty;
}

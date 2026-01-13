/*
 * ui_layout_system.h
 * Main UI Layout System
 * Unified interface for multiple layout algorithms
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef UI_LAYOUT_SYSTEM_H
#define UI_LAYOUT_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include "layout/constraint_layout.h"
#include "layout/layout_invalidation.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * LAYOUT SYSTEM TYPES
 * ============================================================================ */

/* Supported layout engines */
typedef enum {
    LAYOUT_ENGINE_FLEXBOX,       /* CSS Flexbox layout */
    LAYOUT_ENGINE_GRID,          /* CSS Grid layout */
    LAYOUT_ENGINE_CONSTRAINT,    /* Constraint-based layout */
    LAYOUT_ENGINE_ABSOLUTE,      /* Absolute positioning */
    LAYOUT_ENGINE_CUSTOM         /* Custom layout engine */
} LayoutEngine;

/* Layout configuration */
typedef struct {
    LayoutEngine engine;
    float viewport_width;
    float viewport_height;
    bool enable_caching;
    bool enable_profiling;
    bool enable_debug;
} LayoutConfig;

/* UI Layout System - Comprehensive implementation */
typedef struct UILayoutSystem {
    uint32_t id;
    bool initialized;

    /* Layout configuration */
    LayoutConfig config;

    /* Constraint-based layout engine */
    ConstraintLayouter* constraint_layouter;

    /* Invalidation and caching system */
    LayoutInvalidationSystem* invalidation_system;

    /* Element hierarchy tracking */
    uint32_t* element_ids;
    uint32_t element_count;
    uint32_t element_capacity;

    /* Layout pipeline state */
    bool needs_full_layout;
    uint32_t current_layout_frame;

    /* Performance metrics */
    uint32_t total_layouts;
    float total_layout_time_ms;
    float avg_layout_time_ms;
    uint32_t cache_hits;
    uint32_t cache_misses;

    /* Timing information */
    float last_layout_duration_ms;
    float frame_time_budget_ms;
} UILayoutSystem;

/* ============================================================================
 * API - SYSTEM MANAGEMENT
 * ============================================================================ */

/* Initialize layout system */
void ui_layout_system_init(UILayoutSystem* system, const LayoutConfig* config);

/* Shutdown layout system */
void ui_layout_system_shutdown(UILayoutSystem* system);

/* Update layout system (called each frame) */
void ui_layout_system_update(UILayoutSystem* system, float dt);

/* Recalculate all layouts */
void ui_layout_system_relayout(UILayoutSystem* system);

/* ============================================================================
 * API - CONFIGURATION
 * ============================================================================ */

/* Set active layout engine */
void ui_layout_system_set_engine(UILayoutSystem* system, LayoutEngine engine);

/* Set viewport dimensions */
void ui_layout_system_set_viewport(UILayoutSystem* system, float width, float height);

/* Enable/disable caching */
void ui_layout_system_set_caching(UILayoutSystem* system, bool enable);

/* Enable/disable profiling */
void ui_layout_system_set_profiling(UILayoutSystem* system, bool enable);

/* Enable/disable debug output */
void ui_layout_system_set_debug(UILayoutSystem* system, bool enable);

/* ============================================================================
 * API - ELEMENT MANAGEMENT
 * ============================================================================ */

/* Register element with layout system */
void ui_layout_system_register_element(UILayoutSystem* system, uint32_t element_id);

/* Unregister element from layout system */
void ui_layout_system_unregister_element(UILayoutSystem* system, uint32_t element_id);

/* Get layout for element */
bool ui_layout_system_get_layout(const UILayoutSystem* system,
                                uint32_t element_id,
                                float* out_x, float* out_y,
                                float* out_width, float* out_height);

/* ============================================================================
 * API - CONSTRAINT-BASED LAYOUT
 * ============================================================================ */

/* Get constraint layouter for direct use */
ConstraintLayouter* ui_layout_system_get_constraint_layouter(UILayoutSystem* system);

/* Add constraint between elements */
void ui_layout_system_add_constraint(UILayoutSystem* system,
                                    uint32_t element1_id, ConstraintVariableType var1,
                                    uint32_t element2_id, ConstraintVariableType var2,
                                    ConstraintStrength strength);

/* Add constant constraint */
void ui_layout_system_add_constant_constraint(UILayoutSystem* system,
                                             uint32_t element_id,
                                             ConstraintVariableType var,
                                             float constant,
                                             ConstraintStrength strength);

/* ============================================================================
 * API - INVALIDATION AND CACHING
 * ============================================================================ */

/* Invalidate element layout */
void ui_layout_system_invalidate_element(UILayoutSystem* system,
                                        uint32_t element_id,
                                        uint32_t invalidation_flags);

/* Invalidate element subtree */
void ui_layout_system_invalidate_subtree(UILayoutSystem* system,
                                        uint32_t element_id);

/* Invalidate all layouts */
void ui_layout_system_invalidate_all(UILayoutSystem* system);

/* Check if element layout is cached */
bool ui_layout_system_is_cached(const UILayoutSystem* system, uint32_t element_id);

/* Clear layout cache */
void ui_layout_system_clear_cache(UILayoutSystem* system);

/* ============================================================================
 * API - PERFORMANCE MONITORING
 * ============================================================================ */

/* Get layout performance statistics */
void ui_layout_system_get_stats(const UILayoutSystem* system,
                               uint32_t* out_total_layouts,
                               float* out_avg_time_ms,
                               uint32_t* out_cache_hits,
                               uint32_t* out_cache_misses);

/* Get last layout duration */
float ui_layout_system_get_last_layout_time(const UILayoutSystem* system);

/* Get constraint solver stats */
void ui_layout_system_get_solver_stats(const UILayoutSystem* system,
                                      uint32_t* out_iterations,
                                      float* out_solve_time_ms);

/* ============================================================================
 * API - DEBUGGING
 * ============================================================================ */

/* Print system state */
void ui_layout_system_print_state(const UILayoutSystem* system);

/* Print layout for element */
void ui_layout_system_print_element_layout(const UILayoutSystem* system, uint32_t element_id);

/* Print performance report */
void ui_layout_system_print_performance_report(const UILayoutSystem* system);

#ifdef __cplusplus
}
#endif

#endif // UI_LAYOUT_SYSTEM_H

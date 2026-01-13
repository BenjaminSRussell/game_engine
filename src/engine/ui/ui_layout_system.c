/*
 * ui_layout_system.c
 * Main UI Layout System Implementation
 * Unified interface for multiple layout algorithms
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "ui_layout_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

/* ============================================================================
 * INTERNAL UTILITIES
 * ============================================================================ */

static uint32_t g_next_layout_system_id = 1;

static double get_time_ms(void) {
    /* Platform-independent timing approximation */
    static double start_time = 0.0;
    static bool initialized = false;

    if (!initialized) {
        start_time = (double)clock() / CLOCKS_PER_SEC * 1000.0;
        initialized = true;
    }

    return ((double)clock() / CLOCKS_PER_SEC * 1000.0) - start_time;
}

/* ============================================================================
 * SYSTEM MANAGEMENT API
 * ============================================================================ */

void ui_layout_system_init(UILayoutSystem* system, const LayoutConfig* config) {
    assert(system);

    memset(system, 0, sizeof(UILayoutSystem));

    system->id = g_next_layout_system_id++;
    system->initialized = true;

    /* Copy configuration */
    if (config) {
        system->config = *config;
    } else {
        system->config.engine = LAYOUT_ENGINE_CONSTRAINT;
        system->config.viewport_width = 1280.0f;
        system->config.viewport_height = 720.0f;
        system->config.enable_caching = true;
        system->config.enable_profiling = false;
        system->config.enable_debug = false;
    }

    /* Create constraint layouter */
    system->constraint_layouter = constraint_layouter_create();
    if (system->config.enable_debug) {
        constraint_layouter_enable_debug(system->constraint_layouter, true);
    }

    /* Create invalidation system */
    system->invalidation_system = layout_invalidation_create();

    /* Initialize element tracking */
    system->element_capacity = 64;
    system->element_ids = malloc(sizeof(uint32_t) * system->element_capacity);
    system->element_count = 0;

    /* Initialize performance tracking */
    system->total_layouts = 0;
    system->total_layout_time_ms = 0.0f;
    system->cache_hits = 0;
    system->cache_misses = 0;
    system->needs_full_layout = true;
    system->current_layout_frame = 0;
    system->frame_time_budget_ms = 16.67f; /* 60 FPS budget */

    if (system->config.enable_profiling) {
        printf("[UI Layout System] Initialized: %dx%d, Engine: %d\n",
               (int)system->config.viewport_width, (int)system->config.viewport_height,
               system->config.engine);
    }
}

void ui_layout_system_shutdown(UILayoutSystem* system) {
    if (!system) return;

    if (system->constraint_layouter) {
        constraint_layouter_destroy(system->constraint_layouter);
        system->constraint_layouter = NULL;
    }

    if (system->invalidation_system) {
        layout_invalidation_destroy(system->invalidation_system);
        system->invalidation_system = NULL;
    }

    if (system->element_ids) {
        free(system->element_ids);
        system->element_ids = NULL;
    }

    system->initialized = false;
}

void ui_layout_system_update(UILayoutSystem* system, float dt) {
    assert(system && system->initialized);

    system->current_layout_frame++;

    /* Process invalidation queue */
    if (system->invalidation_system) {
        InvalidationEntry entry;
        while (layout_invalidation_dequeue(system->invalidation_system, &entry)) {
            if (system->config.enable_debug) {
                printf("[Layout System] Processing invalidation for element %u: %s\n",
                       entry.element_id, entry.reason ? entry.reason : "no reason");
            }
            system->needs_full_layout = true;
        }
    }

    /* Perform layout if needed */
    if (system->needs_full_layout) {
        ui_layout_system_relayout(system);
    }
}

void ui_layout_system_relayout(UILayoutSystem* system) {
    assert(system && system->initialized);

    double start_time = get_time_ms();

    /* Solve constraints */
    if (system->constraint_layouter) {
        bool solved = constraint_layout_solve(system->constraint_layouter,
                                             system->config.viewport_width,
                                             system->config.viewport_height);

        if (!solved && system->config.enable_debug) {
            printf("[Layout System] Warning: Constraint solver did not converge\n");
        }

        /* Cache results */
        if (system->config.enable_caching && system->invalidation_system) {
            for (uint32_t i = 0; i < system->constraint_layouter->element_count; i++) {
                ConstraintElement* element = system->constraint_layouter->elements[i];
                float x, y, width, height;
                constraint_element_get_layout(element, &x, &y, &width, &height);
                layout_invalidation_cache_layout(system->invalidation_system,
                                                element->id, x, y, width, height);
            }
        }
    }

    double end_time = get_time_ms();
    float layout_duration_ms = (float)(end_time - start_time);

    /* Update performance metrics */
    system->total_layouts++;
    system->total_layout_time_ms += layout_duration_ms;
    system->avg_layout_time_ms = system->total_layout_time_ms / system->total_layouts;
    system->last_layout_duration_ms = layout_duration_ms;
    system->needs_full_layout = false;

    if (system->config.enable_profiling) {
        printf("[Layout System] Layout took %.2f ms (avg: %.2f ms)\n",
               layout_duration_ms, system->avg_layout_time_ms);
    }
}

/* ============================================================================
 * CONFIGURATION API
 * ============================================================================ */

void ui_layout_system_set_engine(UILayoutSystem* system, LayoutEngine engine) {
    assert(system && system->initialized);
    system->config.engine = engine;
}

void ui_layout_system_set_viewport(UILayoutSystem* system, float width, float height) {
    assert(system && system->initialized);

    if (system->config.viewport_width != width || system->config.viewport_height != height) {
        system->config.viewport_width = width;
        system->config.viewport_height = height;
        system->needs_full_layout = true;

        if (system->config.enable_debug) {
            printf("[Layout System] Viewport changed to %.0fx%.0f\n", width, height);
        }
    }
}

void ui_layout_system_set_caching(UILayoutSystem* system, bool enable) {
    assert(system && system->initialized);
    system->config.enable_caching = enable;

    if (!enable && system->invalidation_system) {
        layout_invalidation_cache_clear_all(system->invalidation_system);
    }
}

void ui_layout_system_set_profiling(UILayoutSystem* system, bool enable) {
    assert(system && system->initialized);
    system->config.enable_profiling = enable;
}

void ui_layout_system_set_debug(UILayoutSystem* system, bool enable) {
    assert(system && system->initialized);
    system->config.enable_debug = enable;

    if (system->constraint_layouter) {
        constraint_layouter_enable_debug(system->constraint_layouter, enable);
    }
}

/* ============================================================================
 * ELEMENT MANAGEMENT API
 * ============================================================================ */

void ui_layout_system_register_element(UILayoutSystem* system, uint32_t element_id) {
    assert(system && system->initialized);

    /* Check if already registered */
    for (uint32_t i = 0; i < system->element_count; i++) {
        if (system->element_ids[i] == element_id) {
            return;
        }
    }

    /* Add element */
    if (system->element_count >= system->element_capacity) {
        system->element_capacity *= 2;
        system->element_ids = realloc(system->element_ids,
                                     sizeof(uint32_t) * system->element_capacity);
    }

    system->element_ids[system->element_count++] = element_id;

    /* Register with sub-systems */
    if (system->invalidation_system) {
        layout_invalidation_register_element(system->invalidation_system, element_id);
    }

    system->needs_full_layout = true;

    if (system->config.enable_debug) {
        printf("[Layout System] Registered element %u\n", element_id);
    }
}

void ui_layout_system_unregister_element(UILayoutSystem* system, uint32_t element_id) {
    assert(system && system->initialized);

    /* Remove from element tracking */
    for (uint32_t i = 0; i < system->element_count; i++) {
        if (system->element_ids[i] == element_id) {
            system->element_ids[i] = system->element_ids[system->element_count - 1];
            system->element_count--;
            break;
        }
    }

    /* Unregister from sub-systems */
    if (system->invalidation_system) {
        layout_invalidation_unregister_element(system->invalidation_system, element_id);
    }

    system->needs_full_layout = true;

    if (system->config.enable_debug) {
        printf("[Layout System] Unregistered element %u\n", element_id);
    }
}

bool ui_layout_system_get_layout(const UILayoutSystem* system,
                                uint32_t element_id,
                                float* out_x, float* out_y,
                                float* out_width, float* out_height) {
    assert(system && system->initialized && out_x && out_y && out_width && out_height);

    /* Try cache first */
    if (system->config.enable_caching && system->invalidation_system) {
        if (layout_invalidation_get_cached_layout(system->invalidation_system,
                                                  element_id, out_x, out_y,
                                                  out_width, out_height)) {
            system->cache_hits++;
            return true;
        }
        system->cache_misses++;
    }

    /* Try constraint layouter */
    if (system->constraint_layouter) {
        for (uint32_t i = 0; i < system->constraint_layouter->element_count; i++) {
            ConstraintElement* element = system->constraint_layouter->elements[i];
            if (element->id == element_id) {
                constraint_element_get_layout(element, out_x, out_y, out_width, out_height);
                return true;
            }
        }
    }

    return false;
}

/* ============================================================================
 * CONSTRAINT-BASED LAYOUT API
 * ============================================================================ */

ConstraintLayouter* ui_layout_system_get_constraint_layouter(UILayoutSystem* system) {
    assert(system && system->initialized);
    return system->constraint_layouter;
}

void ui_layout_system_add_constraint(UILayoutSystem* system,
                                    uint32_t element1_id, ConstraintVariableType var1,
                                    uint32_t element2_id, ConstraintVariableType var2,
                                    ConstraintStrength strength) {
    assert(system && system->initialized && system->constraint_layouter);

    /* Find or create elements */
    ConstraintElement* elem1 = NULL;
    ConstraintElement* elem2 = NULL;

    for (uint32_t i = 0; i < system->constraint_layouter->element_count; i++) {
        if (system->constraint_layouter->elements[i]->id == element1_id) {
            elem1 = system->constraint_layouter->elements[i];
        }
        if (system->constraint_layouter->elements[i]->id == element2_id) {
            elem2 = system->constraint_layouter->elements[i];
        }
    }

    if (!elem1) {
        elem1 = constraint_element_create(system->constraint_layouter, "element1");
    }
    if (!elem2) {
        elem2 = constraint_element_create(system->constraint_layouter, "element2");
    }

    constraint_add_equal(system->constraint_layouter, elem1, var1, elem2, var2, strength);
    system->needs_full_layout = true;
}

void ui_layout_system_add_constant_constraint(UILayoutSystem* system,
                                             uint32_t element_id,
                                             ConstraintVariableType var,
                                             float constant,
                                             ConstraintStrength strength) {
    assert(system && system->initialized && system->constraint_layouter);

    /* Find or create element */
    ConstraintElement* element = NULL;

    for (uint32_t i = 0; i < system->constraint_layouter->element_count; i++) {
        if (system->constraint_layouter->elements[i]->id == element_id) {
            element = system->constraint_layouter->elements[i];
            break;
        }
    }

    if (!element) {
        element = constraint_element_create(system->constraint_layouter, "element");
    }

    constraint_add_constant_equal(system->constraint_layouter, element, var, constant, strength);
    system->needs_full_layout = true;
}

/* ============================================================================
 * INVALIDATION AND CACHING API
 * ============================================================================ */

void ui_layout_system_invalidate_element(UILayoutSystem* system,
                                        uint32_t element_id,
                                        uint32_t invalidation_flags) {
    assert(system && system->initialized);

    if (system->invalidation_system) {
        layout_invalidation_invalidate(system->invalidation_system,
                                      element_id, invalidation_flags, "system invalidation");
    }

    system->needs_full_layout = true;
}

void ui_layout_system_invalidate_subtree(UILayoutSystem* system, uint32_t element_id) {
    assert(system && system->initialized);

    if (system->invalidation_system) {
        layout_invalidation_invalidate_subtree(system->invalidation_system,
                                              element_id, INVALIDATE_ALL);
    }

    system->needs_full_layout = true;
}

void ui_layout_system_invalidate_all(UILayoutSystem* system) {
    assert(system && system->initialized);

    if (system->invalidation_system) {
        layout_invalidation_invalidate_all(system->invalidation_system);
    }

    system->needs_full_layout = true;
}

bool ui_layout_system_is_cached(const UILayoutSystem* system, uint32_t element_id) {
    assert(system && system->initialized);

    if (system->invalidation_system) {
        return layout_invalidation_is_cached(system->invalidation_system, element_id);
    }

    return false;
}

void ui_layout_system_clear_cache(UILayoutSystem* system) {
    assert(system && system->initialized);

    if (system->invalidation_system) {
        layout_invalidation_cache_clear_all(system->invalidation_system);
    }
}

/* ============================================================================
 * PERFORMANCE MONITORING API
 * ============================================================================ */

void ui_layout_system_get_stats(const UILayoutSystem* system,
                               uint32_t* out_total_layouts,
                               float* out_avg_time_ms,
                               uint32_t* out_cache_hits,
                               uint32_t* out_cache_misses) {
    assert(system && system->initialized);
    assert(out_total_layouts && out_avg_time_ms && out_cache_hits && out_cache_misses);

    *out_total_layouts = system->total_layouts;
    *out_avg_time_ms = system->avg_layout_time_ms;
    *out_cache_hits = system->cache_hits;
    *out_cache_misses = system->cache_misses;
}

float ui_layout_system_get_last_layout_time(const UILayoutSystem* system) {
    assert(system && system->initialized);
    return system->last_layout_duration_ms;
}

void ui_layout_system_get_solver_stats(const UILayoutSystem* system,
                                      uint32_t* out_iterations,
                                      float* out_solve_time_ms) {
    assert(system && system->initialized && out_iterations && out_solve_time_ms);

    if (system->constraint_layouter) {
        constraint_layouter_get_stats(system->constraint_layouter, out_iterations, out_solve_time_ms);
    } else {
        *out_iterations = 0;
        *out_solve_time_ms = 0.0f;
    }
}

/* ============================================================================
 * DEBUGGING API
 * ============================================================================ */

void ui_layout_system_print_state(const UILayoutSystem* system) {
    if (!system || !system->initialized) return;

    printf("=== UI Layout System State ===\n");
    printf("System ID: %u\n", system->id);
    printf("Engine: %d\n", system->config.engine);
    printf("Viewport: %.0fx%.0f\n", system->config.viewport_width, system->config.viewport_height);
    printf("Elements: %u\n", system->element_count);
    printf("Current frame: %u\n", system->current_layout_frame);
    printf("Needs layout: %s\n", system->needs_full_layout ? "yes" : "no");
    printf("Total layouts: %u\n", system->total_layouts);
    printf("Avg layout time: %.2f ms\n", system->avg_layout_time_ms);
    printf("Cache hits: %u\n", system->cache_hits);
    printf("Cache misses: %u\n", system->cache_misses);

    if (system->cache_hits + system->cache_misses > 0) {
        float hit_rate = (float)system->cache_hits / (float)(system->cache_hits + system->cache_misses);
        printf("Cache hit rate: %.2f%%\n", hit_rate * 100.0f);
    }

    if (system->constraint_layouter) {
        printf("\nConstraint Solver:\n");
        constraint_layouter_print_state(system->constraint_layouter);
    }
}

void ui_layout_system_print_element_layout(const UILayoutSystem* system, uint32_t element_id) {
    if (!system || !system->initialized) return;

    float x, y, width, height;
    if (ui_layout_system_get_layout(system, element_id, &x, &y, &width, &height)) {
        printf("Element %u: (%.2f, %.2f) %.2fx%.2f\n", element_id, x, y, width, height);
    } else {
        printf("Element %u: not found in layout\n", element_id);
    }
}

void ui_layout_system_print_performance_report(const UILayoutSystem* system) {
    if (!system || !system->initialized) return;

    printf("=== Layout System Performance Report ===\n");
    printf("Total layouts: %u\n", system->total_layouts);
    printf("Total time: %.2f ms\n", system->total_layout_time_ms);
    printf("Average time: %.2f ms\n", system->avg_layout_time_ms);
    printf("Last layout: %.2f ms\n", system->last_layout_duration_ms);
    printf("Frame budget: %.2f ms\n", system->frame_time_budget_ms);

    uint32_t total_requests = system->cache_hits + system->cache_misses;
    if (total_requests > 0) {
        float hit_rate = (float)system->cache_hits / (float)total_requests;
        printf("Cache hit rate: %.2f%% (%u hits, %u misses)\n",
               hit_rate * 100.0f, system->cache_hits, system->cache_misses);
    }

    uint32_t iterations;
    float solve_time;
    ui_layout_system_get_solver_stats(system, &iterations, &solve_time);
    printf("Solver iterations: %u\n", iterations);
    printf("Solver time: %.2f ms\n", solve_time);
}

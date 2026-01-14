/*
 * ui_layout_integration_test.c
 * Integration tests for UI Layout System (Flexbox, Grid, Solver)
 */

#include "ui/layout/layout_solver.h"
#include "ui/layout/flexbox_layout.h"
#include "ui/layout/grid_layout.h"
#include "ui/ui_types.h"
#include "core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#define FLOAT_EPSILON 0.01f

static bool float_eq(float a, float b) {
    return fabsf(a - b) < FLOAT_EPSILON;
}

#define ASSERT_FLOAT_EQ(a, b) do { \
    if (!float_eq(a, b)) { \
        printf("Assertion failed: %s (%.2f) != %.2f at line %d\n", #a, (float)(a), (float)(b), __LINE__); \
        exit(1); \
    } \
} while(0)

static void test_flexbox_wrapping() {
    printf("Testing Flexbox Wrapping...\n");

    // Create container 200x200
    FlexboxContainer* container = flexbox_container_create("root");
    container->config.direction = FLEX_DIRECTION_ROW;
    container->config.wrap = FLEX_WRAP_WRAP;
    container->config.align_content = ALIGN_CONTENT_FLEX_START;

    // Create 3 children, 100x50 each
    for (int i = 0; i < 3; i++) {
        char name[32];
        snprintf(name, 32, "child_%d", i);
        UIElement* child = ui_element_create(name);
        child->preferred_size = (Size){100.0f, 50.0f};
        flexbox_add_child(container, child);
    }

    // Solve layout
    LayoutSolver* solver = layout_solver_create();
    layout_solver_solve(solver, (UIElement*)container, 200.0f, 200.0f);

    UIElement* c0 = container->base.children[0];
    UIElement* c1 = container->base.children[1];
    UIElement* c2 = container->base.children[2];

    // Line 1
    ASSERT_FLOAT_EQ(c0->layout.position.x, 0.0f);
    ASSERT_FLOAT_EQ(c0->layout.position.y, 0.0f);

    ASSERT_FLOAT_EQ(c1->layout.position.x, 100.0f);
    ASSERT_FLOAT_EQ(c1->layout.position.y, 0.0f);

    // Line 2 (Wrapped)
    ASSERT_FLOAT_EQ(c2->layout.position.x, 0.0f);
    ASSERT_FLOAT_EQ(c2->layout.position.y, 50.0f);

    printf("  PASSED\n");

    layout_solver_destroy(solver);
    flexbox_container_destroy(container);
}

static void test_grid_placement() {
    printf("Testing Grid Placement...\n");

    GridContainer* container = grid_container_create("grid");

    GridTrack cols[] = { grid_track_fixed(100.0f), grid_track_fixed(100.0f) };
    grid_set_columns(container, cols, 2);

    GridTrack rows[] = { grid_track_fixed(100.0f), grid_track_fixed(100.0f) };
    grid_set_rows(container, rows, 2);

    // Child 1: Col 0, Row 0
    UIElement* c1 = ui_element_create("c1");
    grid_add_child(container, c1);
    grid_item_set_column(container, c1, 0);
    grid_item_set_row(container, c1, 0);

    // Child 2: Col 1, Row 1
    UIElement* c2 = ui_element_create("c2");
    grid_add_child(container, c2);
    grid_item_set_column(container, c2, 1);
    grid_item_set_row(container, c2, 1);

    // Child 3: Auto placement (expect 1,0)
    UIElement* c3 = ui_element_create("c3");
    grid_add_child(container, c3);

    LayoutSolver* solver = layout_solver_create();
    layout_solver_solve(solver, (UIElement*)container, 200.0f, 200.0f);

    ASSERT_FLOAT_EQ(c1->layout.position.x, 0.0f);
    ASSERT_FLOAT_EQ(c1->layout.position.y, 0.0f);
    ASSERT_FLOAT_EQ(c1->layout.size.width, 100.0f);

    ASSERT_FLOAT_EQ(c2->layout.position.x, 100.0f);
    ASSERT_FLOAT_EQ(c2->layout.position.y, 100.0f);

    ASSERT_FLOAT_EQ(c3->layout.position.x, 100.0f);
    ASSERT_FLOAT_EQ(c3->layout.position.y, 0.0f);

    printf("  PASSED\n");

    layout_solver_destroy(solver);
    grid_container_destroy(container);
}

static void test_solver_recursion() {
    printf("Testing Solver Recursion (Grid in Flex)...\n");

    FlexboxContainer* root = flexbox_container_create("root");

    UIElement* c1 = ui_element_create("fixed");
    c1->preferred_size = (Size){100.0f, 200.0f};
    flexbox_add_child(root, c1);

    GridContainer* grid = grid_container_create("nested_grid");
    ui_element_set_flex_grow((UIElement*)grid, 1.0f);

    GridTrack cols[] = { grid_track_fraction(1.0f), grid_track_fraction(1.0f) };
    grid_set_columns(grid, cols, 2);

    GridTrack rows[] = { grid_track_fraction(1.0f) };
    grid_set_rows(grid, rows, 1);

    UIElement* gc1 = ui_element_create("gc1");
    grid_add_child(grid, gc1);

    UIElement* gc2 = ui_element_create("gc2");
    grid_add_child(grid, gc2);

    flexbox_add_child(root, (UIElement*)grid);

    LayoutSolver* solver = layout_solver_create();
    layout_solver_solve(solver, (UIElement*)root, 400.0f, 200.0f);

    ASSERT_FLOAT_EQ(c1->layout.position.x, 0.0f);
    ASSERT_FLOAT_EQ(c1->layout.size.width, 100.0f);

    ASSERT_FLOAT_EQ(((UIElement*)grid)->layout.position.x, 100.0f);
    ASSERT_FLOAT_EQ(((UIElement*)grid)->layout.size.width, 300.0f);

    ASSERT_FLOAT_EQ(gc1->layout.position.x, 0.0f);
    ASSERT_FLOAT_EQ(gc1->layout.size.width, 150.0f);

    ASSERT_FLOAT_EQ(gc2->layout.position.x, 150.0f);
    ASSERT_FLOAT_EQ(gc2->layout.size.width, 150.0f);

    printf("  PASSED\n");

    layout_solver_destroy(solver);
    flexbox_container_destroy(root);
}

int main() {
    printf("=== UI Layout Integration Tests ===\n");
    test_flexbox_wrapping();
    test_grid_placement();
    test_solver_recursion();
    printf("All tests passed!\n");
    return 0;
}

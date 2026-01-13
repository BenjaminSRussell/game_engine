/*
 * ui_layout_engine_test.c
 * Comprehensive test suite for UI Layout Engine
 * Tests measure/arrange passes, box model, anchors, and responsive design
 *
 * Part of the UI subsystem
 * Advanced 3D Rendering Engine
 */

#include "../src/engine/ui/core/ui_layout_engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Test utilities */
#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      printf("❌ FAILED: %s\n", message);                                      \
      printf("   at %s:%d\n", __FILE__, __LINE__);                             \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define FLOAT_EQUAL(a, b) (fabsf((a) - (b)) < 0.01f)

static bool g_all_tests_passed = true;

static void run_test(const char *name, bool (*test_func)(void)) {
  printf("Running: %s... ", name);
  fflush(stdout);

  bool result = test_func();
  if (result) {
    printf("✓ PASSED\n");
  } else {
    printf("❌ FAILED\n");
    g_all_tests_passed = false;
  }
}

/* ============================================================================
 * MEASURE PASS TESTS
 * ============================================================================
 */

bool test_measure_single_element(void) {
  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  TEST_ASSERT(engine != NULL, "Engine creation failed");

  UILayoutElement *element = ui_layout_element_create("test_element");
  TEST_ASSERT(element != NULL, "Element creation failed");

  element->preferred_width = 100.0f;
  element->preferred_height = 50.0f;

  ui_layout_measure(engine, element, 800.0f, 600.0f);

  TEST_ASSERT(FLOAT_EQUAL(element->measure_result.desired_width, 100.0f),
              "Desired width mismatch");
  TEST_ASSERT(FLOAT_EQUAL(element->measure_result.desired_height, 50.0f),
              "Desired height mismatch");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

bool test_measure_with_children(void) {
  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);

  UILayoutElement *parent = ui_layout_element_create("parent");
  UILayoutElement *child1 = ui_layout_element_create("child1");
  UILayoutElement *child2 = ui_layout_element_create("child2");

  child1->preferred_width = 150.0f;
  child1->preferred_height = 75.0f;
  child2->preferred_width = 200.0f;
  child2->preferred_height = 100.0f;

  ui_layout_element_add_child(parent, child1);
  ui_layout_element_add_child(parent, child2);

  ui_layout_measure(engine, parent, 800.0f, 600.0f);

  /* Parent should be sized to contain children (vertical stack) */
  TEST_ASSERT(parent->measure_result.desired_width >= 200.0f,
              "Parent width too small for children");
  TEST_ASSERT(parent->measure_result.desired_height >= 175.0f,
              "Parent height too small for children");

  ui_layout_element_destroy(parent);
  ui_layout_engine_destroy(engine);
  return true;
}

bool test_measure_cache(void) {
  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  ui_layout_engine_set_caching(engine, true);

  UILayoutElement *element = ui_layout_element_create("cached_element");
  element->preferred_width = 100.0f;
  element->preferred_height = 50.0f;

  /* First measure - should be a cache miss */
  ui_layout_measure(engine, element, 800.0f, 600.0f);

  uint32_t measure_passes_1 = engine->total_measure_passes;

  /* Second measure - should use cache */
  ui_layout_measure(engine, element, 800.0f, 600.0f);

  uint32_t measure_passes_2 = engine->total_measure_passes;

  TEST_ASSERT(measure_passes_2 == measure_passes_1,
              "Measure cache not working - measure ran again");
  TEST_ASSERT(engine->measure_cache_hits > 0, "No cache hits recorded");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

/* ============================================================================
 * ARRANGE PASS TESTS
 * ============================================================================
 */

bool test_arrange_single_element(void) {
  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);

  UILayoutElement *element = ui_layout_element_create("test_element");
  element->preferred_width = 200.0f;
  element->preferred_height = 100.0f;

  ui_layout_measure(engine, element, 800.0f, 600.0f);

  UIRect final_rect = {10.0f, 20.0f, 200.0f, 100.0f};
  ui_layout_arrange(engine, element, final_rect);

  TEST_ASSERT(FLOAT_EQUAL(element->arrange_result.final_rect.x, 10.0f),
              "Final x position incorrect");
  TEST_ASSERT(FLOAT_EQUAL(element->arrange_result.final_rect.y, 20.0f),
              "Final y position incorrect");
  TEST_ASSERT(element->arrange_result.is_visible, "Element should be visible");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

bool test_arrange_with_margins(void) {
  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);

  UILayoutElement *element = ui_layout_element_create("element_with_margins");
  element->preferred_width = 200.0f;
  element->preferred_height = 100.0f;
  element->margin.top = 10.0f;
  element->margin.right = 15.0f;
  element->margin.bottom = 10.0f;
  element->margin.left = 15.0f;

  ui_layout_measure(engine, element, 800.0f, 600.0f);

  UIRect final_rect = {0.0f, 0.0f, 230.0f, 120.0f};
  ui_layout_arrange(engine, element, final_rect);

  /* Content rect should exclude margins */
  UIRect content_rect = element->arrange_result.content_rect;
  TEST_ASSERT(FLOAT_EQUAL(content_rect.x, 15.0f),
              "Content left margin not applied");
  TEST_ASSERT(FLOAT_EQUAL(content_rect.y, 10.0f),
              "Content top margin not applied");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

/* ============================================================================
 * BOX MODEL TESTS (TODO-0180)
 * ============================================================================
 */

bool test_content_box_sizing(void) {
  UILayoutElement *element = ui_layout_element_create("content_box");
  element->box_sizing = BOX_SIZING_CONTENT_BOX;
  element->padding.left = 10.0f;
  element->padding.right = 10.0f;
  element->padding.top = 5.0f;
  element->padding.bottom = 5.0f;
  element->border.left = 2.0f;
  element->border.right = 2.0f;
  element->border.top = 2.0f;
  element->border.bottom = 2.0f;

  UISize content = {100.0f, 50.0f};
  UISize border_box = ui_calculate_border_box_size(element, content);

  TEST_ASSERT(FLOAT_EQUAL(border_box.width, 124.0f),
              "Border box width incorrect (should be 100 + 20 + 4)");
  TEST_ASSERT(FLOAT_EQUAL(border_box.height, 64.0f),
              "Border box height incorrect (should be 50 + 10 + 4)");

  ui_layout_element_destroy(element);
  return true;
}

bool test_border_box_sizing(void) {
  UILayoutElement *element = ui_layout_element_create("border_box");
  element->box_sizing = BOX_SIZING_BORDER_BOX;
  element->padding.left = 10.0f;
  element->padding.right = 10.0f;
  element->padding.top = 5.0f;
  element->padding.bottom = 5.0f;
  element->border.left = 2.0f;
  element->border.right = 2.0f;
  element->border.top = 2.0f;
  element->border.bottom = 2.0f;

  UISize outer = {124.0f, 64.0f};
  UISize content = ui_calculate_content_size(element, outer);

  TEST_ASSERT(FLOAT_EQUAL(content.width, 100.0f), "Content width incorrect");
  TEST_ASSERT(FLOAT_EQUAL(content.height, 50.0f), "Content height incorrect");

  ui_layout_element_destroy(element);
  return true;
}

bool test_spacing_calculations(void) {
  UILayoutElement *element = ui_layout_element_create("spacing_test");
  element->margin.left = 5.0f;
  element->margin.right = 5.0f;
  element->margin.top = 3.0f;
  element->margin.bottom = 3.0f;
  element->padding.left = 10.0f;
  element->padding.right = 10.0f;
  element->padding.top = 8.0f;
  element->padding.bottom = 8.0f;
  element->border.left = 2.0f;
  element->border.right = 2.0f;
  element->border.top = 1.0f;
  element->border.bottom = 1.0f;

  float h_spacing = ui_get_horizontal_spacing(element);
  float v_spacing = ui_get_vertical_spacing(element);

  TEST_ASSERT(FLOAT_EQUAL(h_spacing, 34.0f),
              "Horizontal spacing incorrect (should be 5+5+10+10+2+2)");
  TEST_ASSERT(FLOAT_EQUAL(v_spacing, 24.0f),
              "Vertical spacing incorrect (should be 3+3+8+8+1+1)");

  ui_layout_element_destroy(element);
  return true;
}

/* ============================================================================
 * ANCHOR TESTS (TODO-0181)
 * ============================================================================
 */

bool test_anchor_top_left(void) {
  UILayoutElement *element = ui_layout_element_create("anchor_top_left");
  element->preferred_width = 100.0f;
  element->preferred_height = 50.0f;

  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  ui_layout_measure(engine, element, 800.0f, 600.0f);

  ui_element_set_anchor_preset(element, ANCHOR_TOP_LEFT);

  UIRect parent_rect = {0.0f, 0.0f, 400.0f, 300.0f};
  UIRect result = ui_element_apply_anchor_layout(element, parent_rect);

  TEST_ASSERT(FLOAT_EQUAL(result.x, 0.0f), "Top-left anchor x incorrect");
  TEST_ASSERT(FLOAT_EQUAL(result.y, 0.0f), "Top-left anchor y incorrect");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

bool test_anchor_center(void) {
  UILayoutElement *element = ui_layout_element_create("anchor_center");
  element->preferred_width = 100.0f;
  element->preferred_height = 50.0f;

  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  ui_layout_measure(engine, element, 800.0f, 600.0f);

  ui_element_set_anchor_preset(element, ANCHOR_MIDDLE_CENTER);

  UIRect parent_rect = {0.0f, 0.0f, 400.0f, 300.0f};
  UIRect result = ui_element_apply_anchor_layout(element, parent_rect);

  /* Center should be at 200, 150 (middle of 400x300 rect) */
  TEST_ASSERT(FLOAT_EQUAL(result.x, 150.0f) || FLOAT_EQUAL(result.x, 200.0f),
              "Center anchor x should be near center");
  TEST_ASSERT(FLOAT_EQUAL(result.y, 125.0f) || FLOAT_EQUAL(result.y, 150.0f),
              "Center anchor y should be near center");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

bool test_anchor_stretch_horizontal(void) {
  UILayoutElement *element = ui_layout_element_create("stretch_h");
  element->preferred_width = 100.0f;
  element->preferred_height = 50.0f;

  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  ui_layout_measure(engine, element, 800.0f, 600.0f);

  ui_element_set_anchor_preset(element, ANCHOR_STRETCH_HORIZONTAL);

  UIRect parent_rect = {0.0f, 0.0f, 400.0f, 300.0f};
  UIRect result = ui_element_apply_anchor_layout(element, parent_rect);

  TEST_ASSERT(FLOAT_EQUAL(result.width, 400.0f),
              "Horizontal stretch width should match parent");
  TEST_ASSERT(FLOAT_EQUAL(result.height, 50.0f),
              "Horizontal stretch height should be preferred");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

bool test_anchor_stretch_both(void) {
  UILayoutElement *element = ui_layout_element_create("stretch_both");
  element->preferred_width = 100.0f;
  element->preferred_height = 50.0f;

  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  ui_layout_measure(engine, element, 800.0f, 600.0f);

  ui_element_set_anchor_preset(element, ANCHOR_STRETCH_BOTH);

  UIRect parent_rect = {0.0f, 0.0f, 400.0f, 300.0f};
  UIRect result = ui_element_apply_anchor_layout(element, parent_rect);

  TEST_ASSERT(FLOAT_EQUAL(result.width, 400.0f),
              "Stretch both width should match parent");
  TEST_ASSERT(FLOAT_EQUAL(result.height, 300.0f),
              "Stretch both height should match parent");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

/* ============================================================================
 * DOCK TESTS (TODO-0181)
 * ============================================================================
 */

bool test_dock_fill(void) {
  UILayoutElement *element = ui_layout_element_create("dock_fill");
  element->preferred_width = 100.0f;
  element->preferred_height = 50.0f;

  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  ui_layout_measure(engine, element, 800.0f, 600.0f);

  ui_element_set_dock(element, DOCK_FILL);

  UIRect parent_rect = {0.0f, 0.0f, 400.0f, 300.0f};
  UIRect result = ui_element_apply_dock_layout(element, parent_rect);

  TEST_ASSERT(FLOAT_EQUAL(result.width, 400.0f),
              "Dock fill width should match parent");
  TEST_ASSERT(FLOAT_EQUAL(result.height, 300.0f),
              "Dock fill height should match parent");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

bool test_dock_top(void) {
  UILayoutElement *element = ui_layout_element_create("dock_top");
  element->preferred_width = 100.0f;
  element->preferred_height = 50.0f;

  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  ui_layout_measure(engine, element, 800.0f, 600.0f);

  ui_element_set_dock(element, DOCK_TOP);

  UIRect parent_rect = {0.0f, 0.0f, 400.0f, 300.0f};
  UIRect result = ui_element_apply_dock_layout(element, parent_rect);

  TEST_ASSERT(FLOAT_EQUAL(result.y, 0.0f),
              "Dock top y should be at parent top");
  TEST_ASSERT(FLOAT_EQUAL(result.height, 50.0f),
              "Dock top height should be preferred");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

bool test_dock_bottom(void) {
  UILayoutElement *element = ui_layout_element_create("dock_bottom");
  element->preferred_width = 100.0f;
  element->preferred_height = 50.0f;

  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  ui_layout_measure(engine, element, 800.0f, 600.0f);

  ui_element_set_dock(element, DOCK_BOTTOM);

  UIRect parent_rect = {0.0f, 0.0f, 400.0f, 300.0f};
  UIRect result = ui_element_apply_dock_layout(element, parent_rect);

  TEST_ASSERT(FLOAT_EQUAL(result.y, 250.0f),
              "Dock bottom y should be at parent bottom minus height");
  TEST_ASSERT(FLOAT_EQUAL(result.height, 50.0f),
              "Dock bottom height should be preferred");

  ui_layout_element_destroy(element);
  ui_layout_engine_destroy(engine);
  return true;
}

/* ============================================================================
 * RESPONSIVE DESIGN TESTS (TODO-0182)
 * ============================================================================
 */

bool test_breakpoint_system(void) {
  UILayoutEngine *engine = ui_layout_engine_create(500.0f, 600.0f);

  /* Add mobile breakpoint */
  UIBreakpoint mobile = {.type = BREAKPOINT_MOBILE,
                         .min_width = 0.0f,
                         .max_width = 600.0f,
                         .scale_factor = 1.0f};
  strcpy(mobile.name, "Mobile");
  ui_layout_engine_add_breakpoint(engine, mobile);

  /* Add desktop breakpoint */
  UIBreakpoint desktop = {.type = BREAKPOINT_DESKTOP,
                          .min_width = 1024.0f,
                          .max_width = 1920.0f,
                          .scale_factor = 1.0f};
  strcpy(desktop.name, "Desktop");
  ui_layout_engine_add_breakpoint(engine, desktop);

  /* At 500px width, should be mobile */
  ui_layout_engine_update_breakpoint(engine);
  UIBreakpoint *active = ui_layout_engine_get_active_breakpoint(engine);

  TEST_ASSERT(active != NULL, "No active breakpoint");
  TEST_ASSERT(active->type == BREAKPOINT_MOBILE,
              "Should be mobile breakpoint at 500px");

  /* Change to desktop width */
  ui_layout_engine_set_viewport(engine, 1280.0f, 720.0f);
  active = ui_layout_engine_get_active_breakpoint(engine);

  TEST_ASSERT(active->type == BREAKPOINT_DESKTOP,
              "Should be desktop breakpoint at 1280px");

  ui_layout_engine_destroy(engine);
  return true;
}

bool test_dpi_scaling(void) {
  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);
  ui_layout_engine_set_dpi_scale(engine, 2.0f);

  TEST_ASSERT(FLOAT_EQUAL(engine->viewport_dpi_scale, 2.0f),
              "DPI scale not set correctly");

  ui_layout_engine_destroy(engine);
  return true;
}

/* ============================================================================
 * INTEGRATION TESTS
 * ============================================================================
 */

bool test_complete_layout_cycle(void) {
  UILayoutEngine *engine = ui_layout_engine_create(800.0f, 600.0f);

  /* Create a simple UI hierarchy */
  UILayoutElement *root = ui_layout_element_create("root");
  root->preferred_width = 400.0f;
  root->preferred_height = 300.0f;
  root->padding.left = 10.0f;
  root->padding.right = 10.0f;
  root->padding.top = 10.0f;
  root->padding.bottom = 10.0f;

  UILayoutElement *child = ui_layout_element_create("child");
  child->preferred_width = 100.0f;
  child->preferred_height = 50.0f;
  child->margin.left = 5.0f;
  child->margin.top = 5.0f;

  ui_layout_element_add_child(root, child);

  /* Measure pass */
  ui_layout_measure(engine, root, 800.0f, 600.0f);

  TEST_ASSERT(root->measure_result.desired_width > 0.0f,
              "Root should have desired width");
  TEST_ASSERT(child->measure_result.desired_width == 100.0f,
              "Child desired width incorrect");

  /* Arrange pass */
  UIRect root_rect = {0.0f, 0.0f, 400.0f, 300.0f};
  ui_layout_arrange(engine, root, root_rect);

  TEST_ASSERT(root->arrange_result.is_visible, "Root should be visible");
  TEST_ASSERT(!root->needs_measure && !root->needs_arrange,
              "Root should not need layout after arrange");

  ui_layout_element_destroy(root);
  ui_layout_engine_destroy(engine);
  return true;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================
 */

int main(void) {
  printf("\n");
  printf("╔═══════════════════════════════════════════════════════════╗\n");
  printf("║        UI Layout Engine Test Suite                       ║\n");
  printf("║    Testing TODO-0179, 0180, 0181, 0182                   ║\n");
  printf("╚═══════════════════════════════════════════════════════════╝\n\n");

  printf("=== MEASURE PASS TESTS ===\n");
  run_test("Measure single element", test_measure_single_element);
  run_test("Measure with children", test_measure_with_children);
  run_test("Measure caching", test_measure_cache);

  printf("\n=== ARRANGE PASS TESTS ===\n");
  run_test("Arrange single element", test_arrange_single_element);
  run_test("Arrange with margins", test_arrange_with_margins);

  printf("\n=== BOX MODEL TESTS (TODO-0180) ===\n");
  run_test("Content box sizing", test_content_box_sizing);
  run_test("Border box sizing", test_border_box_sizing);
  run_test("Spacing calculations", test_spacing_calculations);

  printf("\n=== ANCHOR TESTS (TODO-0181) ===\n");
  run_test("Anchor top-left", test_anchor_top_left);
  run_test("Anchor center", test_anchor_center);
  run_test("Anchor stretch horizontal", test_anchor_stretch_horizontal);
  run_test("Anchor stretch both", test_anchor_stretch_both);

  printf("\n=== DOCK TESTS (TODO-0181) ===\n");
  run_test("Dock fill", test_dock_fill);
  run_test("Dock top", test_dock_top);
  run_test("Dock bottom", test_dock_bottom);

  printf("\n=== RESPONSIVE DESIGN TESTS (TODO-0182) ===\n");
  run_test("Breakpoint system", test_breakpoint_system);
  run_test("DPI scaling", test_dpi_scaling);

  printf("\n=== INTEGRATION TESTS ===\n");
  run_test("Complete layout cycle", test_complete_layout_cycle);

  printf("\n");
  printf("═══════════════════════════════════════════════════════════\n");
  if (g_all_tests_passed) {
    printf("✓ ALL TESTS PASSED\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    return 0;
  } else {
    printf("❌ SOME TESTS FAILED\n");
    printf("═══════════════════════════════════════════════════════════\n\n");
    return 1;
  }
}

/**
 * Unit Test for Utility AI System
 * Tests utility-based decision making for NPCs
 */

#include "../../../../src/engine/ai/npc_advanced/utility_ai.h"
#include "../../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test utility AI initialization
static TestResult test_utility_ai_init(void) {
    UtilityAIContext* ctx = utility_ai_create();
    TEST_ASSERT_NOT_NULL(ctx, "Utility AI context created");
    utility_ai_destroy(ctx);
    return TEST_PASS;
}

// Test adding considerations
static TestResult test_add_consideration(void) {
    UtilityAIContext* ctx = utility_ai_create();
    
    // Add a consideration (e.g., "How hungry am I?")
    ConsiderationConfig hunger_config = {
        .name = "Hunger",
        .curve_type = CURVE_LINEAR,
        .weight = 1.0f
    };
    
    uint32_t consideration_id = utility_ai_add_consideration(ctx, &hunger_config);
    TEST_ASSERT(consideration_id > 0, "Consideration added successfully");
    
    utility_ai_destroy(ctx);
    return TEST_PASS;
}

// Test action scoring
static TestResult test_action_scoring(void) {
    UtilityAIContext* ctx = utility_ai_create();
    
    // Create action: "Eat Food"
    ActionConfig eat_config = {
        .name = "Eat Food",
        .base_score = 0.5f
    };
    
    uint32_t action_id = utility_ai_add_action(ctx, &eat_config);
    TEST_ASSERT(action_id > 0, "Action added");
    
    // Add consideration to action
    ConsiderationConfig hunger = {
        .name = "Hunger",
        .curve_type = CURVE_LINEAR,
        .weight = 1.0f
    };
    
    uint32_t consideration_id = utility_ai_add_consideration(ctx, &hunger);
    utility_ai_link_consideration_to_action(ctx, action_id, consideration_id);
    
    // Set hunger value
    utility_ai_set_input(ctx, "Hunger", 0.8f);  // Very hungry
    
    // Calculate score
    float score = utility_ai_evaluate_action(ctx, action_id);
    TEST_ASSERT(score > 0.0f, "Action should have positive score");
    TEST_ASSERT(score > 0.3f, "High hunger should give decent score");
    
    utility_ai_destroy(ctx);
    return TEST_PASS;
}

// Test best action selection
static TestResult test_best_action_selection(void) {
    UtilityAIContext* ctx = utility_ai_create();
    
    // Add two actions: Eat and Sleep
    ActionConfig eat_config = {.name = "Eat", .base_score = 0.5f};
    ActionConfig sleep_config = {.name = "Sleep", .base_score = 0.5f};
    
    uint32_t eat_id = utility_ai_add_action(ctx, &eat_config);
    uint32_t sleep_id = utility_ai_add_action(ctx, &sleep_config);
    
    // Add considerations
    ConsiderationConfig hunger = {.name = "Hunger", .curve_type = CURVE_LINEAR, .weight = 1.0f};
    ConsiderationConfig tiredness = {.name = "Tiredness", .curve_type = CURVE_LINEAR, .weight = 1.0f};
    
    uint32_t hunger_id = utility_ai_add_consideration(ctx, &hunger);
    uint32_t tired_id = utility_ai_add_consideration(ctx, &tiredness);
    
    // Link considerations
    utility_ai_link_consideration_to_action(ctx, eat_id, hunger_id);
    utility_ai_link_consideration_to_action(ctx, sleep_id, tired_id);
    
    // Set values: very tired, slightly hungry
    utility_ai_set_input(ctx, "Hunger", 0.3f);
    utility_ai_set_input(ctx, "Tiredness", 0.9f);
    
    // Get best action
    uint32_t best_action = utility_ai_select_best_action(ctx);
    TEST_ASSERT_EQ(best_action, sleep_id, "Should choose sleep when very tired");
    
    utility_ai_destroy(ctx);
    return TEST_PASS;
}

// Test curve types
static TestResult test_curve_types(void) {
    UtilityAIContext* ctx = utility_ai_create();
    
    // Test different curve types
    ConsiderationConfig linear = {.name = "Linear", .curve_type = CURVE_LINEAR, .weight = 1.0f};
    ConsiderationConfig quadratic = {.name = "Quadratic", .curve_type = CURVE_QUADRATIC, .weight = 1.0f};
    ConsiderationConfig sigmoid = {.name = "Sigmoid", .curve_type = CURVE_SIGMOID, .weight = 1.0f};
    
    uint32_t linear_id = utility_ai_add_consideration(ctx, &linear);
    uint32_t quad_id = utility_ai_add_consideration(ctx, &quadratic);
    uint32_t sig_id = utility_ai_add_consideration(ctx, &sigmoid);
    
    TEST_ASSERT(linear_id > 0, "Linear curve added");
    TEST_ASSERT(quad_id > 0, "Quadratic curve added");
    TEST_ASSERT(sig_id > 0, "Sigmoid curve added");
    
    utility_ai_destroy(ctx);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("AI:UtilityAI", "Initialization", test_utility_ai_init, NULL, NULL);
    test_register("AI:UtilityAI", "Add Consideration", test_add_consideration, NULL, NULL);
    test_register("AI:UtilityAI", "Action Scoring", test_action_scoring, NULL, NULL);
    test_register("AI:UtilityAI", "Best Action Selection", test_best_action_selection, NULL, NULL);
    test_register("AI:UtilityAI", "Curve Types", test_curve_types, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Utility AI System Test Results\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  Total:   %u\n", stats.total);
    printf("  Passed:  %u\n", stats.passed);
    printf("  Failed:  %u\n", stats.failed);
    printf("  Skipped: %u\n", stats.skipped);
    printf("  Time:    %.2f ms\n", stats.duration_ms);
    printf("════════════════════════════════════════════════════════\n");
    
    test_cleanup();
    
    return stats.failed > 0 ? 1 : 0;
}

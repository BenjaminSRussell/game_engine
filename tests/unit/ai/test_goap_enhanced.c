/**
 * Unit Test for GOAP Enhanced System  
 * Tests Goal-Oriented Action Planning for NPCs
 */

#include "../../../../src/engine/ai/npc_advanced/goap.h"
#include "../../../../tests/test_framework_unified.h"
#include <string.h>
#include <stdio.h>

// Test GOAP planner initialization
static TestResult test_goap_init(void) {
    GOAPPlanner* planner = goap_planner_create();
    TEST_ASSERT_NOT_NULL(planner, "GOAP planner created");
    goap_planner_destroy(planner);
    return TEST_PASS;
}

// Test adding actions
static TestResult test_add_action(void) {
    GOAPPlanner* planner = goap_planner_create();
    
    GOAPAction chop_wood = {
        .name = "Chop Wood",
        .cost = 4.0f,
        .preconditions = {{"has_axe", true}},
        .effects = {{"has_wood", true}},
        .precondition_count = 1,
        .effect_count = 1
    };
    
    bool success = goap_add_action(planner, &chop_wood);
    TEST_ASSERT(success, "Action added successfully");
    
    goap_planner_destroy(planner);
    return TEST_PASS;
}

// Test plan generation
static TestResult test_plan_generation(void) {
    GOAPPlanner* planner = goap_planner_create();
    
    // Add actions
    GOAPAction get_axe = {
        .name = "Get Axe",
        .cost = 2.0f,
        .preconditions = {},
        .effects = {{"has_axe", true}},
        .precondition_count = 0,
        .effect_count = 1
    };
    
    GOAPAction chop_wood = {
        .name = "Chop Wood",
        .cost = 4.0f,
        .preconditions = {{"has_axe", true}},
        .effects = {{"has_wood", true}},
        .precondition_count = 1,
        .effect_count = 1
    };
    
    goap_add_action(planner, &get_axe);
    goap_add_action(planner, &chop_wood);
    
    // Set world state
    GOAPWorldState world = {
        .facts = {},
        .fact_count = 0
    };
    
    // Set goal
    GOAPGoal goal = {
        .desired_state = {{"has_wood", true}},
        .condition_count = 1
    };
    
    // Generate plan
    GOAPPlan* plan = goap_create_plan(planner, &world, &goal);
    TEST_ASSERT_NOT_NULL(plan, "Plan created");
    TEST_ASSERT(plan->action_count >= 2, "Plan should have at least 2 actions");
    
    goap_destroy_plan(plan);
    goap_planner_destroy(planner);
    return TEST_PASS;
}

// Test impossible goal
static TestResult test_impossible_goal(void) {
    GOAPPlanner* planner = goap_planner_create();
    
    GOAPAction action = {
        .name = "Do Something",
        .cost = 1.0f,
        .preconditions = {{"impossible", true}},
        .effects = {{"result", true}},
        .precondition_count = 1,
        .effect_count = 1
    };
    
    goap_add_action(planner, &action);
    
    GOAPWorldState world = {.fact_count = 0};
    GOAPGoal goal = {
        .desired_state = {{"result", true}},
        .condition_count = 1
    };
    
    GOAPPlan* plan = goap_create_plan(planner, &world, &goal);
    TEST_ASSERT(plan == NULL || plan->action_count == 0, "Impossible goal should produce no plan");
    
    if (plan) goap_destroy_plan(plan);
    goap_planner_destroy(planner);
    return TEST_PASS;
}

// Test cost optimization
static TestResult test_cost_optimization(void) {
    GOAPPlanner* planner = goap_planner_create();
    
    // Two ways to achieve the same goal, one cheaper
    GOAPAction expensive = {
        .name = "Expensive Path",
        .cost = 10.0f,
        .preconditions = {},
        .effects = {{"goal_achieved", true}},
        .precondition_count = 0,
        .effect_count = 1
    };
    
    GOAPAction cheap = {
        .name = "Cheap Path",
        .cost = 2.0f,
        .preconditions = {},
        .effects = {{"goal_achieved", true}},
        .precondition_count = 0,
        .effect_count = 1
    };
    
    goap_add_action(planner, &expensive);
    goap_add_action(planner, &cheap);
    
    GOAPWorldState world = {.fact_count = 0};
    GOAPGoal goal = {
        .desired_state = {{"goal_achieved", true}},
        .condition_count = 1
    };
    
    GOAPPlan* plan = goap_create_plan(planner, &world, &goal);
    TEST_ASSERT_NOT_NULL(plan, "Plan created");
    TEST_ASSERT(plan->total_cost <= 3.0f, "Planner should choose cheaper path");
    
    goap_destroy_plan(plan);
    goap_planner_destroy(planner);
    return TEST_PASS;
}

// Register all tests
int main(void) {
    test_init();
    
    test_register("AI:GOAP", "Initialization", test_goap_init, NULL, NULL);
    test_register("AI:GOAP", "Add Action", test_add_action, NULL, NULL);
    test_register("AI:GOAP", "Plan Generation", test_plan_generation, NULL, NULL);
    test_register("AI:GOAP", "Impossible Goal", test_impossible_goal, NULL, NULL);
    test_register("AI:GOAP", "Cost Optimization", test_cost_optimization, NULL, NULL);
    
    TestStats stats = test_run_all();
    
    printf("\n");
    printf("════════════════════════════════════════════════════════\n");
    printf("  GOAP System Test Results\n");
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

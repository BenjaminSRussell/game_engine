#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#include "include/animation/state_machines/anim_state_machine.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s\n", message); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_FLOAT_EQ(a, b, tolerance, message) \
    do { \
        if (fabsf((a) - (b)) > (tolerance)) { \
            printf("FAIL: %s (expected %.6f, got %.6f)\n", message, (b), (a)); \
            return false; \
        } \
    } while(0)

typedef struct {
    const char* test_name;
    bool (*test_func)(void);
    bool passed;
} TestCase;

static TestCase g_tests[32];
static uint32_t g_test_count = 0;
static uint32_t g_tests_passed = 0;

void add_test(const char* name, bool (*test_func)(void)) {
    if (g_test_count < 32) {
        g_tests[g_test_count].test_name = name;
        g_tests[g_test_count].test_func = test_func;
        g_tests[g_test_count].passed = false;
        g_test_count++;
    }
}

// Test 1: Creation and Destruction
bool test_create_destroy(void) {
    AnimStateMachine* sm = anim_state_machine_create();
    TEST_ASSERT(sm != NULL, "State machine creation failed");

    // Check default state
    TEST_ASSERT(sm->layer_count == 1, "Default layer count should be 1");
    TEST_ASSERT(strcmp(sm->layers[0].name, "Base Layer") == 0, "Default layer name mismatch");

    anim_state_machine_destroy(sm);
    return true;
}

// Test 2: Parameters
bool test_parameters(void) {
    AnimStateMachine* sm = anim_state_machine_create();

    uint32_t f_id = anim_add_parameter_float(sm, "Speed", 0.0f);
    uint32_t i_id = anim_add_parameter_int(sm, "Ammo", 10);
    uint32_t b_id = anim_add_parameter_bool(sm, "IsGrounded", true);
    uint32_t t_id = anim_add_parameter_trigger(sm, "Jump");

    TEST_ASSERT(sm->param_count == 4, "Parameter count mismatch");

    // Check values
    TEST_ASSERT_FLOAT_EQ(sm->parameters[f_id].value.float_val, 0.0f, 0.001f, "Float default value");
    TEST_ASSERT(sm->parameters[i_id].value.int_val == 10, "Int default value");
    TEST_ASSERT(sm->parameters[b_id].value.bool_val == true, "Bool default value");

    // Set values
    anim_set_float(sm, f_id, 5.5f);
    anim_set_int(sm, i_id, 5);
    anim_set_bool(sm, b_id, false);
    anim_set_trigger(sm, t_id);

    TEST_ASSERT_FLOAT_EQ(sm->parameters[f_id].value.float_val, 5.5f, 0.001f, "Float set value");
    TEST_ASSERT(sm->parameters[i_id].value.int_val == 5, "Int set value");
    TEST_ASSERT(sm->parameters[b_id].value.bool_val == false, "Bool set value");
    TEST_ASSERT(sm->parameters[t_id].value.bool_val == true, "Trigger set value");

    anim_state_machine_destroy(sm);
    return true;
}

// Test 3: Transitions and Conditions
bool test_transitions(void) {
    AnimStateMachine* sm = anim_state_machine_create();

    // Setup States
    uint32_t idle = anim_add_state(sm, "Idle", 0);
    uint32_t run = anim_add_state(sm, "Run", 1);

    // Setup Parameters
    uint32_t speed_param = anim_add_parameter_float(sm, "Speed", 0.0f);

    // Transition Idle -> Run if Speed > 1.0
    uint32_t t1 = anim_add_transition(sm, idle, run, 0.2f, 0.0f);
    anim_add_transition_condition_float(sm, t1, speed_param, COND_GREATER, 1.0f);

    // Transition Run -> Idle if Speed <= 1.0
    uint32_t t2 = anim_add_transition(sm, run, idle, 0.2f, 0.0f);
    anim_add_transition_condition_float(sm, t2, speed_param, COND_LESS_EQUAL, 1.0f);

    // Update - should stay in Idle (Speed 0.0)
    anim_state_machine_update(sm, 0.1f);
    TEST_ASSERT(anim_get_current_state(sm, 0) == idle, "Should be in Idle");

    // Change Speed -> 2.0
    anim_set_float(sm, speed_param, 2.0f);
    anim_state_machine_update(sm, 0.1f);

    // Should be transitioning or already in Run
    // transition_duration is 0.2, dt is 0.1, so it should be transitioning
    TEST_ASSERT(anim_is_transitioning(sm, 0), "Should be transitioning to Run");
    TEST_ASSERT(sm->layers[0].current_state == run, "Current state should be Run (target)");

    // Finish transition
    anim_state_machine_update(sm, 0.2f);
    TEST_ASSERT(!anim_is_transitioning(sm, 0), "Transition should end");

    // Change Speed -> 0.5
    anim_set_float(sm, speed_param, 0.5f);
    anim_state_machine_update(sm, 0.1f);

    // Should transition back to Idle
    TEST_ASSERT(anim_is_transitioning(sm, 0), "Should be transitioning to Idle");
    TEST_ASSERT(sm->layers[0].current_state == idle, "Current state should be Idle (target)");

    anim_state_machine_destroy(sm);
    return true;
}

// Test 4: Integer Conditions
bool test_int_conditions(void) {
    AnimStateMachine* sm = anim_state_machine_create();

    uint32_t s1 = anim_add_state(sm, "S1", 0);
    uint32_t s2 = anim_add_state(sm, "S2", 0);

    uint32_t p = anim_add_parameter_int(sm, "Val", 0);

    // S1 -> S2 if Val == 5
    uint32_t t = anim_add_transition(sm, s1, s2, 0.1f, 0.0f);
    anim_add_transition_condition_int(sm, t, p, COND_EQUALS, 5);

    anim_state_machine_update(sm, 0.1f);
    TEST_ASSERT(anim_get_current_state(sm, 0) == s1, "Start at S1");

    anim_set_int(sm, p, 4);
    anim_state_machine_update(sm, 0.1f);
    TEST_ASSERT(anim_get_current_state(sm, 0) == s1, "Still S1 (4 != 5)");

    anim_set_int(sm, p, 5);
    anim_state_machine_update(sm, 0.1f);
    TEST_ASSERT(anim_is_transitioning(sm, 0) || anim_get_current_state(sm, 0) == s2, "Transition to S2");

    anim_state_machine_destroy(sm);
    return true;
}

// Test 5: Trigger Condition
bool test_trigger_condition(void) {
    AnimStateMachine* sm = anim_state_machine_create();

    uint32_t s1 = anim_add_state(sm, "S1", 0);
    uint32_t s2 = anim_add_state(sm, "S2", 0);

    uint32_t trig = anim_add_parameter_trigger(sm, "Go");

    // S1 -> S2 on trigger
    uint32_t t = anim_add_transition(sm, s1, s2, 0.0f, 0.0f);
    anim_add_transition_condition_bool(sm, t, trig, true); // Trigger treated as bool condition usually?
    // Wait, the code says: if (cond->condition == COND_TRUE && p->value.bool_val && !p->trigger_consumed)

    anim_state_machine_update(sm, 0.1f);
    TEST_ASSERT(anim_get_current_state(sm, 0) == s1, "Start at S1");

    anim_set_trigger(sm, trig);
    anim_state_machine_update(sm, 0.1f);
    TEST_ASSERT(anim_get_current_state(sm, 0) == s2, "Transition to S2");

    // Check trigger consumed
    TEST_ASSERT(sm->parameters[trig].trigger_consumed == true, "Trigger should be consumed");
    TEST_ASSERT(sm->parameters[trig].value.bool_val == false, "Trigger value reset");

    anim_state_machine_destroy(sm);
    return true;
}

int main(void) {
    printf("Animation State Machine Unit Tests\n");
    printf("==================================\n\n");

    add_test("Create/Destroy", test_create_destroy);
    add_test("Parameters", test_parameters);
    add_test("Transitions (Float)", test_transitions);
    add_test("Integer Conditions", test_int_conditions);
    add_test("Trigger Condition", test_trigger_condition);

    for (uint32_t i = 0; i < g_test_count; i++) {
        printf("Running Test %u: %s... ", i + 1, g_tests[i].test_name);
        if (g_tests[i].test_func()) {
            printf("PASS\n");
            g_tests_passed++;
        } else {
            // Error message already printed by macro
        }
    }

    printf("\nPassed %u/%u tests.\n", g_tests_passed, g_test_count);
    return (g_tests_passed == g_test_count) ? 0 : 1;
}

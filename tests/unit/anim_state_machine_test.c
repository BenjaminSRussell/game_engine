#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// Include animation state machine header
#include "include/animation/state_machines/anim_state_machine.h"

// Define helper macros if not available (since we are linking against the .c file directly probably)
// or we will compile them together.

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
} AnimTest;

static AnimTest g_tests[32];
static u32 g_test_count = 0;
static u32 g_tests_passed = 0;

void add_test(const char* name, bool (*test_func)(void)) {
    if (g_test_count < 32) {
        g_tests[g_test_count].test_name = name;
        g_tests[g_test_count].test_func = test_func;
        g_tests[g_test_count].passed = false;
        g_test_count++;
    }
}

// =================================================================================================
// TESTS
// =================================================================================================

// Test 1: Creation and Destruction
bool test_creation(void) {
    AnimStateMachine* sm = anim_state_machine_create("TestSM");
    TEST_ASSERT(sm != NULL, "Created SM should not be NULL");

    // Check default layer
    TEST_ASSERT(sm->layer_count == 1, "Should have 1 default layer");
    TEST_ASSERT(sm->state_count == 0, "Should have 0 states initially");

    anim_state_machine_destroy(sm);
    return true;
}

// Test 2: Parameters
bool test_parameters(void) {
    AnimStateMachine* sm = anim_state_machine_create("ParamSM");

    u32 p_float = anim_sm_add_parameter_float(sm, "Speed", 0.0f);
    u32 p_int = anim_sm_add_parameter_int(sm, "Ammo", 10);
    u32 p_bool = anim_sm_add_parameter_bool(sm, "IsGrounded", true);

    TEST_ASSERT(p_float == 0, "First param ID should be 0");
    TEST_ASSERT(p_int == 1, "Second param ID should be 1");

    // Test Get/Set
    anim_sm_set_float(sm, p_float, 5.0f);
    TEST_ASSERT_FLOAT_EQ(anim_sm_get_float(sm, p_float), 5.0f, 0.001f, "Float param set/get");

    anim_sm_set_int(sm, p_int, 5);
    TEST_ASSERT(anim_sm_get_int(sm, p_int) == 5, "Int param set/get");

    anim_sm_set_bool(sm, p_bool, false);
    TEST_ASSERT(anim_sm_get_bool(sm, p_bool) == false, "Bool param set/get");

    // Test name lookup
    u32 lookup_id = anim_sm_get_param_id(sm, "Ammo");
    TEST_ASSERT(lookup_id == p_int, "Parameter name lookup");

    anim_state_machine_destroy(sm);
    return true;
}

// Test 3: State Transitions
bool test_transitions(void) {
    AnimStateMachine* sm = anim_state_machine_create("TransitionSM");

    // Add parameters
    u32 p_speed = anim_sm_add_parameter_float(sm, "Speed", 0.0f);

    // Add states
    u32 s_idle = anim_sm_add_state(sm, "Idle");
    u32 s_run = anim_sm_add_state(sm, "Run");

    // Add transition Idle -> Run if Speed > 1.0
    u32 t_run = anim_sm_add_transition(sm, s_idle, s_run, 0.2f);
    anim_sm_add_condition_float(sm, t_run, p_speed, ANIM_COND_GREATER, 1.0f);

    // Add transition Run -> Idle if Speed < 0.1
    u32 t_idle = anim_sm_add_transition(sm, s_run, s_idle, 0.2f);
    anim_sm_add_condition_float(sm, t_idle, p_speed, ANIM_COND_LESS, 0.1f);

    // Initial State is Idle (0)
    TEST_ASSERT(anim_sm_get_current_state(sm, 0) == s_idle, "Initial state should be Idle");

    // Update (should stay Idle)
    anim_sm_update(sm, 0.1f);
    TEST_ASSERT(anim_sm_get_current_state(sm, 0) == s_idle, "Should stay Idle with Speed 0");

    // Set speed to trigger transition
    anim_sm_set_float(sm, p_speed, 2.0f);
    anim_sm_update(sm, 0.1f);

    // Should be transitioning now
    // In current implementation, we switch state ID immediately at start of transition
    TEST_ASSERT(anim_sm_get_current_state(sm, 0) == s_run, "Should switch to Run");

    // Go back to idle
    anim_sm_set_float(sm, p_speed, 0.0f);

    // Finish transition (0.2s duration, we did 0.1s already? No, we did 0.1s update which STARTED it).
    // Wait, first update(0.1) started it. transition_time initialized to 0.
    // So we need 0.2s more to finish?
    // Actually, let's just update enough time.

    anim_sm_update(sm, 0.25f); // Finish blend AND trigger next transition because Speed is 0.0

    // Because we set Speed to 0.0 before finishing the transition, and the update loop checks for new transitions
    // immediately after finishing one, it will immediately switch back to Idle in the same frame.

    TEST_ASSERT(anim_sm_get_current_state(sm, 0) == s_idle, "Should switch back to Idle immediately after transition finishes due to condition");

    anim_state_machine_destroy(sm);
    return true;
}

// Test 4: Events / Notifies
static bool g_notify_called = false;
static void on_notify_test(AnimStateMachine* sm, const char* name, void* context) {
    g_notify_called = true;
    if (strcmp(name, "Footstep") != 0) {
        printf("Wrong notify name: %s\n", name);
        g_notify_called = false;
    }
}

static bool g_enter_called = false;
static void on_enter_test(AnimStateMachine* sm, void* context) {
    g_enter_called = true;
}

bool test_events(void) {
    AnimStateMachine* sm = anim_state_machine_create("EventSM");

    u32 s_attack = anim_sm_add_state(sm, "Attack");

    // Add notify at 0.5 time
    anim_sm_add_state_notify(sm, s_attack, "Footstep", 0.5f, on_notify_test);

    // Add enter callback
    anim_sm_set_state_callbacks(sm, s_attack, on_enter_test, NULL, NULL);

    // Note: Default state is 0 (Attack)

    // Reset flags
    g_notify_called = false;
    g_enter_called = false;

    // We haven't started yet. When we update, we are already in state 0?
    // The system initializes current_state to 0. But OnEnter is usually called on transition.
    // For the initial state, it might not be called automatically in this simple impl unless we force it.
    // However, Notifies should work.

    // Update to 0.4
    anim_sm_update(sm, 0.4f); // Assuming 1.0s length
    TEST_ASSERT(!g_notify_called, "Notify should not be called yet");

    // Update past 0.5
    anim_sm_update(sm, 0.2f); // Now at 0.6
    TEST_ASSERT(g_notify_called, "Notify should be called");

    anim_state_machine_destroy(sm);
    return true;
}

int main(void) {
    printf("Animation State Machine Unit Tests\n");
    printf("==================================\n\n");

    add_test("Creation & Destruction", test_creation);
    add_test("Parameters", test_parameters);
    add_test("Transitions", test_transitions);
    add_test("Events & Notifies", test_events);

    for (u32 i = 0; i < g_test_count; i++) {
        printf("Running Test %u: %s... ", i + 1, g_tests[i].test_name);
        if (g_tests[i].test_func()) {
            printf("PASS\n");
            g_tests_passed++;
        } else {
            printf("FAIL\n");
        }
    }

    printf("\nPassed: %u/%u\n", g_tests_passed, g_test_count);
    return (g_tests_passed == g_test_count) ? 0 : 1;
}

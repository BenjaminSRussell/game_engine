#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// Include animation state machine header
#include "include/animation/state_machines/anim_state_machine.h"
#include "include/core/logger/unified_logger.h"
#include <stdarg.h>

// Mock Logger
void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    // Silent mock or print if needed
    // va_list args;
    // va_start(args, format);
    // vprintf(format, args);
    // printf("\n");
    // va_end(args);
}

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

// Test 5: Sub-state Machines
bool test_sub_machines(void) {
    AnimStateMachine* sm = anim_state_machine_create("ParentSM");
    AnimStateMachine* sub = anim_state_machine_create("SubSM");

    // Setup Sub: Idle -> Walk (after 0.5s)
    u32 sub_idle = anim_sm_add_state(sub, "SubIdle");
    u32 sub_walk = anim_sm_add_state(sub, "SubWalk");
    u32 t_sub = anim_sm_add_transition(sub, sub_idle, sub_walk, 0.1f);
    anim_sm_add_condition_state_time(sub, t_sub, 0.5f, ANIM_COND_GREATER);

    // Setup Parent: State A (with Sub) -> State B
    u32 s_A = anim_sm_add_state(sm, "A");
    anim_sm_set_state_sub_machine(sm, s_A, sub);

    u32 s_B = anim_sm_add_state(sm, "B");

    // Transition A -> B trigger
    u32 p_trig = anim_sm_add_parameter_trigger(sm, "Next");
    u32 t_AB = anim_sm_add_transition(sm, s_A, s_B, 0.1f);
    anim_sm_add_condition_trigger(sm, t_AB, p_trig);

    // Transition B -> A trigger
    u32 t_BA = anim_sm_add_transition(sm, s_B, s_A, 0.1f);
    anim_sm_add_condition_trigger(sm, t_BA, p_trig);

    // Start in A
    anim_sm_update(sm, 0.1f);

    // Run for 0.6s -> Sub should be in Walk
    anim_sm_update(sm, 0.6f);
    TEST_ASSERT(anim_sm_get_current_state(sub, 0) == sub_walk, "Sub should be in Walk");

    // Trigger transition to B
    anim_sm_set_trigger(sm, p_trig);
    anim_sm_update(sm, 0.15f); // Start transition
    anim_sm_update(sm, 0.15f); // Finish transition
    TEST_ASSERT(anim_sm_get_current_state(sm, 0) == s_B, "Parent should be in B");

    // Now go back to A
    anim_sm_set_trigger(sm, p_trig);
    anim_sm_update(sm, 0.15f); // Start transition back to A

    // When entering A, sub-machine should be RESET (back to Idle)
    TEST_ASSERT(anim_sm_get_current_state(sub, 0) == sub_idle, "Sub should be reset to Idle upon re-entry");

    anim_state_machine_destroy(sm);
    anim_state_machine_destroy(sub);
    return true;
}

// Test 6: Timeouts and Time Remaining
bool test_timeouts(void) {
    AnimStateMachine* sm = anim_state_machine_create("TimeoutSM");
    u32 s_1 = anim_sm_add_state(sm, "State1");
    // Default length is 1.0s

    u32 s_2 = anim_sm_add_state(sm, "State2");

    // Transition when time remaining < 0.2s
    u32 t_12 = anim_sm_add_transition(sm, s_1, s_2, 0.1f);
    anim_sm_add_condition_time_remaining(sm, t_12, 0.2f, ANIM_COND_LESS);

    anim_sm_update(sm, 0.5f); // Time 0.5, Remaining 0.5. > 0.2. No trigger.
    TEST_ASSERT(anim_sm_get_current_state(sm, 0) == s_1, "Should stay in State1");

    anim_sm_update(sm, 0.4f); // Time 0.9. Remaining 0.1. <= 0.2. Trigger!

    TEST_ASSERT(anim_sm_get_current_state(sm, 0) == s_2, "Should transition to State2 due to time remaining");

    anim_state_machine_destroy(sm);
    return true;
}

// Test 7: Blending Data
bool test_blending(void) {
    AnimStateMachine* sm = anim_state_machine_create("BlendSM");
    u32 s_A = anim_sm_add_state(sm, "A");
    u32 s_B = anim_sm_add_state(sm, "B");

    u32 p_trig = anim_sm_add_parameter_trigger(sm, "Go");

    u32 t_AB = anim_sm_add_transition(sm, s_A, s_B, 1.0f); // 1.0s duration

    anim_sm_add_condition_trigger(sm, t_AB, p_trig);

    anim_sm_update(sm, 0.1f); // Setup
    anim_sm_set_trigger(sm, p_trig);
    anim_sm_update(sm, 0.1f); // Start transition (time is 0 at end of this frame)
    anim_sm_update(sm, 0.1f); // Advance transition (time becomes 0.1)

    TEST_ASSERT(anim_sm_get_current_state(sm, 0) == s_B, "Current state is B");

    AnimBlendState blend_states[4];
    AnimBlendData data = { .states = blend_states, .count = 4 };

    anim_sm_get_blend_data(sm, 0, &data);

    TEST_ASSERT(data.count == 2, "Should have 2 states blending");

    // Find B
    bool found_B = false;
    for(u32 i=0; i<data.count; i++) {
        if(data.states[i].state_id == s_B) {
            TEST_ASSERT_FLOAT_EQ(data.states[i].weight, 0.1f, 0.001f, "Weight for B should be 0.1");
            found_B = true;
        }
    }
    TEST_ASSERT(found_B, "Should find state B in blend data");

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
    add_test("Sub-Machines", test_sub_machines);
    add_test("Timeouts", test_timeouts);
    add_test("Blending", test_blending);

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

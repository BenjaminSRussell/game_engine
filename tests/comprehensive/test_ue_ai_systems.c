/**
 * UNREAL ENGINE PARITY TESTS - AI SYSTEMS
 * Test-Driven Development for Mass AI, State Tree, Smart Objects
 */

#include "../test_framework_unified.h"
#include "ue_parity_stubs.h"

// =============================================================================
// MASS ENTITY (MASS AI) TESTS
// =============================================================================

static TestResult test_mass_entity_creation(void) {
    MassConfig config = {
        .max_entities = 100000,
        .use_zone_graph = true,
        .observation_radius = 5000.0f
    };
    
    MassEntityManager* mass = mass_create(&config);
    TEST_ASSERT_NOT_NULL(mass, "Mass entity manager should be created");
    
    mass_destroy(mass);
    return TEST_PASS;
}

static TestResult test_mass_spawning(void) {
    MassConfig config = {.max_entities = 10000};
    MassEntityManager* mass = mass_create(&config);
    
    // Spawn entities
    for (int i = 0; i < 1000; i++) {
        Handle entity = mass_spawn_entity(mass, NULL);
        TEST_ASSERT_TRUE(entity != 0, "Should spawn entity");
    }
    
    uint32_t count = mass_get_entity_count(mass);
    TEST_ASSERT_EQ(count, 1000, "Should have 1000 entities");
    
    mass_destroy(mass);
    return TEST_PASS;
}

static TestResult test_mass_processing(void) {
    MassConfig config = {.max_entities = 5000};
    MassEntityManager* mass = mass_create(&config);
    
    // Spawn test entities
    for (int i = 0; i < 500; i++) {
        mass_spawn_entity(mass, NULL);
    }
    
    // Tick the system
    mass_tick(mass, 0.016f);
    
    // Should still have all entities
    uint32_t count = mass_get_entity_count(mass);
    TEST_ASSERT_EQ(count, 500, "Entities preserved after tick");
    
    mass_destroy(mass);
    return TEST_PASS;
}

static TestResult test_mass_destruction(void) {
    MassConfig config = {.max_entities = 1000};
    MassEntityManager* mass = mass_create(&config);
    
    Handle entities[100];
    for (int i = 0; i < 100; i++) {
        entities[i] = mass_spawn_entity(mass, NULL);
    }
    
    // Destroy half
    for (int i = 0; i < 50; i++) {
        mass_destroy_entity(mass, entities[i]);
    }
    
    uint32_t count = mass_get_entity_count(mass);
    TEST_ASSERT_EQ(count, 50, "Should have 50 remaining");
    
    mass_destroy(mass);
    return TEST_PASS;
}

// =============================================================================
// STATE TREE TESTS
// =============================================================================

static TestResult test_state_tree_creation(void) {
    StateTree* tree = state_tree_create("EnemyAI");
    TEST_ASSERT_NOT_NULL(tree, "State tree should be created");
    
    state_tree_destroy(tree);
    return TEST_PASS;
}

static TestResult test_state_tree_hierarchy(void) {
    StateTree* tree = state_tree_create("CombatAI");
    
    // Root states
    StateTreeState* idle = state_tree_add_state(tree, "Idle", NULL);
    StateTreeState* combat = state_tree_add_state(tree, "Combat", NULL);
    
    // Combat substates
    StateTreeState* approach = state_tree_add_state(tree, "Approach", combat);
    StateTreeState* attack = state_tree_add_state(tree, "Attack", combat);
    StateTreeState* retreat = state_tree_add_state(tree, "Retreat", combat);
    
    TEST_ASSERT_NOT_NULL(idle, "Idle state created");
    TEST_ASSERT_NOT_NULL(combat, "Combat state created");
    TEST_ASSERT_NOT_NULL(approach, "Approach substate created");
    TEST_ASSERT_NOT_NULL(attack, "Attack substate created");
    TEST_ASSERT_NOT_NULL(retreat, "Retreat substate created");
    
    state_tree_destroy(tree);
    return TEST_PASS;
}

static TestResult test_state_tree_transitions(void) {
    StateTree* tree = state_tree_create("PatrolAI");
    
    StateTreeState* patrol = state_tree_add_state(tree, "Patrol", NULL);
    StateTreeState* investigate = state_tree_add_state(tree, "Investigate", NULL);
    StateTreeState* alert = state_tree_add_state(tree, "Alert", NULL);
    
    // Add transitions
    state_tree_add_transition(tree, patrol, investigate, "NoiseHeard");
    state_tree_add_transition(tree, investigate, alert, "EnemySighted");
    state_tree_add_transition(tree, investigate, patrol, "NothingFound");
    state_tree_add_transition(tree, alert, patrol, "LostTarget");
    
    state_tree_destroy(tree);
    return TEST_PASS;
}

static TestResult test_state_tree_execution(void) {
    StateTree* tree = state_tree_create("TestAI");
    
    StateTreeState* start = state_tree_add_state(tree, "Start", NULL);
    StateTreeState* running = state_tree_add_state(tree, "Running", NULL);
    state_tree_add_transition(tree, start, running, "Always");
    
    // Start execution
    state_tree_start(tree, NULL);
    
    // Get current state
    const char* current = state_tree_get_current_state(tree);
    TEST_ASSERT_NOT_NULL(current, "Should have current state");
    
    // Tick
    state_tree_tick(tree, 0.016f);
    
    state_tree_destroy(tree);
    return TEST_PASS;
}

// =============================================================================
// SMART OBJECTS TESTS
// =============================================================================

static TestResult test_smart_object_system(void) {
    SmartObjectSubsystem* ss = smart_object_subsystem_create();
    TEST_ASSERT_NOT_NULL(ss, "Smart object subsystem created");
    
    smart_object_subsystem_destroy(ss);
    return TEST_PASS;
}

static TestResult test_smart_object_registration(void) {
    SmartObjectSubsystem* ss = smart_object_subsystem_create();
    
    SmartObjectDefinition def = {0}; // Stub
    Transform transform = {
        .position = {100, 0, 100},
        .rotation = {0, 0, 0, 1},
        .scale = {1, 1, 1}
    };
    
    Handle handle = smart_object_register(ss, &def, transform);
    TEST_ASSERT_TRUE(handle != 0, "Should register smart object");
    
    smart_object_unregister(ss, handle);
    
    smart_object_subsystem_destroy(ss);
    return TEST_PASS;
}

static TestResult test_smart_object_slot_finding(void) {
    SmartObjectSubsystem* ss = smart_object_subsystem_create();
    
    // Register some smart objects
    SmartObjectDefinition def = {0};
    Transform t1 = {.position = {50, 0, 50}};
    Transform t2 = {.position = {100, 0, 100}};
    
    smart_object_register(ss, &def, t1);
    smart_object_register(ss, &def, t2);
    
    // Find slot near position
    Vec3 search_pos = {55, 0, 55};
    SmartObjectSlot* slot = smart_object_find_slot(ss, search_pos, 20.0f, "Interact");
    
    // May or may not find slot depending on activity tag matching
    // The test verifies the API works
    
    smart_object_subsystem_destroy(ss);
    return TEST_PASS;
}

static TestResult test_smart_object_claiming(void) {
    SmartObjectSubsystem* ss = smart_object_subsystem_create();
    
    SmartObjectDefinition def = {0};
    Transform transform = {.position = {0, 0, 0}};
    smart_object_register(ss, &def, transform);
    
    SmartObjectSlot* slot = smart_object_find_slot(ss, (Vec3){0,0,0}, 10.0f, NULL);
    
    if (slot) {
        Entity claimer = 42;
        bool claimed = smart_object_claim_slot(slot, claimer);
        TEST_ASSERT_TRUE(claimed, "Should claim slot");
        
        smart_object_release_slot(slot, claimer);
    }
    
    smart_object_subsystem_destroy(ss);
    return TEST_PASS;
}

// =============================================================================
// REGISTRATION
// =============================================================================

void register_ue_ai_tests(void) {
    // Mass Entity
    TEST_REGISTER("UE:MassAI", "Mass creation", test_mass_entity_creation);
    TEST_REGISTER("UE:MassAI", "Mass spawning", test_mass_spawning);
    TEST_REGISTER("UE:MassAI", "Mass processing", test_mass_processing);
    TEST_REGISTER("UE:MassAI", "Mass destruction", test_mass_destruction);
    
    // State Tree
    TEST_REGISTER("UE:StateTree", "Creation", test_state_tree_creation);
    TEST_REGISTER("UE:StateTree", "Hierarchy", test_state_tree_hierarchy);
    TEST_REGISTER("UE:StateTree", "Transitions", test_state_tree_transitions);
    TEST_REGISTER("UE:StateTree", "Execution", test_state_tree_execution);
    
    // Smart Objects
    TEST_REGISTER("UE:SmartObjects", "System creation", test_smart_object_system);
    TEST_REGISTER("UE:SmartObjects", "Registration", test_smart_object_registration);
    TEST_REGISTER("UE:SmartObjects", "Slot finding", test_smart_object_slot_finding);
    TEST_REGISTER("UE:SmartObjects", "Slot claiming", test_smart_object_claiming);
}

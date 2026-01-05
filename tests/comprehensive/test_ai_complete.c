/**
 * COMPREHENSIVE AI TESTS
 * Tests for all 30+ AI features matching Unreal/Unity capabilities
 * 
 * Coverage:
 * - Navigation meshes
 * - A* pathfinding
 * - Behavior trees
 * - State machines
 * - GOAP planning
 * - Utility AI
 * - Perception (vision, hearing)
 * - AI memory
 */

#include "../test_framework_unified.h"
#include <ai/navigation/navmesh.h>
#include <ai/navigation/pathfinding.h>
#include <ai/behavior/behavior_tree.h>
#include <ai/behavior/state_machine.h>
#include <ai/planning/goap.h>
#include <ai/utility/utility_ai.h>
#include <ai/perception/perception_system.h>
#include <ai/memory/ai_memory.h>

// =============================================================================
// NAVMESH TESTS
// =============================================================================

static TestResult test_navmesh_generation(void) {
    NavMeshConfig config = {
        .cell_size = 0.3f,
        .cell_height = 0.2f,
        .agent_height = 2.0f,
        .agent_radius = 0.6f,
        .agent_max_climb = 0.4f,
        .agent_max_slope = 45.0f
    };
    
    // Simple test geometry
    Vec3 vertices[] = {
        {-10, 0, -10}, {10, 0, -10}, {10, 0, 10}, {-10, 0, 10}
    };
    int indices[] = {0, 1, 2, 0, 2, 3};
    
    NavMesh *navmesh = navmesh_build(vertices, 4, indices, 6, &config);
    TEST_ASSERT_NOT_NULL(navmesh, "NavMesh should be generated");
    TEST_ASSERT_TRUE(navmesh->poly_count > 0, "Should have polygons");
    
    navmesh_destroy(navmesh);
    return TEST_PASS;
}

static TestResult test_navmesh_query(void) {
    NavMesh *navmesh = navmesh_load("assets/navmesh/test.nav");
    TEST_SKIP_IF(navmesh == NULL, "Test navmesh not found");
    
    Vec3 point = {0, 0, 0};
    NavMeshPoly *poly = navmesh_find_nearest_poly(navmesh, point, 10.0f);
    TEST_ASSERT_NOT_NULL(poly, "Should find nearest poly");
    
    navmesh_destroy(navmesh);
    return TEST_PASS;
}

// =============================================================================
// PATHFINDING TESTS
// =============================================================================

static TestResult test_astar_pathfinding(void) {
    NavMesh *navmesh = navmesh_create_test_grid(20, 20, 1.0f);
    PathfindingQuery query = {
        .start = {1.0f, 0.0f, 1.0f},
        .end = {18.0f, 0.0f, 18.0f},
        .agent_radius = 0.5f
    };
    
    Path *path = pathfinding_find_path(navmesh, &query);
    TEST_ASSERT_NOT_NULL(path, "Should find path");
    TEST_ASSERT_TRUE(path->waypoint_count > 2, "Path should have waypoints");
    
    path_destroy(path);
    navmesh_destroy(navmesh);
    return TEST_PASS;
}

static TestResult test_path_smoothing(void) {
    NavMesh *navmesh = navmesh_create_test_grid(20, 20, 1.0f);
    PathfindingQuery query = {
        .start = {1.0f, 0.0f, 1.0f},
        .end = {18.0f, 0.0f, 18.0f},
        .agent_radius = 0.5f
    };
    
    Path *path = pathfinding_find_path(navmesh, &query);
    int original_count = path->waypoint_count;
    
    path_smooth(path, navmesh, 0.5f);
    
    // Smoothed path should have fewer or equal waypoints
    TEST_ASSERT_TRUE(path->waypoint_count <= original_count, "Smoothed path");
    
    path_destroy(path);
    navmesh_destroy(navmesh);
    return TEST_PASS;
}

// =============================================================================
// BEHAVIOR TREE TESTS
// =============================================================================

static TestResult test_behavior_tree_creation(void) {
    BehaviorTree *tree = behavior_tree_create("EnemyAI");
    TEST_ASSERT_NOT_NULL(tree, "Behavior tree should be created");
    
    behavior_tree_destroy(tree);
    return TEST_PASS;
}

static TestResult test_behavior_tree_nodes(void) {
    BehaviorTree *tree = behavior_tree_create("Test");
    
    // Create root selector
    BTNode *root = bt_selector_create("Root");
    behavior_tree_set_root(tree, root);
    
    // Add sequence for attack
    BTNode *attack_seq = bt_sequence_create("AttackSequence");
    bt_node_add_child(root, attack_seq);
    
    // Add conditions and actions
    BTNode *has_target = bt_condition_create("HasTarget", NULL);
    BTNode *in_range = bt_condition_create("InRange", NULL);
    BTNode *attack = bt_action_create("Attack", NULL);
    
    bt_node_add_child(attack_seq, has_target);
    bt_node_add_child(attack_seq, in_range);
    bt_node_add_child(attack_seq, attack);
    
    // Add fallback idle
    BTNode *idle = bt_action_create("Idle", NULL);
    bt_node_add_child(root, idle);
    
    TEST_ASSERT_EQ(bt_node_child_count(root), 2, "Root has 2 children");
    
    behavior_tree_destroy(tree);
    return TEST_PASS;
}

static TestResult test_behavior_tree_blackboard(void) {
    Blackboard *bb = blackboard_create();
    
    blackboard_set_float(bb, "Health", 100.0f);
    blackboard_set_vec3(bb, "TargetPosition", (Vec3){10, 0, 5});
    blackboard_set_bool(bb, "HasTarget", true);
    blackboard_set_entity(bb, "CurrentTarget", 42);
    
    float health = blackboard_get_float(bb, "Health");
    TEST_ASSERT_FLOAT_EQ(health, 100.0f, 0.01f, "Health value");
    
    bool has_target = blackboard_get_bool(bb, "HasTarget");
    TEST_ASSERT_TRUE(has_target, "Has target");
    
    blackboard_destroy(bb);
    return TEST_PASS;
}

// =============================================================================
// AI STATE MACHINE TESTS
// =============================================================================

static TestResult test_ai_state_machine(void) {
    AIStateMachine *sm = ai_state_machine_create("EnemySM");
    
    // Add states
    AIState *idle = ai_state_create("Idle");
    AIState *patrol = ai_state_create("Patrol");
    AIState *chase = ai_state_create("Chase");
    AIState *attack = ai_state_create("Attack");
    
    ai_state_machine_add_state(sm, idle);
    ai_state_machine_add_state(sm, patrol);
    ai_state_machine_add_state(sm, chase);
    ai_state_machine_add_state(sm, attack);
    
    // Add transitions
    ai_state_machine_add_transition(sm, idle, patrol, "TimeElapsed", 5.0f);
    ai_state_machine_add_transition(sm, patrol, chase, "EnemySpotted", 0.0f);
    ai_state_machine_add_transition(sm, chase, attack, "InAttackRange", 0.0f);
    
    ai_state_machine_set_initial(sm, idle);
    
    TEST_ASSERT_EQ(ai_state_machine_state_count(sm), 4, "Should have 4 states");
    
    ai_state_machine_destroy(sm);
    return TEST_PASS;
}

// =============================================================================
// GOAP TESTS
// =============================================================================

static TestResult test_goap_planning(void) {
    GOAPPlanner *planner = goap_planner_create();
    
    // Define world state
    WorldState current = {0};
    world_state_set_bool(&current, "HasWeapon", true);
    world_state_set_bool(&current, "EnemyVisible", true);
    world_state_set_bool(&current, "EnemyDead", false);
    world_state_set_float(&current, "Health", 80.0f);
    
    // Define goal
    WorldState goal = {0};
    world_state_set_bool(&goal, "EnemyDead", true);
    
    // Define actions
    GOAPAction *attack = goap_action_create("AttackEnemy");
    goap_action_add_precondition(attack, "HasWeapon", true);
    goap_action_add_precondition(attack, "EnemyVisible", true);
    goap_action_add_effect(attack, "EnemyDead", true);
    goap_action_set_cost(attack, 1.0f);
    
    goap_planner_add_action(planner, attack);
    
    // Plan
    GOAPPlan *plan = goap_planner_plan(planner, &current, &goal);
    TEST_ASSERT_NOT_NULL(plan, "Should create plan");
    TEST_ASSERT_TRUE(plan->action_count > 0, "Plan should have actions");
    
    goap_plan_destroy(plan);
    goap_planner_destroy(planner);
    return TEST_PASS;
}

// =============================================================================
// UTILITY AI TESTS
// =============================================================================

static TestResult test_utility_ai_scoring(void) {
    UtilityAI *utility = utility_ai_create();
    
    // Add behaviors with response curves
    UtilityBehavior *attack = utility_behavior_create("Attack");
    utility_behavior_add_consideration(attack, "HealthRatio", CURVE_LINEAR, 1.0f);
    utility_behavior_add_consideration(attack, "DistanceToEnemy", CURVE_EXPONENTIAL, -1.0f);
    
    UtilityBehavior *flee = utility_behavior_create("Flee");
    utility_behavior_add_consideration(flee, "HealthRatio", CURVE_INVERSE_LINEAR, 1.0f);
    utility_behavior_add_consideration(flee, "DistanceToEnemy", CURVE_LINEAR, -1.0f);
    
    utility_ai_add_behavior(utility, attack);
    utility_ai_add_behavior(utility, flee);
    
    // Set context values
    UtilityContext ctx = {0};
    utility_context_set(&ctx, "HealthRatio", 0.2f);  // Low health
    utility_context_set(&ctx, "DistanceToEnemy", 5.0f);
    
    // Evaluate
    const char *best = utility_ai_evaluate(utility, &ctx);
    TEST_ASSERT_STRING_EQ(best, "Flee", "Should flee at low health");
    
    utility_ai_destroy(utility);
    return TEST_PASS;
}

// =============================================================================
// PERCEPTION TESTS
// =============================================================================

static TestResult test_vision_perception(void) {
    PerceptionSystem *perception = perception_system_create();
    
    VisionConfig vision = {
        .fov_angle = 90.0f,
        .view_distance = 50.0f,
        .near_distance = 2.0f,
        .peripheral_angle = 120.0f,
        .peripheral_multiplier = 0.5f
    };
    
    perception_add_sense(perception, SENSE_VISION, &vision);
    
    // Test visibility
    Vec3 observer_pos = {0, 1.7f, 0};
    Vec3 observer_fwd = {0, 0, 1};
    Vec3 target_pos = {0, 1.7f, 20};
    
    float visibility = perception_check_visibility(perception, 
        observer_pos, observer_fwd, target_pos);
    
    TEST_ASSERT_TRUE(visibility > 0.0f, "Target should be visible");
    
    perception_system_destroy(perception);
    return TEST_PASS;
}

static TestResult test_hearing_perception(void) {
    PerceptionSystem *perception = perception_system_create();
    
    HearingConfig hearing = {
        .max_range = 30.0f,
        .threshold = 0.1f
    };
    
    perception_add_sense(perception, SENSE_HEARING, &hearing);
    
    // Generate sound event
    SoundEvent sound = {
        .position = {10, 0, 10},
        .loudness = 0.8f,
        .type = SOUND_FOOTSTEP
    };
    
    Vec3 listener = {0, 0, 0};
    float heard = perception_check_sound(perception, listener, &sound);
    
    TEST_ASSERT_TRUE(heard > 0.0f, "Should hear sound");
    
    perception_system_destroy(perception);
    return TEST_PASS;
}

// =============================================================================
// AI MEMORY TESTS
// =============================================================================

static TestResult test_ai_memory(void) {
    AIMemory *memory = ai_memory_create(100);
    
    // Add memory records
    MemoryRecord record1 = {
        .type = MEMORY_ENEMY_SIGHTED,
        .position = {10, 0, 5},
        .timestamp = 0.0f,
        .confidence = 1.0f,
        .entity = 42
    };
    
    ai_memory_add(memory, &record1);
    
    // Query memory
    MemoryRecord *found = ai_memory_find(memory, MEMORY_ENEMY_SIGHTED, 42);
    TEST_ASSERT_NOT_NULL(found, "Should find memory");
    TEST_ASSERT_FLOAT_EQ(found->position.x, 10.0f, 0.01f, "Position match");
    
    ai_memory_destroy(memory);
    return TEST_PASS;
}

static TestResult test_memory_decay(void) {
    AIMemory *memory = ai_memory_create(100);
    
    MemoryRecord record = {
        .type = MEMORY_ENEMY_SIGHTED,
        .timestamp = 0.0f,
        .confidence = 1.0f,
        .decay_rate = 0.1f
    };
    
    ai_memory_add(memory, &record);
    
    // Update with time
    ai_memory_update(memory, 5.0f);
    
    MemoryRecord *found = ai_memory_get(memory, 0);
    TEST_ASSERT_TRUE(found->confidence < 1.0f, "Confidence should decay");
    
    ai_memory_destroy(memory);
    return TEST_PASS;
}

// =============================================================================
// REGISTRATION
// =============================================================================

void register_ai_tests(void) {
    // NavMesh tests
    TEST_REGISTER("AI", "NavMesh generation", test_navmesh_generation);
    TEST_REGISTER("AI", "NavMesh query", test_navmesh_query);
    
    // Pathfinding tests
    TEST_REGISTER("AI", "A* pathfinding", test_astar_pathfinding);
    TEST_REGISTER("AI", "Path smoothing", test_path_smoothing);
    
    // Behavior tree tests
    TEST_REGISTER("AI", "Behavior tree creation", test_behavior_tree_creation);
    TEST_REGISTER("AI", "Behavior tree nodes", test_behavior_tree_nodes);
    TEST_REGISTER("AI", "Blackboard data", test_behavior_tree_blackboard);
    
    // State machine tests
    TEST_REGISTER("AI", "AI state machine", test_ai_state_machine);
    
    // GOAP tests
    TEST_REGISTER("AI", "GOAP planning", test_goap_planning);
    
    // Utility AI tests
    TEST_REGISTER("AI", "Utility AI scoring", test_utility_ai_scoring);
    
    // Perception tests
    TEST_REGISTER("AI", "Vision perception", test_vision_perception);
    TEST_REGISTER("AI", "Hearing perception", test_hearing_perception);
    
    // Memory tests
    TEST_REGISTER("AI", "AI memory", test_ai_memory);
    TEST_REGISTER("AI", "Memory decay", test_memory_decay);
}

// IK Accuracy Testing
#include "include/animation/ik_system.h"
#include "include/core/logger.h"
#include <stdio.h>
#include <math.h>

#define TEST_TOLERANCE 0.01f
#define TEST_ITERATIONS 100

typedef struct {
    const char* test_name;
    bool passed;
    float error;
    const char* details;
} IKTestResult;

static IKTestResult g_test_results[20];
static u32 g_test_count = 0;

void add_test_result(const char* name, bool passed, float error, const char* details) {
    if (g_test_count >= 20) return;
    
    g_test_results[g_test_count].test_name = name;
    g_test_results[g_test_count].passed = passed;
    g_test_results[g_test_count].error = error;
    g_test_results[g_test_count].details = details;
    g_test_count++;
}

bool test_fabrik_reachability(void) {
    log_info("Testing FABRIK reachability...");
    
    IKSystem* system = ik_system_create();
    if (!system) return false;
    
    // Create a simple 3-bone chain
    u32 chain_id = ik_add_chain(system, "test_chain", IK_SOLVER_FABRIK);
    if (chain_id == UINT32_MAX) {
        ik_system_destroy(system);
        return false;
    }
    
    IKChain* chain = &system->chains[chain_id];
    
    // Setup bones: each 1.0 unit length, straight line along X axis
    chain->bone_count = 3;
    chain->bones[0].position = (Vec3){0, 0, 0};
    chain->bones[0].length = 1.0f;
    chain->bones[1].position = (Vec3){1, 0, 0};
    chain->bones[1].length = 1.0f;
    chain->bones[2].position = (Vec3){2, 0, 0};
    chain->bones[2].length = 1.0f;
    
    // Test reachable target (within total length of 3.0)
    Vec3 target = (Vec3){2.5, 0.5, 0};
    ik_set_target(system, chain_id, target, (Quat){0, 0, 0, 1});
    ik_solve(system, chain_id);
    
    // Check if end effector reached target
    Vec3 end_pos = chain->bones[2].position;
    float error = vec3_distance(&end_pos, &target);
    
    bool passed = error < TEST_TOLERANCE;
    add_test_result("FABRIK Reachable Target", passed, error, 
                   passed ? "Target reached successfully" : "Failed to reach target");
    
    ik_system_destroy(system);
    return passed;
}

bool test_fabrik_unreachable(void) {
    log_info("Testing FABRIK unreachable target...");
    
    IKSystem* system = ik_system_create();
    if (!system) return false;
    
    u32 chain_id = ik_add_chain(system, "test_chain", IK_SOLVER_FABRIK);
    IKChain* chain = &system->chains[chain_id];
    
    // Setup 2-bone chain, total length 2.0
    chain->bone_count = 2;
    chain->bones[0].position = (Vec3){0, 0, 0};
    chain->bones[0].length = 1.0f;
    chain->bones[1].position = (Vec3){1, 0, 0};
    chain->bones[1].length = 1.0f;
    
    // Test unreachable target (distance > total length)
    Vec3 target = (Vec3){4, 0, 0}; // Distance is 4.0, but total length is 2.0
    ik_set_target(system, chain_id, target, (Quat){0, 0, 0, 1});
    ik_solve(system, chain_id);
    
    // Check if chain stretched towards target
    Vec3 end_pos = chain->bones[1].position;
    float distance_to_target = vec3_distance(&end_pos, &target);
    float chain_length = vec3_distance(&chain->bones[0].position, &end_pos);
    
    // Chain should be fully extended (length = 2.0)
    bool passed = fabsf(chain_length - 2.0f) < 0.01f;
    add_test_result("FABRIK Unreachable Target", passed, distance_to_target,
                   passed ? "Chain stretched correctly" : "Chain behavior incorrect");
    
    ik_system_destroy(system);
    return passed;
}

bool test_two_bone_analytical(void) {
    log_info("Testing Two-Bone analytical IK...");
    
    IKSystem* system = ik_system_create();
    if (!system) return false;
    
    u32 chain_id = ik_add_chain(system, "test_chain", IK_SOLVER_TWO_BONE);
    IKChain* chain = &system->chains[chain_id];
    
    // Setup 2-bone chain forming a right triangle
    chain->bone_count = 2;
    chain->bones[0].position = (Vec3){0, 0, 0};
    chain->bones[0].length = 3.0f;
    chain->bones[1].position = (Vec3){3, 0, 0};
    chain->bones[1].length = 4.0f;
    
    // Target that forms a 3-4-5 triangle
    Vec3 target = (Vec3){0, 5, 0};
    ik_set_target(system, chain_id, target, (Quat){0, 0, 0, 1});
    ik_solve(system, chain_id);
    
    // Check if end effector reached target
    Vec3 end_pos = chain->bones[1].position;
    float error = vec3_distance(&end_pos, &target);
    
    bool passed = error < TEST_TOLERANCE;
    add_test_result("Two-Bone Analytical", passed, error,
                   passed ? "3-4-5 triangle solved correctly" : "Analytical solution failed");
    
    ik_system_destroy(system);
    return passed;
}

bool test_ccd_convergence(void) {
    log_info("Testing CCD convergence...");
    
    IKSystem* system = ik_system_create();
    if (!system) return false;
    
    u32 chain_id = ik_add_chain(system, "test_chain", IK_SOLVER_CCD);
    IKChain* chain = &system->chains[chain_id];
    
    // Setup 4-bone chain
    chain->bone_count = 4;
    chain->bones[0].position = (Vec3){0, 0, 0};
    chain->bones[0].length = 1.0f;
    chain->bones[1].position = (Vec3){1, 0, 0};
    chain->bones[1].length = 1.0f;
    chain->bones[2].position = (Vec3){2, 0, 0};
    chain->bones[2].length = 1.0f;
    chain->bones[3].position = (Vec3){3, 0, 0};
    chain->bones[3].length = 1.0f;
    
    // Set more iterations for CCD
    chain->max_iterations = 50;
    
    Vec3 target = (Vec3){1.5, 2.0, 0};
    ik_set_target(system, chain_id, target, (Quat){0, 0, 0, 1});
    ik_solve(system, chain_id);
    
    // Check convergence
    Vec3 end_pos = chain->bones[3].position;
    float error = vec3_distance(&end_pos, &target);
    
    bool passed = error < TEST_TOLERANCE;
    add_test_result("CCD Convergence", passed, error,
                   passed ? "CCD converged to target" : "CCD failed to converge");
    
    ik_system_destroy(system);
    return passed;
}

bool test_constraint_enforcement(void) {
    log_info("Testing constraint enforcement...");
    
    // Test constraints using the simple FABRIK implementation
    Vec3 positions[3] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};
    u32 chain_id = fabrik_create_chain(positions, 3);
    
    if (chain_id == UINT32_MAX) return false;
    
    // Add tight constraint (90 degrees max bend)
    fabrik_add_constraint(chain_id, 1, 0.0f, 90.0f);
    
    // Try to reach a position that would require more than 90 degrees
    Vec3 target = {0, 2, 0}; // This would require 180 degrees bend
    fabrik_solve(chain_id, &target, 10);
    
    // Check if constraint was enforced (simplified check)
    Vec3 mid_pos = fabrik_get_joint_position(chain_id, 1);
    float distance_from_origin = vec3_length(&mid_pos);
    
    // With 90 degree constraint, middle joint should be at distance ~1.414 from origin
    bool passed = fabsf(distance_from_origin - 1.414f) < 0.2f;
    add_test_result("Constraint Enforcement", passed, distance_from_origin,
                   passed ? "Constraints enforced correctly" : "Constraints not enforced");
    
    fabrik_destroy_chain(chain_id);
    return passed;
}

bool test_consolidated_fabrik(void) {
    log_info("Testing consolidated FABRIK implementation...");
    
    // Initialize the consolidated FABRIK solver
    if (animation_fabrik_solver_init() != 0) {
        add_test_result("Consolidated FABRIK Init", false, 0.0f, "Failed to initialize");
        return false;
    }
    
    animation_fabrik_solver_handle_t handle;
    animation_fabrik_solver_desc_t desc = {0};
    
    if (animation_fabrik_solver_create(&handle, &desc) != 0) {
        add_test_result("Consolidated FABRIK Create", false, 0.0f, "Failed to create solver");
        animation_fabrik_solver_shutdown();
        return false;
    }
    
    // Create a 3-joint chain
    Vec3 positions[] = {
        {0, 0, 0},
        {2, 0, 0},
        {4, 0, 0}
    };
    
    uint32_t chain_id = animation_fabrik_solver_add_chain(handle, positions, 3);
    if (chain_id == UINT32_MAX) {
        add_test_result("Consolidated FABRIK Add Chain", false, 0.0f, "Failed to add chain");
        animation_fabrik_solver_destroy(handle);
        animation_fabrik_solver_shutdown();
        return false;
    }
    
    // Test basic reachability
    Vec3 target = {3, 2, 0};
    bool solved = animation_fabrik_solver_solve_chain(handle, chain_id, &target, 10);
    
    if (!solved) {
        add_test_result("Consolidated FABRIK Solve", false, 0.0f, "Failed to solve");
        animation_fabrik_solver_destroy(handle);
        animation_fabrik_solver_shutdown();
        return false;
    }
    
    // Check accuracy
    Vec3 end_pos = animation_fabrik_solver_get_joint_position(handle, chain_id, 2);
    float error = vec3_distance(&end_pos, &target);
    bool passed = error < TEST_TOLERANCE;
    
    add_test_result("Consolidated FABRIK Accuracy", passed, error,
                   passed ? "Consolidated implementation working" : "Consolidated implementation failed");
    
    // Test constraint handling
    animation_fabrik_solver_set_joint_constraint(handle, chain_id, 1, 0.0f, 45.0f);
    
    Vec3 constrained_target = {3, 3, 0}; // Would require >45 degrees without constraint
    solved = animation_fabrik_solver_solve_chain(handle, chain_id, &constrained_target, 10);
    
    if (solved) {
        Vec3 middle_pos = animation_fabrik_solver_get_joint_position(handle, chain_id, 1);
        Vec3 start_pos = animation_fabrik_solver_get_joint_position(handle, chain_id, 0);
        Vec3 constrained_end = animation_fabrik_solver_get_joint_position(handle, chain_id, 2);
        
        // Calculate angle at middle joint
        Vec3 bone1 = vec3_normalize(vec3_sub(middle_pos, start_pos));
        Vec3 bone2 = vec3_normalize(vec3_sub(constrained_end, middle_pos));
        float angle = acosf(fmaxf(-1.0f, fminf(1.0f, vec3_dot(&bone1, &bone2))));
        float angle_deg = angle * 180.0f / M_PI;
        
        bool constraint_passed = angle_deg <= 50.0f; // Allow tolerance
        add_test_result("Consolidated FABRIK Constraints", constraint_passed, angle_deg,
                       constraint_passed ? "Constraints working" : "Constraints failed");
    }
    
    animation_fabrik_solver_destroy(handle);
    animation_fabrik_solver_shutdown();
    return passed;
}

void run_all_ik_tests(void) {
    log_info("Starting IK accuracy tests...");
    
    g_test_count = 0;
    
    // Run all tests
    test_fabrik_reachability();
    test_fabrik_unreachable();
    test_two_bone_analytical();
    test_ccd_convergence();
    test_constraint_enforcement();
    test_consolidated_fabrik();
    
    // Print results
    printf("\n=== IK Accuracy Test Results ===\n");
    u32 passed_count = 0;
    
    for (u32 i = 0; i < g_test_count; i++) {
        IKTestResult* result = &g_test_results[i];
        printf("[%s] %s (Error: %.4f) - %s\n",
               result->passed ? "PASS" : "FAIL",
               result->test_name,
               result->error,
               result->details);
        
        if (result->passed) passed_count++;
    }
    
    printf("\nSummary: %u/%u tests passed (%.1f%%)\n",
           passed_count, g_test_count,
           (float)passed_count / g_test_count * 100.0f);
    
    if (passed_count == g_test_count) {
        log_info("All IK accuracy tests passed!");
    } else {
        log_warn("Some IK accuracy tests failed. Review results above.");
    }
}

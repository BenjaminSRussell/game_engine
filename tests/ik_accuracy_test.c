/*
 * ik_accuracy_test.c
 * Comprehensive IK Solver Accuracy Testing
 * 
 * Tests all IK solvers for:
 * - Positional accuracy
 * - Convergence speed
 * - Edge case handling
 * - Performance characteristics
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include "src/character/animation/ik/ik_consolidated.h"

/* Test configuration */
#define TEST_ITERATIONS 1000
#define TOLERANCE 0.001f
#define MAX_CHAIN_JOINTS 8
#define TEST_SPACE_SIZE 10.0f

/* Test statistics */
typedef struct {
    uint32_t total_tests;
    uint32_t successful_tests;
    uint32_t converged_tests;
    double total_error;
    double max_error;
    double min_error;
    double total_time;
    double max_time;
    double min_time;
} TestStats;

/* Utility functions */
static float random_float(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

static Vec3 random_vec3(float range) {
    return (Vec3){
        random_float(-range, range),
        random_float(-range, range),
        random_float(-range, range)
    };
}

static void init_stats(TestStats* stats) {
    stats->total_tests = 0;
    stats->successful_tests = 0;
    stats->converged_tests = 0;
    stats->total_error = 0.0;
    stats->max_error = 0.0;
    stats->min_error = INFINITY;
    stats->total_time = 0.0;
    stats->max_time = 0.0;
    stats->min_time = INFINITY;
}

static void update_stats(TestStats* stats, bool success, bool converged, float error, double time) {
    stats->total_tests++;
    if (success) stats->successful_tests++;
    if (converged) stats->converged_tests++;
    stats->total_error += error;
    stats->max_error = fmax(stats->max_error, error);
    stats->min_error = fmin(stats->min_error, error);
    stats->total_time += time;
    stats->max_time = fmax(stats->max_time, time);
    stats->min_time = fmin(stats->min_time, time);
}

static void print_stats(const char* test_name, const TestStats* stats) {
    printf("\n=== %s Results ===\n", test_name);
    printf("Total tests: %u\n", stats->total_tests);
    printf("Successful: %u (%.1f%%)\n", stats->successful_tests, 
           (stats->successful_tests * 100.0f) / stats->total_tests);
    printf("Converged: %u (%.1f%%)\n", stats->converged_tests,
           (stats->converged_tests * 100.0f) / stats->total_tests);
    printf("Average error: %.6f\n", stats->total_error / stats->total_tests);
    printf("Min error: %.6f\n", stats->min_error);
    printf("Max error: %.6f\n", stats->max_error);
    printf("Average time: %.6f ms\n", stats->total_time / stats->total_tests);
    printf("Min time: %.6f ms\n", stats->min_time);
    printf("Max time: %.6f ms\n", stats->max_time);
}

/* Test cases */

static void test_fabrik_accuracy(void) {
    printf("\n=== Testing FABRIK IK Accuracy ===\n");
    TestStats stats;
    init_stats(&stats);
    
    srand((unsigned int)time(NULL));
    
    for (uint32_t i = 0; i < TEST_ITERATIONS; i++) {
        // Create a random chain
        uint32_t joint_count = 3 + (rand() % (MAX_CHAIN_JOINTS - 2));
        Vec3 positions[MAX_CHAIN_JOINTS];
        positions[0] = (Vec3){0, 0, 0}; // Root at origin
        
        float total_length = 0.0f;
        for (uint32_t j = 1; j < joint_count; j++) {
            positions[j] = (Vec3){
                positions[j-1].x + random_float(0.5f, 2.0f),
                positions[j-1].y + random_float(-0.5f, 0.5f),
                positions[j-1].z + random_float(-0.5f, 0.5f)
            };
            total_length += vec3_distance(&positions[j-1], &positions[j]);
        }
        
        uint32_t chain_id = ik_create_chain(IK_SOLVER_FABRIK, positions, joint_count);
        if (chain_id == UINT32_MAX) continue;
        
        // Generate reachable target
        float target_distance = random_float(0.1f, total_length * 0.9f);
        Vec3 direction = vec3_normalize(random_vec3(1.0f));
        Vec3 target = vec3_scale(&direction, target_distance);
        
        // Time the solve
        clock_t start = clock();
        IKSolveResult result = ik_solve_to_target(chain_id, &target);
        clock_t end = clock();
        double solve_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        
        // Calculate actual error
        Vec3 end_effector = ik_get_joint_position(chain_id, joint_count - 1);
        float error = vec3_distance(&end_effector, &target);
        
        update_stats(&stats, result.success, error < TOLERANCE, error, solve_time);
        
        ik_reset_chain(chain_id);
    }
    
    print_stats("FABRIK Accuracy", &stats);
}

static void test_convergence_speed(void) {
    printf("\n=== Testing IK Convergence Speed ===\n");
    TestStats stats;
    init_stats(&stats);
    
    // Create a standard test chain
    Vec3 positions[4] = {
        {0, 0, 0},
        {1, 0, 0},
        {2, 0, 0},
        {3, 0, 0}
    };
    
    uint32_t chain_id = ik_create_chain(IK_SOLVER_FABRIK, positions, 4);
    if (chain_id == UINT32_MAX) return;
    
    // Test with different iteration counts
    int iteration_counts[] = {1, 2, 5, 10, 20, 50};
    int num_tests = sizeof(iteration_counts) / sizeof(iteration_counts[0]);
    
    for (int i = 0; i < num_tests; i++) {
        Vec3 target = {2.5f, 1.0f, 0.5f};
        
        clock_t start = clock();
        IKSolveResult result = ik_solve_with_iterations(chain_id, &target, iteration_counts[i]);
        clock_t end = clock();
        double solve_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        
        Vec3 end_effector = ik_get_joint_position(chain_id, 3);
        float error = vec3_distance(&end_effector, &target);
        
        printf("Iterations %d: error=%.6f, time=%.6f ms, success=%s\n",
               iteration_counts[i], error, solve_time, result.success ? "true" : "false");
        
        update_stats(&stats, result.success, error < TOLERANCE, error, solve_time);
        ik_reset_chain(chain_id);
    }
    
    print_stats("Convergence Speed", &stats);
}

static void test_edge_cases(void) {
    printf("\n=== Testing IK Edge Cases ===\n");
    TestStats stats;
    init_stats(&stats);
    
    // Test 1: Very short chain
    Vec3 short_chain[2] = {{0, 0, 0}, {0.1f, 0, 0}};
    uint32_t short_id = ik_create_chain(IK_SOLVER_FABRIK, short_chain, 2);
    if (short_id != UINT32_MAX) {
        Vec3 target = {0.05f, 0, 0};
        clock_t start = clock();
        IKSolveResult result = ik_solve_to_target(short_id, &target);
        clock_t end = clock();
        double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        float error = vec3_distance(&ik_get_joint_position(short_id, 1), &target);
        update_stats(&stats, result.success, error < TOLERANCE, error, time);
        printf("Short chain: error=%.6f, success=%s\n", error, result.success ? "true" : "false");
    }
    
    // Test 2: Unreachable target
    Vec3 long_chain[3] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};
    uint32_t long_id = ik_create_chain(IK_SOLVER_FABRIK, long_chain, 3);
    if (long_id != UINT32_MAX) {
        Vec3 unreachable_target = {10, 0, 0}; // Far beyond chain length
        clock_t start = clock();
        IKSolveResult result = ik_solve_to_target(long_id, &unreachable_target);
        clock_t end = clock();
        double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        float error = vec3_distance(&ik_get_joint_position(long_id, 2), &unreachable_target);
        update_stats(&stats, result.success, false, error, time); // Expected to fail
        printf("Unreachable target: error=%.6f, success=%s\n", error, result.success ? "true" : "false");
    }
    
    // Test 3: Target at root position
    Vec3 root_chain[3] = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}};
    uint32_t root_id = ik_create_chain(IK_SOLVER_FABRIK, root_chain, 3);
    if (root_id != UINT32_MAX) {
        Vec3 root_target = {0, 0, 0};
        clock_t start = clock();
        IKSolveResult result = ik_solve_to_target(root_id, &root_target);
        clock_t end = clock();
        double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        float error = vec3_distance(&ik_get_joint_position(root_id, 2), &root_target);
        update_stats(&stats, result.success, error < TOLERANCE, error, time);
        printf("Target at root: error=%.6f, success=%s\n", error, result.success ? "true" : "false");
    }
    
    print_stats("Edge Cases", &stats);
}

static void test_performance(void) {
    printf("\n=== Testing IK Performance ===\n");
    TestStats stats;
    init_stats(&stats);
    
    // Create multiple chains for batch testing
    uint32_t chain_ids[10];
    for (int i = 0; i < 10; i++) {
        Vec3 positions[4] = {
            {i * 3.0f, 0, 0},
            {i * 3.0f + 1.0f, 0, 0},
            {i * 3.0f + 2.0f, 0, 0},
            {i * 3.0f + 3.0f, 0, 0}
        };
        chain_ids[i] = ik_create_chain(IK_SOLVER_FABRIK, positions, 4);
    }
    
    // Batch solve test
    clock_t start = clock();
    for (int batch = 0; batch < 100; batch++) {
        for (int i = 0; i < 10; i++) {
            Vec3 target = {
                i * 3.0f + 1.5f + random_float(-0.5f, 0.5f),
                random_float(-1.0f, 1.0f),
                random_float(-1.0f, 1.0f)
            };
            IKSolveResult result = ik_solve_to_target(chain_ids[i], &target);
            Vec3 end_effector = ik_get_joint_position(chain_ids[i], 3);
            float error = vec3_distance(&end_effector, &target);
            update_stats(&stats, result.success, error < TOLERANCE, error, 0.0);
        }
    }
    clock_t end = clock();
    double total_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("Total solves: %u\n", stats.total_tests);
    printf("Total time: %.3f ms\n", total_time);
    printf("Average time per solve: %.6f ms\n", total_time / stats.total_tests);
    printf("Solves per second: %.0f\n", stats.total_tests / (total_time / 1000.0));
    
    print_stats("Performance", &stats);
}

static void test_two_bone_ik(void) {
    printf("\n=== Testing Two-Bone IK ===\n");
    TestStats stats;
    init_stats(&stats);
    
    for (uint32_t i = 0; i < TEST_ITERATIONS / 4; i++) {
        // Create two-bone chain
        float l1 = random_float(1.0f, 3.0f);
        float l2 = random_float(1.0f, 3.0f);
        Vec3 positions[3] = {
            {0, 0, 0},
            {l1, 0, 0},
            {l1 + l2, 0, 0}
        };
        
        uint32_t chain_id = ik_create_chain(IK_SOLVER_TWO_BONE, positions, 3);
        if (chain_id == UINT32_MAX) continue;
        
        // Generate reachable target
        float max_reach = l1 + l2;
        float target_dist = random_float(0.1f, max_reach * 0.9f);
        Vec3 direction = vec3_normalize(random_vec3(1.0f));
        Vec3 target = vec3_scale(&direction, target_dist);
        Vec3 pole_vector = {0, 1, 0}; // Up vector for elbow orientation
        
        clock_t start = clock();
        IKSolveResult result = ik_solve_to_target(chain_id, &target);
        clock_t end = clock();
        double solve_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        
        Vec3 end_effector = ik_get_joint_position(chain_id, 2);
        float error = vec3_distance(&end_effector, &target);
        
        update_stats(&stats, result.success, error < TOLERANCE, error, solve_time);
    }
    
    print_stats("Two-Bone IK", &stats);
}

/* Main test runner */
int main(void) {
    printf("IK Solver Accuracy Test Suite\n");
    printf("============================\n");
    
    // Initialize IK system
    if (ik_system_init() != 0) {
        printf("Failed to initialize IK system\n");
        return 1;
    }
    
    // Run all tests
    test_fabrik_accuracy();
    test_convergence_speed();
    test_edge_cases();
    test_performance();
    test_two_bone_ik();
    
    // Print final statistics
    printf("\n=== Final IK System Statistics ===\n");
    ik_debug_print_solver_stats();
    
    // Cleanup
    ik_system_shutdown();
    
    printf("\nIK accuracy testing completed.\n");
    return 0;
}

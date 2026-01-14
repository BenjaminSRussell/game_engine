/*
 * ik_simple_test.c
 * Simple IK Solver Test
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdbool.h>

// Simple Vec3 implementation for testing
typedef struct {
    float x, y, z;
} Vec3;

static Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

static Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

static Vec3 vec3_scale(Vec3 v, float s) {
    return (Vec3){v.x * s, v.y * s, v.z * s};
}

static float vec3_length(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static float vec3_distance(Vec3 a, Vec3 b) {
    Vec3 diff = vec3_sub(a, b);
    return vec3_length(diff);
}

static Vec3 vec3_normalize(Vec3 v) {
    float len = vec3_length(v);
    if (len < 1e-6f) return (Vec3){0, 1, 0};
    return vec3_scale(v, 1.0f / len);
}

// Simple FABRIK implementation
typedef struct {
    Vec3 positions[8];
    float lengths[7];
    uint32_t joint_count;
    float total_length;
} SimpleChain;

static bool simple_fabrik_solve(SimpleChain* chain, Vec3 target, int max_iterations, float tolerance) {
    Vec3 root = chain->positions[0];
    
    // Check if target is reachable
    float target_distance = vec3_distance(root, target);
    if (target_distance > chain->total_length) {
        // Stretch towards target
        Vec3 direction = vec3_normalize(vec3_sub(target, root));
        float accumulated_length = 0.0f;
        for (uint32_t i = 1; i < chain->joint_count; i++) {
            accumulated_length += chain->lengths[i-1];
            chain->positions[i] = vec3_add(root, vec3_scale(direction, accumulated_length));
        }
        return false;
    }
    
    // FABRIK iterations
    for (int iter = 0; iter < max_iterations; iter++) {
        // Forward reaching
        chain->positions[chain->joint_count - 1] = target;
        for (int i = chain->joint_count - 2; i >= 0; i--) {
            Vec3 direction = vec3_normalize(vec3_sub(chain->positions[i], chain->positions[i + 1]));
            chain->positions[i] = vec3_add(chain->positions[i + 1], vec3_scale(direction, chain->lengths[i]));
        }
        
        // Backward reaching
        chain->positions[0] = root;
        for (uint32_t i = 1; i < chain->joint_count; i++) {
            Vec3 direction = vec3_normalize(vec3_sub(chain->positions[i], chain->positions[i - 1]));
            chain->positions[i] = vec3_add(chain->positions[i - 1], vec3_scale(direction, chain->lengths[i - 1]));
        }
        
        // Check convergence
        float error = vec3_distance(chain->positions[chain->joint_count - 1], target);
        if (error < tolerance) {
            return true;
        }
    }
    
    return false;
}

static void test_fabrik_accuracy(void) {
    printf("=== Testing FABRIK IK Accuracy ===\n");
    
    // Create a test chain
    SimpleChain chain;
    chain.joint_count = 4;
    chain.positions[0] = (Vec3){0, 0, 0};
    chain.positions[1] = (Vec3){1, 0, 0};
    chain.positions[2] = (Vec3){2, 0, 0};
    chain.positions[3] = (Vec3){3, 0, 0};
    
    chain.lengths[0] = 1.0f;
    chain.lengths[1] = 1.0f;
    chain.lengths[2] = 1.0f;
    chain.total_length = 3.0f;
    
    // Test cases
    struct {
        Vec3 target;
        bool expected_reachable;
        const char* description;
    } test_cases[] = {
        {{1.5f, 1.0f, 0.0f}, true, "Reachable target above chain"},
        {{2.5f, 0.5f, 0.5f}, true, "Reachable diagonal target"},
        {{3.0f, 0.0f, 0.0f}, true, "Target at maximum reach"},
        {{4.0f, 0.0f, 0.0f}, false, "Unreachable target beyond reach"},
        {{0.0f, 0.0f, 0.0f}, true, "Target at root position"}
    };
    
    int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);
    int successful_tests = 0;
    double total_error = 0.0;
    
    for (int i = 0; i < num_tests; i++) {
        // Reset chain
        chain.positions[0] = (Vec3){0, 0, 0};
        chain.positions[1] = (Vec3){1, 0, 0};
        chain.positions[2] = (Vec3){2, 0, 0};
        chain.positions[3] = (Vec3){3, 0, 0};
        
        printf("\nTest %d: %s\n", i + 1, test_cases[i].description);
        printf("Target: (%.2f, %.2f, %.2f)\n", 
               test_cases[i].target.x, test_cases[i].target.y, test_cases[i].target.z);
        
        clock_t start = clock();
        bool success = simple_fabrik_solve(&chain, test_cases[i].target, 20, 0.001f);
        clock_t end = clock();
        double solve_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        
        Vec3 end_effector = chain.positions[chain.joint_count - 1];
        float error = vec3_distance(end_effector, test_cases[i].target);
        
        printf("Result: %s, Error: %.6f, Time: %.6f ms\n",
               success ? "SUCCESS" : "FAILED", error, solve_time);
        printf("End effector: (%.3f, %.3f, %.3f)\n",
               end_effector.x, end_effector.y, end_effector.z);
        
        if (success == test_cases[i].expected_reachable) {
            successful_tests++;
        }
        total_error += error;
    }
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", num_tests);
    printf("Successful: %d (%.1f%%)\n", successful_tests, 
           (successful_tests * 100.0f) / num_tests);
    printf("Average error: %.6f\n", total_error / num_tests);
}

static void test_convergence_speed(void) {
    printf("\n=== Testing Convergence Speed ===\n");
    
    SimpleChain chain;
    chain.joint_count = 4;
    chain.positions[0] = (Vec3){0, 0, 0};
    chain.positions[1] = (Vec3){1, 0, 0};
    chain.positions[2] = (Vec3){2, 0, 0};
    chain.positions[3] = (Vec3){3, 0, 0};
    
    chain.lengths[0] = 1.0f;
    chain.lengths[1] = 1.0f;
    chain.lengths[2] = 1.0f;
    chain.total_length = 3.0f;
    
    Vec3 target = {2.5f, 1.0f, 0.0f};
    
    int iteration_counts[] = {1, 2, 5, 10, 20};
    int num_tests = sizeof(iteration_counts) / sizeof(iteration_counts[0]);
    
    for (int i = 0; i < num_tests; i++) {
        // Reset chain
        chain.positions[0] = (Vec3){0, 0, 0};
        chain.positions[1] = (Vec3){1, 0, 0};
        chain.positions[2] = (Vec3){2, 0, 0};
        chain.positions[3] = (Vec3){3, 0, 0};
        
        clock_t start = clock();
        bool success = simple_fabrik_solve(&chain, target, iteration_counts[i], 0.001f);
        clock_t end = clock();
        double solve_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        
        Vec3 end_effector = chain.positions[chain.joint_count - 1];
        float error = vec3_distance(end_effector, target);
        
        printf("Iterations %d: error=%.6f, time=%.6f ms, success=%s\n",
               iteration_counts[i], error, solve_time, success ? "true" : "false");
    }
}

static void test_performance(void) {
    printf("\n=== Testing Performance ===\n");
    
    const int num_solves = 10000;
    SimpleChain chain;
    chain.joint_count = 4;
    chain.positions[0] = (Vec3){0, 0, 0};
    chain.positions[1] = (Vec3){1, 0, 0};
    chain.positions[2] = (Vec3){2, 0, 0};
    chain.positions[3] = (Vec3){3, 0, 0};
    
    chain.lengths[0] = 1.0f;
    chain.lengths[1] = 1.0f;
    chain.lengths[2] = 1.0f;
    chain.total_length = 3.0f;
    
    clock_t start = clock();
    int successful = 0;
    
    for (int i = 0; i < num_solves; i++) {
        // Reset chain
        chain.positions[0] = (Vec3){0, 0, 0};
        chain.positions[1] = (Vec3){1, 0, 0};
        chain.positions[2] = (Vec3){2, 0, 0};
        chain.positions[3] = (Vec3){3, 0, 0};
        
        // Random target
        float angle = ((float)i / num_solves) * 2.0f * M_PI;
        Vec3 target = {
            cosf(angle) * 2.0f,
            sinf(angle) * 2.0f,
            0.0f
        };
        
        if (simple_fabrik_solve(&chain, target, 10, 0.001f)) {
            successful++;
        }
    }
    
    clock_t end = clock();
    double total_time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("Total solves: %d\n", num_solves);
    printf("Successful: %d (%.1f%%)\n", successful, (successful * 100.0f) / num_solves);
    printf("Total time: %.3f ms\n", total_time);
    printf("Average time per solve: %.6f ms\n", total_time / num_solves);
    printf("Solves per second: %.0f\n", num_solves / (total_time / 1000.0));
}

int main(void) {
    printf("IK Solver Simple Test Suite\n");
    printf("==========================\n");
    
    test_fabrik_accuracy();
    test_convergence_speed();
    test_performance();
    
    printf("\nIK testing completed.\n");
    return 0;
}

/**
 * @file physics_test.c
 * @brief Test program for consolidated physics system
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

// Include the consolidated physics system
#include "src/engine/physics/solver/physics_solver_system.h"
#include "src/engine/physics/solver/xpbd_solver.h"
#include "src/engine/physics/solver/sequential_impulse.h"

int test_unified_physics_solver() {
    printf("Testing Unified Physics Solver...\n");
    
    // Create default configuration
    physics_solver_config config = physics_solver_get_default_config();
    printf("✓ Default config created\n");
    
    // Create solver system
    physics_solver_system *system = physics_solver_create(&config);
    if (!system) {
        printf("✗ Failed to create physics solver system\n");
        return 0;
    }
    printf("✓ Physics solver system created\n");
    
    // Test validation
    if (!physics_solver_validate(system)) {
        printf("✗ Physics solver validation failed\n");
        physics_solver_destroy(system);
        return 0;
    }
    printf("✓ Physics solver validation passed\n");
    
    // Test gravity setting
    float gravity[3] = {0.0f, -9.81f, 0.0f};
    physics_solver_set_gravity(system, gravity);
    printf("✓ Gravity set to (%.2f, %.2f, %.2f)\n", gravity[0], gravity[1], gravity[2]);
    
    // Create test rigid body
    RigidBody test_body = {
        .position = {0.0f, 10.0f, 0.0f},
        .orientation = {0.0f, 0.0f, 0.0f, 1.0f},
        .linear_velocity = {1.0f, 0.0f, 0.0f},
        .angular_velocity = {0.0f, 0.0f, 0.0f},
        .mass = 1.0f,
        .inertia = 1.0f,
        .restitution = 0.5f,
        .static_friction = 0.5f,
        .kinetic_friction = 0.3f,
        .is_static = false
    };
    
    int body_id = physics_solver_add_rigid_body(system, &test_body);
    if (body_id < 0) {
        printf("✗ Failed to add rigid body\n");
        physics_solver_destroy(system);
        return 0;
    }
    printf("✓ Rigid body added with ID %d\n", body_id);
    
    // Test simulation step
    physics_solver_step(system, 1.0f / 60.0f);
    printf("✓ Simulation step completed\n");
    
    // Get statistics
    physics_solver_stats stats;
    physics_solver_get_stats(system, &stats);
    printf("✓ Statistics: %d bodies, %d contacts, %.3f ms solve time\n", 
           stats.rigid_body_count, stats.contact_count, stats.solve_time_ms);
    
    // Cleanup
    physics_solver_destroy(system);
    printf("✓ Physics solver system destroyed\n");
    
    return 1;
}

int main() {
    printf("=== Physics System Consolidation Test ===\n");
    
    int passed = 0;
    int total = 1;
    
    if (test_unified_physics_solver()) passed++;
    
    printf("\n=== Test Results ===\n");
    printf("Passed: %d/%d tests\n", passed, total);
    
    if (passed == total) {
        printf("✓ All physics consolidation tests PASSED!\n");
        return 0;
    } else {
        printf("✗ Some physics consolidation tests FAILED!\n");
        return 1;
    }
}
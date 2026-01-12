/**
 * @file physics_integration_tests.h
 * @brief Physics system integration tests
 *
 * Comprehensive test suite for the physics pipeline including collision
 * detection, constraint solving, and specialized physics systems.
 */

#ifndef PHYSICS_INTEGRATION_TESTS_H
#define PHYSICS_INTEGRATION_TESTS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Test Configuration
// ========================================

/**
 * Run all physics integration tests
 * @param verbose Enable verbose output
 * @return True if all tests pass
 */
bool physics_run_all_integration_tests(bool verbose);

/**
 * Run performance benchmarks for physics systems
 * @return True if benchmarks complete successfully
 */
bool physics_run_performance_benchmarks(void);

/**
 * Generate comprehensive test report
 * @param filename Output file for the report
 */
void physics_generate_test_report(const char *filename);

// ========================================
// Individual Test Categories
// ========================================

/**
 * Test broadphase collision detection systems
 * @return True if tests pass
 */
bool physics_test_broadphase_systems(void);

/**
 * Test narrowphase collision detection systems
 * @return True if tests pass
 */
bool physics_test_narrowphase_systems(void);

/**
 * Test constraint solving systems
 * @return True if tests pass
 */
bool physics_test_constraint_solvers(void);

/**
 * Test cloth physics simulation
 * @return True if tests pass
 */
bool physics_test_cloth_physics(void);

/**
 * Test particle physics systems
 * @return True if tests pass
 */
bool physics_test_particle_physics(void);

/**
 * Test deformable body systems
 * @return True if tests pass
 */
bool physics_test_deformable_bodies(void);

/**
 * Test fluid dynamics systems
 * @return True if tests pass
 */
bool physics_test_fluid_dynamics(void);

/**
 * Test destruction systems
 * @return True if tests pass
 */
bool physics_test_destruction_systems(void);

/**
 * Test performance profiling
 * @return True if tests pass
 */
bool physics_test_performance_profiling(void);

// ========================================
// Stress Testing
// ========================================

/**
 * Run stress tests for physics systems
 * @param duration_ms Test duration in milliseconds
 * @return True if stress tests pass
 */
bool physics_run_stress_tests(int duration_ms);

/**
 * Test physics system with large numbers of objects
 * @param object_count Number of objects to simulate
 * @return True if large-scale test passes
 */
bool physics_test_large_scale_simulation(int object_count);

/**
 * Test physics system memory usage
 * @return True if memory usage is acceptable
 */
bool physics_test_memory_usage(void);

// ========================================
// Regression Testing
// ========================================

/**
 * Run regression tests for known issues
 * @return True if regression tests pass
 */
bool physics_run_regression_tests(void);

/**
 * Validate physics system against reference implementation
 * @return True if validation passes
 */
bool physics_validate_against_reference(void);

// ========================================
// Test Utilities
// ========================================

/**
 * Initialize test environment
 * @return True if initialization succeeds
 */
bool physics_tests_initialize(void);

/**
 * Cleanup test environment
 */
void physics_tests_cleanup(void);

/**
 * Set test configuration parameters
 * @param test_timeout_ms Test timeout in milliseconds
 * @param max_objects Maximum objects for stress tests
 * @param enable_verbose_logging Enable verbose logging
 */
void physics_tests_set_config(int test_timeout_ms, int max_objects, bool enable_verbose_logging);

/**
 * Get test execution statistics
 * @param total_tests Total number of tests run
 * @param passed_tests Number of tests that passed
 * @param failed_tests Number of tests that failed
 * @param total_time_ms Total execution time
 */
void physics_tests_get_stats(int *total_tests, int *passed_tests, int *failed_tests, float *total_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_INTEGRATION_TESTS_H */

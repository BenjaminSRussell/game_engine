/**
 * @file physics_integration_tests_extended.h
 * @brief Extended physics integration tests for gravity and collisions
 *
 * Comprehensive integration test suite for physics systems including
 * gravity simulation, collision detection, and physics stability tests.
 */

#ifndef PHYSICS_INTEGRATION_TESTS_EXTENDED_H
#define PHYSICS_INTEGRATION_TESTS_EXTENDED_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Physics Test Configuration
// ========================================

typedef struct PhysicsTestConfig {
    bool enable_verbose_output;         // Enable verbose test output
    bool enable_performance_tests;       // Enable performance benchmarks
    bool enable_stress_tests;           // Enable stress tests
    bool enable_visual_validation;      // Enable visual validation
    float gravity_strength;              // Gravity acceleration (m/s²)
    float time_step;                    // Physics time step (seconds)
    int max_substeps;                   // Maximum physics substeps
    float position_tolerance;            // Position tolerance for validation
    float velocity_tolerance;            // Velocity tolerance for validation
    float energy_tolerance;              // Energy tolerance for validation
    int stress_test_iterations;         // Number of stress test iterations
    const char *output_file;            // Output file for results
} PhysicsTestConfig;

// ========================================
// Physics Test Results
// ========================================

typedef struct PhysicsTestResults {
    uint32_t total_tests;               // Total tests run
    uint32_t passed_tests;              // Tests that passed
    uint32_t failed_tests;              // Tests that failed
    uint32_t skipped_tests;             // Tests that were skipped
    double total_time_ms;               // Total execution time
    double average_frame_time_ms;        // Average frame time
    double max_frame_time_ms;            // Maximum frame time
    double min_frame_time_ms;            // Minimum frame time
    char error_messages[4096];          // Accumulated error messages
    char performance_report[2048];      // Performance report
} PhysicsTestResults;

// ========================================
// Physics Test Objects
// ========================================

typedef struct PhysicsTestObject {
    uint32_t id;                        // Object ID
    float position[3];                  // Position (x, y, z)
    float velocity[3];                  // Velocity (vx, vy, vz)
    float acceleration[3];              // Acceleration (ax, ay, az)
    float mass;                         // Mass
    float radius;                       // Bounding sphere radius
    bool is_static;                     // Whether object is static
    bool is_active;                     // Whether object is active
    void *physics_handle;               // Internal physics object handle
} PhysicsTestObject;

// ========================================
// Test State
// ========================================

typedef struct PhysicsTestState {
    PhysicsTestObject *objects;         // Array of test objects
    uint32_t object_count;              // Number of objects
    uint32_t max_objects;               // Maximum objects
    float simulation_time;              // Current simulation time
    uint32_t frame_count;               // Frame counter
    PhysicsTestConfig config;           // Test configuration
    void *physics_world;                // Physics world handle
} PhysicsTestState;

// ========================================
// Global Test State
// ========================================

extern PhysicsTestConfig g_physics_test_config;
extern PhysicsTestResults g_physics_test_results;
extern PhysicsTestState g_physics_test_state;

// ========================================
// Test Suite Functions
// ========================================

/**
 * Initialize physics test suite
 * @param config Test configuration
 * @return True if initialization successful
 */
bool physics_integration_tests_init(const PhysicsTestConfig *config);

/**
 * Shutdown physics test suite
 * @param generate_report Whether to generate final report
 */
void physics_integration_tests_shutdown(bool generate_report);

/**
 * Run all physics integration tests
 * @return True if all tests pass
 */
bool physics_run_all_integration_tests(void);

/**
 * Get test results
 * @return Test results
 */
PhysicsTestResults physics_get_test_results(void);

/**
 * Print test summary
 */
void physics_print_test_summary(void);

/**
 * Export test results to file
 * @param filename Output filename
 * @return True if export successful
 */
bool physics_export_results(const char *filename);

// ========================================
// Gravity Tests
// ========================================

/**
 * Test basic gravity simulation
 * @return True if test passes
 */
bool test_gravity_basic_simulation(void);

/**
 * Test gravity with different masses
 * @return True if test passes
 */
bool test_gravity_mass_independence(void);

/**
 * Test gravity energy conservation
 * @return True if test passes
 */
bool test_gravity_energy_conservation(void);

/**
 * Test gravity with time step variations
 * @return True if test passes
 */
bool test_gravity_time_step_stability(void);

/**
 * Test gravity at different heights
 * @return True if test passes
 */
bool test_gravity_height_variation(void);

/**
 * Test gravity with initial velocities
 * @return True if test passes
 */
bool test_gravity_initial_velocity(void);

/**
 * Test gravity with air resistance
 * @return True if test passes
 */
bool test_gravity_air_resistance(void);

/**
 * Test gravity with multiple objects
 * @return True if test passes
 */
bool test_gravity_multiple_objects(void);

// ========================================
// Collision Tests
// ========================================

/**
 * Test sphere-sphere collision detection
 * @return True if test passes
 */
bool test_collision_sphere_sphere(void);

/**
 * Test sphere-plane collision detection
 * @return True if test passes
 */
bool test_collision_sphere_plane(void);

/**
 * Test box-box collision detection
 * @return True if test passes
 */
bool test_collision_box_box(void);

/**
 * Test collision response and impulse
 * @return True if test passes
 */
bool test_collision_response(void);

/**
 * Test collision with restitution
 * @return True if test passes
 */
bool test_collision_restitution(void);

/**
 * Test collision with friction
 * @return True if test passes
 */
bool test_collision_friction(void);

/**
 * Test collision stacking
 * @return True if test passes
 */
bool test_collision_stacking(void);

/**
 * Test collision with moving objects
 * @return True if test passes
 */
bool test_collision_moving_objects(void);

/**
 * Test collision with complex shapes
 * @return True if test passes
 */
bool test_collision_complex_shapes(void);

// ========================================
// Integration Tests
// ========================================

/**
 * Test falling object with ground collision
 * @return True if test passes
 */
bool test_integration_falling_object(void);

/**
 * Test projectile motion with gravity
 * @return True if test passes
 */
bool test_integration_projectile_motion(void);

/**
 * Test pendulum simulation
 * @return True if test passes
 */
bool test_integration_pendulum(void);

/**
 * Test spring-mass system
 * @return True if test passes
 */
bool test_integration_spring_mass(void);

/**
 * Test bouncing ball with gravity
 * @return True if test passes
 */
bool test_integration_bouncing_ball(void);

/**
 * Test rolling object on incline
 * @return True if test passes
 */
bool test_integration_rolling_incline(void);

/**
 * Test multi-body collision chain
 * @return True if test passes
 */
bool test_integration_collision_chain(void);

/**
 * Test ragdoll physics
 * @return True if test passes
 */
bool test_integration_ragdoll(void);

/**
 * Test cloth simulation with gravity
 * @return True if test passes
 */
bool test_integration_cloth_gravity(void);

// ========================================
// Performance Tests
// ========================================

/**
 * Performance test with many objects
 * @return True if test passes
 */
bool test_performance_many_objects(void);

/**
 * Performance test with many collisions
 * @return True if test passes
 */
bool test_performance_many_collisions(void);

/**
 * Performance test with complex geometry
 * @return True if test passes
 */
bool test_performance_complex_geometry(void);

/**
 * Performance test with different time steps
 * @return True if test passes
 */
bool test_performance_time_steps(void);

/**
 * Performance test with multi-threading
 * @return True if test passes
 */
bool test_performance_multithreading(void);

// ========================================
// Stress Tests
// ========================================

/**
 * Stress test with 1000 objects
 * @return True if test passes
 */
bool stress_test_1000_objects(void);

/**
 * Stress test with high collision frequency
 * @return True if test passes
 */
bool stress_test_high_collision_frequency(void);

/**
 * Stress test with extreme velocities
 * @return True if test passes
 */
bool stress_test_extreme_velocities(void);

/**
 * Stress test with tiny objects
 * @return True if test passes
 */
bool stress_test_tiny_objects(void);

/**
 * Stress test with large mass ratios
 * @return True if test passes
 */
bool stress_test_mass_ratios(void);

/**
 * Stress test with long simulation time
 * @return True if test passes
 */
bool stress_test_long_simulation(void);

// ========================================
// Stability Tests
// ========================================

/**
 * Test physics stability at 180 Hz (3x speed)
 * @return True if test passes
 */
bool test_stability_180hz(void);

/**
 * Test physics stability with variable time steps
 * @return True if test passes
 */
bool test_stability_variable_timestep(void);

/**
 * Test physics stability with numerical precision
 * @return True if test passes
 */
bool test_stability_numerical_precision(void);

/**
 * Test physics stability with edge cases
 * @return True if test passes
 */
bool test_stability_edge_cases(void);

// ========================================
// Utility Functions
// ========================================

/**
 * Create test physics object
 * @param position Initial position
 * @param velocity Initial velocity
 * @param mass Object mass
 * @param radius Object radius
 * @param is_static Whether object is static
 * @return Created object or NULL on failure
 */
PhysicsTestObject* physics_create_test_object(const float position[3], 
                                            const float velocity[3],
                                            float mass, float radius, 
                                            bool is_static);

/**
 * Destroy test physics object
 * @param object Object to destroy
 */
void physics_destroy_test_object(PhysicsTestObject *object);

/**
 * Update physics simulation
 * @param dt Time step
 * @return True if update successful
 */
bool physics_update_simulation(float dt);

/**
 * Get object position
 * @param object Object
 * @param position Output position
 */
void physics_get_object_position(const PhysicsTestObject *object, float position[3]);

/**
 * Get object velocity
 * @param object Object
 * @param velocity Output velocity
 */
void physics_get_object_velocity(const PhysicsTestObject *object, float velocity[3]);

/**
 * Set object position
 * @param object Object
 * @param position New position
 */
void physics_set_object_position(PhysicsTestObject *object, const float position[3]);

/**
 * Set object velocity
 * @param object Object
 * @param velocity New velocity
 */
void physics_set_object_velocity(PhysicsTestObject *object, const float velocity[3]);

/**
 * Apply force to object
 * @param object Object
 * @param force Force vector
 */
void physics_apply_force(PhysicsTestObject *object, const float force[3]);

/**
 * Apply impulse to object
 * @param object Object
 * @param impulse Impulse vector
 */
void physics_apply_impulse(PhysicsTestObject *object, const float impulse[3]);

/**
 * Check collision between two objects
 * @param obj1 First object
 * @param obj2 Second object
 * @return True if objects are colliding
 */
bool physics_check_collision(const PhysicsTestObject *obj1, const PhysicsTestObject *obj2);

/**
 * Calculate object kinetic energy
 * @param object Object
 * @return Kinetic energy
 */
float physics_calculate_kinetic_energy(const PhysicsTestObject *object);

/**
 * Calculate object potential energy
 * @param object Object
 * @param gravity Gravity strength
 * @return Potential energy
 */
float physics_calculate_potential_energy(const PhysicsTestObject *object, float gravity);

/**
 * Validate physics state
 * @return True if physics state is valid
 */
bool physics_validate_state(void);

/**
 * Reset physics simulation
 */
void physics_reset_simulation(void);

// ========================================
// Validation Functions
// ========================================

/**
 * Validate position within bounds
 * @param position Position to validate
 * @param min_bounds Minimum bounds
 * @param max_bounds Maximum bounds
 * @return True if position is valid
 */
bool physics_validate_position(const float position[3], 
                               const float min_bounds[3], 
                               const float max_bounds[3]);

/**
 * Validate velocity within reasonable limits
 * @param velocity Velocity to validate
 * @param max_speed Maximum allowed speed
 * @return True if velocity is valid
 */
bool physics_validate_velocity(const float velocity[3], float max_speed);

/**
 * Validate energy conservation
 * @param initial_energy Initial total energy
 * @param current_energy Current total energy
 * @param tolerance Energy tolerance
 * @return True if energy is conserved
 */
bool physics_validate_energy_conservation(float initial_energy, 
                                        float current_energy, 
                                        float tolerance);

/**
 * Validate collision response
 * @param obj1 First object
 * @param obj2 Second object
 * @param restitution Coefficient of restitution
 * @return True if collision response is valid
 */
bool physics_validate_collision_response(const PhysicsTestObject *obj1,
                                       const PhysicsTestObject *obj2,
                                       float restitution);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_INTEGRATION_TESTS_EXTENDED_H */

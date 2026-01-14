/**
 * @file physics_stability_test.h
 * @brief Physics stability test at high frequency (180 Hz)
 *
 * Comprehensive physics stability test to verify system stability
 * when running physics simulation at 3x normal speed (180 Hz).
 */

#ifndef PHYSICS_STABILITY_TEST_H
#define PHYSICS_STABILITY_TEST_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Physics Stability Test Configuration
// ========================================

typedef struct PhysicsStabilityTestConfig {
    bool enable_verbose_output;         // Enable verbose test output
    bool enable_detailed_logging;      // Enable detailed logging
    bool enable_energy_tracking;       // Enable energy conservation tracking
    bool enable_position_tracking;      // Enable position accuracy tracking
    float target_frequency;            // Target simulation frequency (Hz)
    float simulation_duration;          // Total simulation duration (seconds)
    float gravity_strength;             // Gravity acceleration (m/s²)
    float time_step;                   // Physics time step (seconds)
    int max_substeps;                  // Maximum physics substeps
    float position_tolerance;           // Position tolerance for validation
    float velocity_tolerance;           // Velocity tolerance for validation
    float energy_tolerance;            // Energy tolerance for validation
    uint32_t object_count;            // Number of physics objects
    bool enable_collisions;            // Enable collision detection
    bool enable_constraints;           // Enable constraint solving
    bool enable_sleeping;              // Enable object sleeping
    const char *output_file;            // Output file for results
} PhysicsStabilityTestConfig;

// ========================================
// Physics Stability Test Results
// ========================================

typedef struct PhysicsStabilityTestResults {
    uint32_t total_frames;             // Total frames simulated
    uint32_t successful_frames;         // Successful frames
    uint32_t failed_frames;             // Failed frames
    double total_time_ms;               // Total execution time
    double average_frame_time_ms;        // Average frame time
    double max_frame_time_ms;            // Maximum frame time
    double min_frame_time_ms;            // Minimum frame time
    double actual_frequency;             // Actual simulation frequency
    double frequency_variance;           // Frequency variance
    float max_position_error;           // Maximum position error detected
    float max_velocity_error;           // Maximum velocity error detected
    float max_energy_error;            // Maximum energy error detected
    uint32_t instability_events;        // Number of instability events
    uint32_t collision_events;         // Number of collision events
    uint32_t constraint_violations;    // Number of constraint violations
    bool test_passed;                   // Whether test passed
    char error_messages[4096];          // Accumulated error messages
    char stability_report[2048];       // Stability analysis report
} PhysicsStabilityTestResults;

// ========================================
// Physics Object Structure
// ========================================

typedef struct PhysicsObject {
    uint32_t id;                        // Unique object ID
    char name[64];                      // Object name
    float position[3];                  // Current position
    float velocity[3];                  // Current velocity
    float acceleration[3];              // Current acceleration
    float rotation[4];                  // Rotation (quaternion)
    float angular_velocity[3];           // Angular velocity
    float mass;                         // Object mass
    float radius;                       // Bounding sphere radius
    float restitution;                   // Coefficient of restitution
    float friction;                     // Coefficient of friction
    bool is_static;                     // Whether object is static
    bool is_sleeping;                   // Whether object is sleeping
    bool is_active;                     // Whether object is active
    uint32_t collision_count;            // Number of collisions
    float initial_energy;                // Initial total energy
    float current_energy;                // Current total energy
    float max_energy;                   // Maximum energy reached
    float min_energy;                   // Minimum energy reached
    float *position_history;             // Position history for tracking
    float *velocity_history;             // Velocity history for tracking
    uint32_t history_size;              // Size of history arrays
    uint32_t history_index;             // Current index in history
    uint64_t creation_time;              // Creation timestamp
    uint64_t last_update_time;           // Last update timestamp
} PhysicsObject;

// ========================================
// Collision Contact Structure
// ========================================

typedef struct CollisionContact {
    uint32_t object1_id;                // First object ID
    uint32_t object2_id;                // Second object ID
    float contact_point[3];              // Contact point
    float contact_normal[3];             // Contact normal
    float penetration_depth;              // Penetration depth
    float relative_velocity[3];          // Relative velocity at contact
    float impulse_magnitude;              // Impulse magnitude
    uint64_t contact_time;              // Contact timestamp
    bool is_valid;                      // Whether contact is valid
} CollisionContact;

// ========================================
// Stability Metrics Structure
// ========================================

typedef struct StabilityMetrics {
    double position_variance;             // Position variance over time
    double velocity_variance;             // Velocity variance over time
    double energy_variance;              // Energy variance over time
    double frequency_stability;           // Frequency stability metric
    double collision_stability;          // Collision stability metric
    double constraint_stability;         // Constraint stability metric
    uint32_t explosion_events;           // Number of explosion events
    uint32_t tunneling_events;          // Number of tunneling events
    uint32_t jitter_events;              // Number of jitter events
    float average_separation;             // Average object separation
    float max_separation;                // Maximum object separation
    float min_separation;                // Minimum object separation
} StabilityMetrics;

// ========================================
// Global Test State
// ========================================

extern PhysicsStabilityTestConfig g_physics_stability_test_config;
extern PhysicsStabilityTestResults g_physics_stability_test_results;

// ========================================
// Test Suite Functions
// ========================================

/**
 * Initialize physics stability test
 * @param config Test configuration
 * @return True if initialization successful
 */
bool physics_stability_test_init(const PhysicsStabilityTestConfig *config);

/**
 * Shutdown physics stability test
 * @param generate_report Whether to generate final report
 */
void physics_stability_test_shutdown(bool generate_report);

/**
 * Run physics stability test
 * @return True if test passes
 */
bool physics_stability_test_run(void);

/**
 * Get test results
 * @return Test results
 */
PhysicsStabilityTestResults physics_stability_test_get_results(void);

/**
 * Print test summary
 */
void physics_stability_test_print_summary(void);

/**
 * Export test results to file
 * @param filename Output filename
 * @return True if export successful
 */
bool physics_stability_test_export_results(const char *filename);

// ========================================
// Physics Object Management
// ========================================

/**
 * Create physics object
 * @param position Initial position
 * @param velocity Initial velocity
 * @param mass Object mass
 * @param radius Object radius
 * @param is_static Whether object is static
 * @return Created object or NULL on failure
 */
PhysicsObject* physics_create_object(const float position[3],
                                   const float velocity[3],
                                   float mass, float radius,
                                   bool is_static);

/**
 * Destroy physics object
 * @param object Object to destroy
 */
void physics_destroy_object(PhysicsObject *object);

/**
 * Update physics object
 * @param object Object to update
 * @param dt Time step
 * @param gravity Gravity vector
 */
void physics_update_object(PhysicsObject *object, float dt, const float gravity[3]);

/**
 * Apply force to physics object
 * @param object Object
 * @param force Force vector
 */
void physics_apply_force(PhysicsObject *object, const float force[3]);

/**
 * Apply impulse to physics object
 * @param object Object
 * @param impulse Impulse vector
 */
void physics_apply_impulse(PhysicsObject *object, const float impulse[3]);

// ========================================
// Collision Detection Functions
// ========================================

/**
 * Check collision between two objects
 * @param obj1 First object
 * @param obj2 Second object
 * @param contact Output contact information
 * @return True if objects are colliding
 */
bool physics_check_collision(const PhysicsObject *obj1,
                          const PhysicsObject *obj2,
                          CollisionContact *contact);

/**
 * Resolve collision between two objects
 * @param obj1 First object
 * @param obj2 Second object
 * @param contact Contact information
 */
void physics_resolve_collision(PhysicsObject *obj1,
                            PhysicsObject *obj2,
                            const CollisionContact *contact);

/**
 * Find all collisions in scene
 * @param objects Object array
 * @param object_count Number of objects
 * @param contacts Output contact array
 * @param max_contacts Maximum contacts
 * @return Number of contacts found
 */
uint32_t physics_find_all_collisions(const PhysicsObject *objects,
                                    uint32_t object_count,
                                    CollisionContact *contacts,
                                    uint32_t max_contacts);

// ========================================
// Stability Test Functions
// ========================================

/**
 * Test physics stability at 180 Hz
 * @return True if test passes
 */
bool test_physics_stability_180hz(void);

/**
 * Test physics with varying time steps
 * @return True if test passes
 */
bool test_physics_variable_timestep(void);

/**
 * Test physics energy conservation
 * @return True if test passes
 */
bool test_physics_energy_conservation(void);

/**
 * Test physics numerical stability
 * @return True if test passes
 */
bool test_physics_numerical_stability(void);

/**
 * Test physics collision stability
 * @return True if test passes
 */
bool test_physics_collision_stability(void);

/**
 * Test physics constraint stability
 * @return True if test passes
 */
bool test_physics_constraint_stability(void);

// ========================================
// Validation Functions
// ========================================

/**
 * Validate physics object state
 * @param object Object to validate
 * @return True if object state is valid
 */
bool validate_physics_object(const PhysicsObject *object);

/**
 * Validate collision response
 * @param obj1 First object
 * @param obj2 Second object
 * @param contact Contact information
 * @return True if collision response is valid
 */
bool validate_collision_response(const PhysicsObject *obj1,
                              const PhysicsObject *obj2,
                              const CollisionContact *contact);

/**
 * Validate energy conservation
 * @param objects Object array
 * @param object_count Number of objects
 * @param tolerance Energy tolerance
 * @return True if energy is conserved
 */
bool validate_energy_conservation(const PhysicsObject *objects,
                                uint32_t object_count,
                                float tolerance);

/**
 * Validate position stability
 * @param object Object to validate
 * @param tolerance Position tolerance
 * @return True if position is stable
 */
bool validate_position_stability(const PhysicsObject *object, float tolerance);

// ========================================
// Analysis Functions
// ========================================

/**
 * Calculate object kinetic energy
 * @param object Physics object
 * @return Kinetic energy
 */
float calculate_kinetic_energy(const PhysicsObject *object);

/**
 * Calculate object potential energy
 * @param object Physics object
 * @param gravity Gravity strength
 * @return Potential energy
 */
float calculate_potential_energy(const PhysicsObject *object, float gravity);

/**
 * Calculate object total energy
 * @param object Physics object
 * @param gravity Gravity strength
 * @return Total energy
 */
float calculate_total_energy(const PhysicsObject *object, float gravity);

/**
 * Calculate stability metrics
 * @param objects Object array
 * @param object_count Number of objects
 * @param simulation_time Total simulation time
 * @return Stability metrics
 */
StabilityMetrics calculate_stability_metrics(const PhysicsObject *objects,
                                          uint32_t object_count,
                                          float simulation_time);

/**
 * Detect instability events
 * @param objects Object array
 * @param object_count Number of objects
 * @param events Output instability events
 * @param max_events Maximum events
 * @return Number of events detected
 */
uint32_t detect_instability_events(const PhysicsObject *objects,
                                   uint32_t object_count,
                                   char *events,
                                   uint32_t max_events);

// ========================================
// Utility Functions
// ========================================

/**
 * Generate random position
 * @param position Output position
 * @param bounds Position bounds
 */
void generate_random_position(float position[3], const float bounds[3]);

/**
 * Generate random velocity
 * @param velocity Output velocity
 * @param max_speed Maximum speed
 */
void generate_random_velocity(float velocity[3], float max_speed);

/**
 * Calculate distance between two positions
 * @param pos1 First position
 * @param pos2 Second position
 * @return Distance
 */
float calculate_distance(const float pos1[3], const float pos2[3]);

/**
 * Calculate velocity magnitude
 * @param velocity Velocity vector
 * @return Magnitude
 */
float calculate_velocity_magnitude(const float velocity[3]);

/**
 * Check for NaN or infinite values
 * @param value Value to check
 * @return True if value is invalid
 */
bool is_invalid_float(float value);

/**
 * Check for invalid vector
 * @param vector Vector to check
 * @return True if vector is invalid
 */
bool is_invalid_vector(const float vector[3]);

/**
 * Clamp value to range
 * @param value Value to clamp
 * @param min Minimum value
 * @param max Maximum value
 * @return Clamped value
 */
float clamp_float(float value, float min, float max);

/**
 * Interpolate between two values
 * @param a First value
 * @param b Second value
 * @param t Interpolation parameter
 * @return Interpolated value
 */
float lerp_float(float a, float b, float t);

/**
 * Generate stability report
 * @param metrics Stability metrics
 * @param report Output report string
 * @param max_size Maximum report size
 * @return True if report generated
 */
bool generate_stability_report(const StabilityMetrics *metrics,
                             char *report,
                             size_t max_size);

#ifdef __cplusplus
}
#endif

#endif /* PHYSICS_STABILITY_TEST_H */

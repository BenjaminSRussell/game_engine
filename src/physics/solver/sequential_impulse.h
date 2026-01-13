/**
 * @file sequential_impulse.h
 * @brief Sequential impulse constraint solver
 *
 * Provides interface for the sequential impulse method of solving contact constraints
 * with support for warm starting, position correction, and friction.
 */

#ifndef SEQUENTIAL_IMPULSE_H
#define SEQUENTIAL_IMPULSE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct SequentialImpulseSolver SequentialImpulseSolver;
typedef struct RigidBody RigidBody;
typedef struct Contact Contact;

// ========================================
// Rigid Body Structure (Simplified Interface)
// ========================================

struct RigidBody {
    float position[3];            // Center of mass position
    float orientation[4];          // Quaternion orientation
    float linear_velocity[3];      // Linear velocity
    float angular_velocity[3];     // Angular velocity
    float mass;                   // Mass
    float inertia;                // Moment of inertia (simplified)
    float restitution;            // Coefficient of restitution
    float static_friction;        // Static friction coefficient
    float kinetic_friction;       // Kinetic friction coefficient
    bool is_static;               // Static body flag
    void *user_data;              // User data pointer
    
};

// ========================================
// Contact Structure
// ========================================

struct Contact {
    float position[3];            // Contact position in world space
    float normal[3];              // Contact normal (pointing from A to B)
    float penetration;            // Penetration depth
    int persistent_id;            // Persistent contact ID for warm starting
    float previous_normal_impulse; // Previous normal impulse (for warm starting)
    float previous_tangent_impulse[2]; // Previous tangent impulses
    bool valid;                   // Contact is valid
    
};

// ========================================
// Solver Configuration
// ========================================

typedef struct SequentialImpulseConfig {
    int max_iterations;           // Maximum solver iterations
    bool position_correction;     // Enable position correction
    bool warm_starting;           // Enable warm starting
    bool friction_enabled;        // Enable friction solving
    bool restitution_enabled;     // Enable restitution
    float min_velocity_threshold; // Minimum velocity threshold
    float min_penetration_threshold; // Minimum penetration threshold
    
} SequentialImpulseConfig;

// ========================================
// Constants
// ========================================

#define MAX_CONTACTS 64
#define MAX_BODIES 2

// ========================================
// Creation/Destruction
// ========================================

/**
 * Create sequential impulse solver
 * @param config Solver configuration (NULL for default)
 * @return Solver instance or NULL on failure
 */
SequentialImpulseSolver* sequential_impulse_create(const SequentialImpulseConfig *config);

/**
 * Destroy sequential impulse solver
 * @param solver Solver instance
 */
void sequential_impulse_destroy(SequentialImpulseSolver *solver);

// ========================================
// Contact Management
// ========================================

/**
 * Reset solver state
 * @param solver Solver instance
 */
void sequential_impulse_reset(SequentialImpulseSolver *solver);

/**
 * Add contact to solver
 * @param solver Solver instance
 * @param contact Contact data
 * @return Contact ID or -1 on failure
 */
int sequential_impulse_add_contact(SequentialImpulseSolver *solver, const Contact *contact);

/**
 * Remove contact from solver
 * @param solver Solver instance
 * @param contact_id Contact ID
 */
void sequential_impulse_remove_contact(SequentialImpulseSolver *solver, int contact_id);

/**
 * Update contact data
 * @param solver Solver instance
 * @param contact_id Contact ID
 * @param contact New contact data
 */
void sequential_impulse_update_contact(SequentialImpulseSolver *solver, int contact_id, 
                                      const Contact *contact);

// ========================================
// Solving Interface
// ========================================

/**
 * Solve contacts between two bodies
 * @param solver Solver instance
 * @param body_a First rigid body
 * @param body_b Second rigid body
 * @param dt Time step
 */
void sequential_impulse_solve(SequentialImpulseSolver *solver, RigidBody *body_a, RigidBody *body_b, float dt);

/**
 * Solve contacts for multiple body pairs
 * @param solver Solver instance
 * @param bodies Array of rigid bodies
 * @param body_count Number of bodies
 * @param body_pairs Array of body index pairs
 * @param pair_count Number of body pairs
 * @param dt Time step
 */
void sequential_impulse_solve_multiple(SequentialImpulseSolver *solver, RigidBody **bodies, int body_count,
                                       const int (*body_pairs)[2], int pair_count, float dt);

// ========================================
// Configuration Management
// ========================================

/**
 * Set solver configuration
 * @param solver Solver instance
 * @param config New configuration
 */
void sequential_impulse_set_config(SequentialImpulseSolver *solver, const SequentialImpulseConfig *config);

/**
 * Get solver configuration
 * @param solver Solver instance
 * @return Current configuration
 */
SequentialImpulseConfig sequential_impulse_get_config(SequentialImpulseSolver *solver);

/**
 * Get default configuration
 * @return Default configuration
 */
SequentialImpulseConfig sequential_impulse_get_default_config(void);

// ========================================
// Statistics and Debugging
// ========================================

/**
 * Get solver statistics
 * @param solver Solver instance
 * @param contact_count Number of contacts solved
 * @param iteration_count Number of iterations performed
 * @param total_impulse Total accumulated impulse
 */
void sequential_impulse_get_stats(const SequentialImpulseSolver *solver, 
                                 int *contact_count, int *iteration_count, 
                                 float *total_impulse);

/**
 * Get contact information
 * @param solver Solver instance
 * @param contact_id Contact ID
 * @param contact Output contact data
 */
void sequential_impulse_get_contact(const SequentialImpulseSolver *solver, int contact_id, Contact *contact);

/**
 * Validate solver state
 * @param solver Solver instance
 * @return True if state is valid
 */
bool sequential_impulse_validate(const SequentialImpulseSolver *solver);

// ========================================
// Advanced Features
// ========================================

/**
 * Enable/disable warm starting
 * @param solver Solver instance
 * @param enabled Enable warm starting
 */
void sequential_impulse_enable_warm_starting(SequentialImpulseSolver *solver, bool enabled);

/**
 * Enable/disable position correction
 * @param solver Solver instance
 * @param enabled Enable position correction
 */
void sequential_impulse_enable_position_correction(SequentialImpulseSolver *solver, bool enabled);

/**
 * Set iteration count
 * @param solver Solver instance
 * @param iterations Maximum iterations
 */
void sequential_impulse_set_iterations(SequentialImpulseSolver *solver, int iterations);

/**
 * Set velocity and penetration thresholds
 * @param solver Solver instance
 * @param velocity_threshold Minimum velocity threshold
 * @param penetration_threshold Minimum penetration threshold
 */
void sequential_impulse_set_thresholds(SequentialImpulseSolver *solver, 
                                     float velocity_threshold, float penetration_threshold);

/**
 * Get solver performance metrics
 * @param solver Solver instance
 * @param solve_time_ms Time spent solving (milliseconds)
 * @param contacts_per_second Contacts solved per second
 */
void sequential_impulse_get_performance_metrics(const SequentialImpulseSolver *solver, 
                                              float *solve_time_ms, float *contacts_per_second);

#ifdef __cplusplus
}
#endif

#endif /* SEQUENTIAL_IMPULSE_H */

/**
 * Structure of Arrays (SoA) Container for Rigid Bodies
 * 
 * Memory layout optimized for SIMD processing. Each property is stored in
 * a separate, cache-aligned array enabling vectorized operations.
 * 
 * Traditional AoS layout (cache-inefficient):
 *   [pos vel mass | pos vel mass | pos vel mass ...]
 * 
 * SoA layout (cache-friendly):
 *   positions:  [pos | pos | pos | pos | pos | pos | pos | pos ...]
 *   velocities: [vel | vel | vel | vel | vel | vel | vel | vel ...]
 *   masses:     [m   | m   | m   | m   | m   | m   | m   | m   ...]
 */

#ifndef SOA_RIGIDBODY_H
#define SOA_RIGIDBODY_H

#include "core/simd/simd_types.h"
#include "core/simd/simd_math.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// SoA Rigid Body Storage
// ========================================

/**
 * RigidBodySoA: Structure of Arrays for physics simulation
 * 
 * All dynamic properties stored in separate, aligned arrays for SIMD access.
 */
typedef struct RigidBodySoA {
    // Capacity and count
    uint32_t capacity;          // Total allocated slots
    uint32_t count;             // Active bodies
    
    // Transform (position + orientation)
    v4f *positions;             // World space positions (x, y, z, 0)
    v4f *rotations;             // Quaternions (x, y, z, w)
    
    // Linear dynamics
    v4f *velocities;            // Linear velocities (m/s)
    v4f *forces;                // Accumulated forces (N)
    float *inv_masses;          // Inverse masses (1/kg)
    
    // Angular dynamics
    v4f *angular_velocities;    // Angular velocities (rad/s)
    v4f *torques;               // Accumulated torques (Nm)
    
    // Inertia tensors (3x3 matrices stored as 3 v4f)
    v4f *inv_inertia_0;         // Inverse inertia column 0
    v4f *inv_inertia_1;         // Inverse inertia column 1
    v4f *inv_inertia_2;         // Inverse inertia column 2
    
    // Material properties
    float *friction;            // Friction coefficients
    float *restitution;         // Bounciness (0-1)
    float *linear_damping;      // Linear velocity damping
    float *angular_damping;     // Angular velocity damping
    
    // State flags
    uint8_t *flags;             // Packed flags (sleeping, active, etc.)
    
    // User data
    uint32_t *entity_ids;       // Associated ECS entity IDs
    void **user_data;           // Arbitrary user pointers
    
} RigidBodySoA;

// State flags (bit flags in `flags` array)
#define RIGIDBODY_FLAG_ACTIVE    (1 << 0)
#define RIGIDBODY_FLAG_SLEEPING  (1 << 1)
#define RIGIDBODY_FLAG_STATIC    (1 << 2)
#define RIGIDBODY_FLAG_KINEMATIC (1 << 3)

// ========================================
// SoA Container Management
// ========================================

/**
 * Create a new SoA container with specified capacity
 * 
 * @param capacity Maximum number of rigid bodies
 * @return Allocated SoA container, or NULL on failure
 */
RigidBodySoA* soa_rigidbody_create(uint32_t capacity);

/**
 * Destroy SoA container and free all memory
 */
void soa_rigidbody_destroy(RigidBodySoA *soa);

/**
 * Add a new rigid body to the container
 * 
 * @param soa The SoA container
 * @return Index of the new body, or UINT32_MAX if full
 */
uint32_t soa_rigidbody_add(RigidBodySoA *soa);

/**
 * Remove a rigid body by index (swap with last, then pop)
 * 
 * @param soa The SoA container
 * @param index Index to remove
 */
void soa_rigidbody_remove(RigidBodySoA *soa, uint32_t index);

/**
 * Clear all bodies (reset count to 0)
 */
void soa_rigidbody_clear(RigidBodySoA *soa);

// ========================================
// Batch Accessors (for vectorized operations)
// ========================================

/**
 * Get pointers to arrays for batch processing
 * Use these for SIMD operations over entire arrays
 */
static inline v4f* soa_get_positions(RigidBodySoA *soa) { return soa->positions; }
static inline v4f* soa_get_velocities(RigidBodySoA *soa) { return soa->velocities; }
static inline v4f* soa_get_forces(RigidBodySoA *soa) { return soa->forces; }
static inline float* soa_get_inv_masses(RigidBodySoA *soa) { return soa->inv_masses; }

// ========================================
// Individual Element Access (for convenience)
// ========================================

/**
 * Set position for a single body
 */
static inline void soa_set_position(RigidBodySoA *soa, uint32_t index, const v4f *pos) {
    v4f_copy(&soa->positions[index], pos);
}

/**
 * Get position for a single body
 */
static inline void soa_get_position(const RigidBodySoA *soa, uint32_t index, v4f *out_pos) {
    v4f_copy(out_pos, &soa->positions[index]);
}

/**
 * Set velocity for a single body
 */
static inline void soa_set_velocity(RigidBodySoA *soa, uint32_t index, const v4f *vel) {
    v4f_copy(&soa->velocities[index], vel);
}

/**
 * Set inverse mass (use 0.0 for static/infinite mass)
 */
static inline void soa_set_inv_mass(RigidBodySoA *soa, uint32_t index, float inv_mass) {
    soa->inv_masses[index] = inv_mass;
}

/**
 * Set mass (automatically computes inverse)
 */
static inline void soa_set_mass(RigidBodySoA *soa, uint32_t index, float mass) {
    soa->inv_masses[index] = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
}

/**
 * Check if body is active
 */
static inline bool soa_is_active(const RigidBodySoA *soa, uint32_t index) {
    return (soa->flags[index] & RIGIDBODY_FLAG_ACTIVE) != 0;
}

/**
 * Set active state
 */
static inline void soa_set_active(RigidBodySoA *soa, uint32_t index, bool active) {
    if (active) {
        soa->flags[index] |= RIGIDBODY_FLAG_ACTIVE;
    } else {
        soa->flags[index] &= ~RIGIDBODY_FLAG_ACTIVE;
    }
}

#ifdef __cplusplus
}
#endif

#endif // SOA_RIGIDBODY_H

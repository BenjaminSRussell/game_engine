/**
 * Vectorized Physics Integration
 * 
 * SIMD-optimized physics update functions operating on SoA container.
 * Demonstrates AAA-grade performance through data-oriented design.
 */

#ifndef PHYSICS_INTEGRATION_H
#define PHYSICS_INTEGRATION_H

#include "core/containers/soa_rigidbody.h"
#include "core/simd/simd_math.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Semi-implicit Euler integration (Symplectic Euler)
 * 
 * Updates velocities and positions for all active bodies using SIMD batch operations.
 * Formula:
 *   v' = v + (F / m) * dt
 *   p' = p + v' * dt
 * 
 * @param soa RigidBody container
 * @param dt Timestep (seconds)
 */
void physics_integrate_velocities(RigidBodySoA *soa, float dt);

/**
 * Apply linear damping to velocities
 * 
 * v' = v * (1 - damping * dt)
 * 
 * @param soa RigidBody container
 * @param dt Timestep (seconds)
 */
void physics_apply_damping(RigidBodySoA *soa, float dt);

/**
 * Clear accumulated forces (call after integration)
 */
void physics_clear_forces(RigidBodySoA *soa);

/**
 * Add gravity to all dynamic bodies
 * 
 * F += m * g
 * 
 * @param soa RigidBody container
 * @param gravity Gravity vector (e.g., (0, -9.81, 0))
 */
void physics_apply_gravity(RigidBodySoA *soa, const v4f *gravity);

#ifdef __cplusplus
}
#endif

#endif // PHYSICS_INTEGRATION_H

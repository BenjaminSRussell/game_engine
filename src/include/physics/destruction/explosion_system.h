/**
 * =================================================================================================
 *                          EXPLOSION & SHOCKWAVE SYSTEM
 * =================================================================================================
 * 
 * Physics-based explosion system with shockwave propagation, damage falloff,
 * and impulse application to rigid bodies.
 */

#ifndef EXPLOSION_SYSTEM_H
#define EXPLOSION_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct PhysicsWorld PhysicsWorld;
typedef struct Vec3 Vec3;

// ============================================================================
// Types & Structures
// ============================================================================

typedef struct {
    float position[3];
    float radius;
    float force;
    float damage;
    float duration;
    float elapsed_time;
    bool active;
    
    // Falloff curve
    float falloff_exponent;  // 1.0 = linear, 2.0 = quadratic
    
    // Effects
    bool spawn_particles;
    bool apply_screen_shake;
    float shake_intensity;
} Explosion;

typedef struct {
    Explosion *explosions;
    uint32_t capacity;
    uint32_t count;
    
    PhysicsWorld *physics_world;
} ExplosionSystem;

// ============================================================================
// System Management
// ============================================================================

ExplosionSystem *explosion_system_create(PhysicsWorld *world, uint32_t max_explosions);
void explosion_system_destroy(ExplosionSystem *system);
void explosion_system_update(ExplosionSystem *system, float dt);

// ============================================================================
// Explosion Spawning
// ============================================================================

/**
 * Spawn an explosion at the given position
 * 
 * @param system Explosion system
 * @param position World position
 * @param radius Effect radius (meters)
 * @param force Maximum impulse force (Newtons)
 * @param damage Maximum damage value
 * @return Explosion index, or -1 if failed
 */
int explosion_spawn(ExplosionSystem *system,
                    const float position[3],
                    float radius,
                    float force,
                    float damage);

/**
 * Spawn explosion with advanced parameters
 */
int explosion_spawn_ex(ExplosionSystem *system,
                      const float position[3],
                      float radius,
                      float force,
                      float damage,
                      float falloff_exponent,
                      float shake_intensity,
                      bool spawn_particles);

// ============================================================================
// Queries
// ============================================================================

/**
 * Calculate damage at a point from an explosion
 */
float explosion_calculate_damage(const Explosion *explosion, const float point[3]);

/**
 * Calculate impulse force vector at a point
 */
void explosion_calculate_impulse(const Explosion *explosion,
                                const float point[3],
                                float *out_impulse);

/**
 * Check if explosion affects a point
 */
bool explosion_affects_point(const Explosion *explosion, const float point[3]);

// ============================================================================
// Debug / Visualization
// ============================================================================

/**
 * Get blast radius for visualization
 */
void explosion_get_blast_radius_sphere(const Explosion *explosion,
                                      float *out_center,
                                      float *out_radius);

#endif // EXPLOSION_SYSTEM_H

/**
 * Contact Manifold Generation
 * 
 * Extracts contact points from collision detection results.
 * Uses reference/incident face determination and clipping.
 */

#ifndef CONTACT_MANIFOLD_H
#define CONTACT_MANIFOLD_H

#include "collision_gjk_epa.h"
#include "../../core/simd/simd_math.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MANIFOLD_POINTS 4

// ========================================
// Contact Point
// ========================================

typedef struct ContactPoint {
    v4f position_a;      // Contact point on body A in world space
    v4f position_b;      // Contact point on body B in world space
    v4f local_a;         // Contact point on body A in local space
    v4f local_b;         // Contact point on body B in local space
    float penetration;   // Penetration depth
    v4f normal;          // Contact normal (from B to A)
    float friction;      // Combined friction coefficient
    float restitution;   // Combined restitution coefficient
} ContactPoint;

// ========================================
// Contact Manifold
// ========================================

typedef struct ContactManifold {
    ContactPoint points[MAX_MANIFOLD_POINTS];
    int num_points;
    v4f normal;          // Manifold normal (average or reference)
} ContactManifold;

/**
 * Generate contact manifold from EPA result
 * 
 * @param manifold Output manifold
 * @param shape_a Shape A
 * @param tx_a Transform of A
 * @param shape_b Shape B
 * @param tx_b Transform of B
 * @param epa EPA result containing penetration info
 * @return true if manifold generated successfully
 */
bool generate_contact_manifold(ContactManifold *manifold,
                               const CollisionShape *shape_a, const Transform *tx_a,
                               const CollisionShape *shape_b, const Transform *tx_b,
                               const EPAResult *epa);

/**
 * Get support face for a shape in a given direction
 * Returns indices of vertices forming the face most aligned with the direction.
 */
void get_support_face(const CollisionShape *shape, const Transform *tx,
                     const v4f *direction, v4f *face_vertices, int *num_vertices);

/**
 * Clip incident face against reference face side planes
 * Uses Sutherland-Hodgman clipping algorithm
 */
int clip_face_against_plane(const v4f *input_vertices, int num_input,
                            v4f *output_vertices,
                            const v4f *plane_normal, float plane_distance);

#ifdef __cplusplus
}
#endif

#endif // CONTACT_MANIFOLD_H

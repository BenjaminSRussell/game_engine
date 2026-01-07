/**
 * GJK/EPA Narrowphase Collision Detection
 * 
 * Implements Gilbert-Johnson-Keerthi (GJK) for boolean collision and
 * Expanding Polytope Algorithm (EPA) for penetration depth/normal.
 */

#ifndef COLLISION_GJK_EPA_H
#define COLLISION_GJK_EPA_H

#include "core/simd/simd_math.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Shape Types
// ========================================

typedef enum ShapeType {
    SHAPE_SPHERE,
    SHAPE_BOX,
    SHAPE_CAPSULE,
    SHAPE_CONVEX_HULL
} ShapeType;

// Base shape config (opaque pointer for specific data)
typedef struct CollisionShape {
    ShapeType type;
    void *data;       // Shape-specific data (e.g., radius, dimensions)
    v4f center;       // Local center offset
} CollisionShape;

// Shape Definitions
typedef struct SphereShape {
    float radius;
} SphereShape;

typedef struct BoxShape {
    v4f half_extents; // Half-width, half-height, half-depth
} BoxShape;

typedef struct CapsuleShape {
    float radius;
    float height;     // Cylinder height (total height = height + 2*radius)
} CapsuleShape;

// ========================================
// Transform
// ========================================

typedef struct Transform {
    v4f position;
    v4f rotation; // Quaternion
} Transform;

// ========================================
// Support Function
// ========================================

/**
 * Support function callback type.
 * Returns the point on the shape furthest in direction 'dir'.
 */
typedef v4f (*SupportFunc)(const void *shape, const v4f *dir);

/**
 * Get support point for a generic shape in world space.
 */
v4f get_support(const CollisionShape *shape, const Transform *tx, const v4f *dir);

// ========================================
// GJK Output
// ========================================

typedef struct GJKResult {
    bool colliding;
    v4f simplex[4];   // Final simplex points
    int simplex_dim;  // 1=point, 2=line, 3=triangle, 4=tetrahedron
    v4f closest_uv;   // Closest point on shape A/B in Minkowski difference (if not colliding)
} GJKResult;

/**
 * Run GJK algorithm to detect collision.
 * 
 * @param shape_a Shape A
 * @param tx_a Transform of Shape A
 * @param shape_b Shape B
 * @param tx_b Transform of Shape B
 * @return GJKResult
 */
GJKResult gjk_detect_collision(const CollisionShape *shape_a, const Transform *tx_a,
                              const CollisionShape *shape_b, const Transform *tx_b);

// ========================================
// EPA Output
// ========================================

typedef struct EPAResult {
    bool valid;
    float penetration_depth;
    v4f normal;             // Normal on B pointing towards A
    v4f contact_point_a;    // Contact point on A in world space
    v4f contact_point_b;    // Contact point on B in world space
} EPAResult;

/**
 * Run EPA algorithm to find penetration depth and contact info.
 * Should only be called if GJK returned colliding = true.
 * 
 * @param shape_a Shape A
 * @param tx_a Transform of Shape A
 * @param shape_b Shape B
 * @param tx_b Transform of Shape B
 * @param gjk_simplex Simplex from GJK
 * @param gjk_dim Dimension of simplex from GJK
 * @return EPAResult
 */
EPAResult epa_compute_penetration(const CollisionShape *shape_a, const Transform *tx_a,
                                 const CollisionShape *shape_b, const Transform *tx_b,
                                 const v4f *gjk_simplex, int gjk_dim);

#ifdef __cplusplus
}
#endif

#endif // COLLISION_GJK_EPA_H

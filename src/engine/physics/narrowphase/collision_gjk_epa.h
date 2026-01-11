/**
 * GJK/EPA Narrowphase Collision Detection
 * 
 * Implements Gilbert-Johnson-Keerthi (GJK) for boolean collision and
 * Expanding Polytope Algorithm (EPA) for penetration depth/normal.
 * Includes continuous collision detection (CCD) support for fast-moving objects.
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
    bool ccd_enabled;  // Enable CCD for this shape
    float ccd_margin;  // CCD margin for continuous collision detection
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
// Velocity Structure for CCD
// ========================================

typedef struct Velocity {
    v4f linear;      // Linear velocity
    v4f angular;     // Angular velocity
} Velocity;

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
    float distance;   // Minimum distance between shapes
    int iterations;   // Number of iterations performed
    bool converged;   // Algorithm converged
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

/**
 * Run GJK algorithm with continuous collision detection.
 * 
 * @param shape_a Shape A
 * @param tx_a Transform of Shape A
 * @param vel_a Velocity of Shape A
 * @param shape_b Shape B
 * @param tx_b Transform of Shape B
 * @param vel_b Velocity of Shape B
 * @param dt Time step
 * @param time_of_impact Output time of impact (0-1)
 * @return GJKResult with CCD information
 */
GJKResult gjk_detect_collision_ccd(const CollisionShape *shape_a, const Transform *tx_a, const Velocity *vel_a,
                                   const CollisionShape *shape_b, const Transform *tx_b, const Velocity *vel_b,
                                   float dt, float *time_of_impact);

// ========================================
// EPA Output
// ========================================

typedef struct EPAResult {
    bool valid;
    float penetration_depth;
    v4f normal;             // Normal on B pointing towards A
    v4f contact_point_a;    // Contact point on A in world space
    v4f contact_point_b;    // Contact point on B in world space
    int iterations;         // Number of iterations performed
    bool converged;         // Algorithm converged
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

// ========================================
// CCD Configuration
// ========================================

typedef struct CCDConfig {
    bool enabled;              // Enable CCD globally
    float max_time_step;        // Maximum CCD time step
    float max_distance;         // Maximum CCD distance
    float safety_margin;         // Safety margin for CCD
    int max_iterations;         // Maximum CCD iterations
} CCDConfig;

/**
 * Set CCD configuration.
 * @param config CCD configuration
 */
void ccd_set_config(const CCDConfig *config);

/**
 * Get current CCD configuration.
 * @return Current CCD configuration
 */
CCDConfig ccd_get_config(void);

/**
 * Enable/disable CCD globally.
 * @param enabled Enable CCD
 */
void ccd_set_enabled(bool enabled);

/**
 * Set CCD maximum time step.
 * @param max_time_step Maximum time step
 */
void ccd_set_max_time_step(float max_time_step);

/**
 * Set CCD maximum distance.
 * @param max_distance Maximum distance
 */
void ccd_set_max_distance(float max_distance);

// ========================================
// Shape Creation with CCD Support
// ========================================

/**
 * Create sphere shape with CCD support.
 * @param radius Sphere radius
 * @param ccd_margin CCD margin
 * @return Sphere shape
 */
CollisionShape* shape_create_sphere_ccd(float radius, float ccd_margin);

/**
 * Create box shape with CCD support.
 * @param half_extents Box half extents
 * @param ccd_margin CCD margin
 * @return Box shape
 */
CollisionShape* shape_create_box_ccd(const v4f *half_extents, float ccd_margin);

/**
 * Create capsule shape with CCD support.
 * @param radius Capsule radius
 * @param height Capsule height
 * @param ccd_margin CCD margin
 * @return Capsule shape
 */
CollisionShape* shape_create_capsule_ccd(float radius, float height, float ccd_margin);

/**
 * Enable/disable CCD for a shape.
 * @param shape Shape to modify
 * @param enabled Enable CCD
 */
void shape_set_ccd_enabled(CollisionShape *shape, bool enabled);

/**
 * Set CCD margin for a shape.
 * @param shape Shape to modify
 * @param margin CCD margin
 */
void shape_set_ccd_margin(CollisionShape *shape, float margin);

/**
 * Get CCD margin for a shape.
 * @param shape Shape to query
 * @return CCD margin
 */
float shape_get_ccd_margin(const CollisionShape *shape);

/**
 * Check if CCD is enabled for a shape.
 * @param shape Shape to query
 * @return True if CCD is enabled
 */
bool shape_is_ccd_enabled(const CollisionShape *shape);

// ========================================
// Broadphase CCD Support
// ========================================

/**
 * Perform broadphase sweep for CCD.
 * @param shapes Array of shapes
 * @param transforms Array of transforms
 * @param velocities Array of velocities
 * @param count Number of shapes
 * @param dt Time step
 * @param potential_pairs Output array of potential collision pairs
 * @param max_pairs Maximum number of pairs to output
 * @return Number of potential pairs found
 */
int ccd_broadphase_sweep(const CollisionShape **shapes, const Transform **transforms, const Velocity **velocities,
                        int count, float dt, int (*potential_pairs)[2], int max_pairs);

/**
 * Perform conservative advancement for CCD.
 * @param shape_a Shape A
 * @param tx_a Transform of Shape A
 * @param vel_a Velocity of Shape A
 * @param shape_b Shape B
 * @param tx_b Transform of Shape B
 * @param vel_b Velocity of Shape B
 * @param dt Time step
 * @param time_of_impact Output time of impact
 * @return True if collision detected
 */
bool ccd_conservative_advancement(const CollisionShape *shape_a, const Transform *tx_a, const Velocity *vel_a,
                                  const CollisionShape *shape_b, const Transform *tx_b, const Velocity *vel_b,
                                  float dt, float *time_of_impact);

// ========================================
// Utility Functions
// ========================================

/**
 * Calculate distance between two shapes.
 * @param shape_a Shape A
 * @param tx_a Transform of Shape A
 * @param shape_b Shape B
 * @param tx_b Transform of Shape B
 * @return Minimum distance
 */
float shape_distance(const CollisionShape *shape_a, const Transform *tx_a,
                    const CollisionShape *shape_b, const Transform *tx_b);

/**
 * Check if two shapes intersect.
 * @param shape_a Shape A
 * @param tx_a Transform of Shape A
 * @param shape_b Shape B
 * @param tx_b Transform of Shape B
 * @return True if shapes intersect
 */
bool shape_intersects(const CollisionShape *shape_a, const Transform *tx_a,
                     const CollisionShape *shape_b, const Transform *tx_b);

/**
 * Ray cast against shape with CCD support.
 * @param shape Shape to cast against
 * @param tx Shape transform
 * @param ray_origin Ray origin
 * @param ray_direction Ray direction (must be normalized)
 * @param max_distance Maximum ray distance
 * @param hit_point Output hit point
 * @param hit_normal Output hit normal
 * @param hit_time Output hit time (for moving shapes)
 * @return True if ray hits shape
 */
bool shape_ray_cast_ccd(const CollisionShape *shape, const Transform *tx,
                        const v4f *ray_origin, const v4f *ray_direction,
                        float max_distance, v4f *hit_point, v4f *hit_normal, float *hit_time);

/**
 * Get shape bounding box with CCD margin.
 * @param shape Shape to query
 * @param tx Shape transform
 * @param min Output minimum corner
 * @param max Output maximum corner
 */
void shape_get_bounds_ccd(const CollisionShape *shape, const Transform *tx, v4f *min, v4f *max);

/**
 * Validate shape integrity.
 * @param shape Shape to validate
 * @return True if shape is valid
 */
bool shape_validate(const CollisionShape *shape);

/**
 * Get shape statistics.
 * @param shape Shape to query
 * @param vertex_count Output vertex count
 * @param triangle_count Output triangle count
 * @param memory_usage Output memory usage in bytes
 */
void shape_get_stats(const CollisionShape *shape, int *vertex_count, int *triangle_count, size_t *memory_usage);

#ifdef __cplusplus
}
#endif

#endif // COLLISION_GJK_EPA_H

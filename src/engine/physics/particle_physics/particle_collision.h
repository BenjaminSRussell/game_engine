/**
 * @file particle_collision.h
 * @brief Particle-world collision detection and response
 *
 * Implements efficient collision detection for particle systems with various
 * geometry types including planes, spheres, boxes, and triangle meshes.
 * Supports spatial partitioning for performance optimization.
 */

#ifndef RENDER_PARTICLE_COLLISION_H
#define RENDER_PARTICLE_COLLISION_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ========================================
// Forward Declarations
// ========================================

typedef struct ParticleCollisionSystem ParticleCollisionSystem;
typedef struct Particle Particle;
typedef struct CollisionShape CollisionShape;

// ========================================
// Particle Structure
// ========================================

typedef struct Particle {
    // Position and velocity
    float position[3];
    float velocity[3];
    float acceleration[3];
    
    // Physical properties
    float mass;
    float inv_mass;
    float radius;
    float restitution;     // Bounciness (0-1)
    float friction;         // Friction coefficient
    
    // Collision state
    bool collision_enabled;
    int collision_mask;      // Collision layers/mask
    
    // User data
    void *user_data;
    
} Particle;

// ========================================
// Collision Shape Types
// ========================================

typedef enum ShapeType {
    SHAPE_PLANE,            // Infinite plane
    SHAPE_SPHERE,           // Sphere
    SHAPE_BOX,              // Axis-aligned box
    SHAPE_TRIANGLE_MESH     // Triangle mesh
} ShapeType;

// ========================================
// Plane Shape
// ========================================

typedef struct PlaneShape {
    float normal[3];        // Plane normal (must be normalized)
    float distance;         // Distance from origin along normal
} PlaneShape;

// ========================================
// Sphere Shape
// ========================================

typedef struct SphereShape {
    float center[3];        // Sphere center
    float radius;           // Sphere radius
} SphereShape;

// ========================================
// Box Shape
// ========================================

typedef struct BoxShape {
    float min[3];           // Minimum corner
    float max[3];           // Maximum corner
} BoxShape;

// ========================================
// Triangle Mesh Shape
// ========================================

typedef struct TriangleMeshShape {
    float *vertices;        // Vertex positions (3 floats per vertex)
    int *triangles;         // Triangle indices (3 ints per triangle)
    int vertex_count;
    int triangle_count;
    
    // Spatial acceleration
    void *bvh;              // Bounding volume hierarchy
    bool bvh_dirty;         // BVH needs rebuilding
    
} TriangleMeshShape;

// ========================================
// Unified Collision Shape
// ========================================

struct CollisionShape {
    ShapeType type;
    union {
        PlaneShape plane;
        SphereShape sphere;
        BoxShape box;
        TriangleMeshShape mesh;
    } data;
    
    // Material properties
    float restitution;
    float friction;
    
    // Collision filtering
    int collision_layer;
    int collision_mask;
    
    bool enabled;
};

// ========================================
// Collision Result
// ========================================

typedef struct CollisionResult {
    bool collided;
    float contact_point[3];     // Contact point in world space
    float contact_normal[3];     // Contact normal (pointing away from shape)
    float penetration_depth;     // How deep particle is penetrating
    float contact_velocity[3];  // Relative velocity at contact
    
    // Shape reference
    CollisionShape *shape;
    
} CollisionResult;

// ========================================
// Collision System Configuration
// ========================================

typedef struct CollisionConfig {
    // Performance settings
    bool use_spatial_hash;       // Use spatial hashing for broadphase
    int hash_grid_size;          // Spatial hash grid size
    float cell_size;             // Spatial hash cell size
    
    // Collision settings
    float max_collision_distance; // Maximum collision detection distance
    float min_penetration;       // Minimum penetration to resolve
    int max_contacts_per_particle; // Max contacts per particle
    
    // Solver settings
    int solver_iterations;       // Collision solver iterations
    float position_correction;   // Position correction factor
    float velocity_damping;      // Velocity damping on collision
    
} CollisionConfig;

// ========================================
// Spatial Hash for Broadphase
// ========================================

typedef struct SpatialHashGrid {
    int *cell_buckets;           // Bucket start indices
    int *particle_indices;       // Particle indices in buckets
    int bucket_count;
    int max_particles_per_bucket;
    float cell_size;
    
} SpatialHashGrid;

// ========================================
// System Creation/Destruction
// ========================================

/**
 * Create particle collision system
 */
ParticleCollisionSystem* particle_collision_create(int max_particles, int max_shapes, 
                                                   const CollisionConfig *config);

/**
 * Destroy collision system
 */
void particle_collision_destroy(ParticleCollisionSystem *system);

// ========================================
// Particle Management
// ========================================

/**
 * Add particle to collision system
 */
int particle_collision_add_particle(ParticleCollisionSystem *system, const Particle *particle);

/**
 * Remove particle from collision system
 */
void particle_collision_remove_particle(ParticleCollisionSystem *system, int particle_id);

/**
 * Update particle data
 */
void particle_collision_update_particle(ParticleCollisionSystem *system, int particle_id, 
                                       const Particle *particle);

/**
 * Get particle data
 */
Particle* particle_collision_get_particle(ParticleCollisionSystem *system, int particle_id);

// ========================================
// Collision Shape Management
// ========================================

/**
 * Create plane shape
 */
CollisionShape* particle_collision_create_plane(ParticleCollisionSystem *system, 
                                               const float *normal, float distance);

/**
 * Create sphere shape
 */
CollisionShape* particle_collision_create_sphere(ParticleCollisionSystem *system, 
                                                const float *center, float radius);

/**
 * Create box shape
 */
CollisionShape* particle_collision_create_box(ParticleCollisionSystem *system, 
                                             const float *min, const float *max);

/**
 * Create triangle mesh shape
 */
CollisionShape* particle_collision_create_mesh(ParticleCollisionSystem *system, 
                                              const float *vertices, int vertex_count,
                                              const int *triangles, int triangle_count);

/**
 * Remove collision shape
 */
void particle_collision_remove_shape(ParticleCollisionSystem *system, CollisionShape *shape);

/**
 * Update shape transform (for dynamic shapes)
 */
void particle_collision_update_shape(ParticleCollisionSystem *system, CollisionShape *shape);

// ========================================
// Collision Detection
// ========================================

/**
 * Detect collisions for all particles
 */
void particle_collision_detect_all(ParticleCollisionSystem *system);

/**
 * Detect collisions for specific particle
 */
int particle_collision_detect_particle(ParticleCollisionSystem *system, int particle_id, 
                                      CollisionResult *results, int max_results);

/**
 * Check if particle collides with any shape
 */
bool particle_collision_check_particle(ParticleCollisionSystem *system, int particle_id);

// ========================================
// Collision Response
// ========================================

/**
 * Resolve collisions for all particles
 */
void particle_collision_resolve_all(ParticleCollisionSystem *system, float dt);

/**
 * Resolve collisions for specific particle
 */
void particle_collision_resolve_particle(ParticleCollisionSystem *system, int particle_id, 
                                        const CollisionResult *results, int result_count, float dt);

/**
 * Apply collision response to particle
 */
void particle_collision_apply_response(Particle *particle, const CollisionResult *result, float dt);

// ========================================
// Utility Functions
// ========================================

/**
 * Get default collision configuration
 */
CollisionConfig particle_collision_get_default_config(void);

/**
 * Update collision system configuration
 */
void particle_collision_set_config(ParticleCollisionSystem *system, const CollisionConfig *config);

/**
 * Get system statistics
 */
void particle_collision_get_stats(const ParticleCollisionSystem *system, 
                                 int *particle_count, int *shape_count, 
                                 int *collision_count);

/**
 * Clear all particles
 */
void particle_collision_clear_particles(ParticleCollisionSystem *system);

/**
 * Clear all collision shapes
 */
void particle_collision_clear_shapes(ParticleCollisionSystem *system);

/**
 * Reset collision system
 */
void particle_collision_reset(ParticleCollisionSystem *system);

/**
 * Validate collision system state
 */
bool particle_collision_validate(const ParticleCollisionSystem *system);

// ========================================
// Ray Casting
// ========================================

typedef struct RayCastResult {
    bool hit;
    float distance;            // Distance along ray to hit point
    float hit_point[3];        // Hit point in world space
    float hit_normal[3];        // Normal at hit point
    CollisionShape *shape;      // Hit shape
    Particle *particle;         // Hit particle (if any)
    
} RayCastResult;

/**
 * Cast ray through collision system
 */
bool particle_collision_ray_cast(ParticleCollisionSystem *system, 
                                const float *ray_start, const float *ray_dir,
                                float max_distance, RayCastResult *result);

/**
 * Cast ray and find all hits
 */
int particle_collision_ray_cast_all(ParticleCollisionSystem *system, 
                                    const float *ray_start, const float *ray_dir,
                                    float max_distance, RayCastResult *results, int max_results);

// ========================================
// Broadphase Queries
// ========================================

/**
 * Find all particles in sphere
 */
int particle_collision_query_sphere(ParticleCollisionSystem *system, 
                                    const float *center, float radius,
                                    int *particle_ids, int max_ids);

/**
 * Find all particles in box
 */
int particle_collision_query_box(ParticleCollisionSystem *system, 
                                const float *min, const float *max,
                                int *particle_ids, int max_ids);

/**
 * Find all shapes in sphere
 */
int particle_collision_query_shapes_sphere(ParticleCollisionSystem *system, 
                                           const float *center, float radius,
                                           CollisionShape **shapes, int max_shapes);

#ifdef __cplusplus
}
#endif

#endif /* RENDER_PARTICLE_COLLISION_H */

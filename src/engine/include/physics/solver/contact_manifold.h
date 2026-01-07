#pragma once

#include "../../core/types.h"
#include "../../math/vec3.h"
#include "../../math/quat.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct ContactManifold ContactManifold;
typedef struct ContactPoint ContactPoint;
typedef struct RigidBody RigidBody;

// Maximum contact points per manifold
#define MAX_CONTACT_POINTS 4

// Contact point state
typedef enum {
    CONTACT_STATE_NEW,
    CONTACT_STATE_PERSISTENT,
    CONTACT_STATE_SEPARATED,
    CONTACT_STATE_RESTING
} ContactPointState;

// Single contact point
typedef struct {
    Vec3 position_a;           // Contact point on body A
    Vec3 position_b;           // Contact point on body B
    f32 depth;                 // Penetration depth
    Vec3 normal;               // Contact normal
    ContactPointState state;
    u32 persistence_frames;    // How long this point has been active
    f32 normal_impulse;        // Accumulated normal impulse
    Vec3 tangent_impulse;      // Accumulated tangential impulse
} ContactPoint;

// Contact manifold (all contact points between two bodies)
typedef struct ContactManifold {
    u32 body_a_id;
    u32 body_b_id;
    RigidBody *body_a;
    RigidBody *body_b;
    Vec3 normal;               // Primary contact normal
    ContactPoint points[MAX_CONTACT_POINTS];
    u32 point_count;
    u32 max_points;
    f32 friction;
    f32 restitution;
    bool touching;
    u32 frame_id;              // Frame last updated
} ContactManifold;

/**
 * ContactManifold: Multi-point contact representation
 *
 * Properties:
 *   - Up to 4 contact points per manifold
 *   - Persistent contact tracking
 *   - Face-face clipping for precise contacts
 *   - Edge-edge contact detection
 *   - Friction cone visualization
 *
 * Use cases:
 *   - Accurate contact representation
 *   - Stacking and resting contacts
 *   - Friction cone constraints
 */

// ============================================================================
// Manifold Creation and Destruction
// ============================================================================

/**
 * Create contact manifold
 *
 * Args:
 *   body_a_id: First body ID
 *   body_b_id: Second body ID
 *   body_a: First RigidBody
 *   body_b: Second RigidBody
 *
 * Returns:
 *   New ContactManifold (must be freed with contact_manifold_free)
 */
ContactManifold *contact_manifold_create(u32 body_a_id, u32 body_b_id, RigidBody *body_a, RigidBody *body_b);

/**
 * Free contact manifold
 *
 * Args:
 *   manifold: ContactManifold to free (NULL-safe)
 */
void contact_manifold_free(ContactManifold *manifold);

/**
 * Reset manifold (clear contact points)
 *
 * Args:
 *   manifold: ContactManifold
 */
void contact_manifold_reset(ContactManifold *manifold);

// ============================================================================
// Contact Point Management
// ============================================================================

/**
 * Add contact point to manifold
 *
 * Args:
 *   manifold: ContactManifold
 *   pos_a: Contact position on body A
 *   pos_b: Contact position on body B
 *   normal: Contact normal
 *   depth: Penetration depth
 *
 * Returns:
 *   true if point added (false if manifold full)
 */
bool contact_manifold_add_point(ContactManifold *manifold, Vec3 pos_a, Vec3 pos_b, Vec3 normal, f32 depth);

/**
 * Remove contact point
 *
 * Args:
 *   manifold: ContactManifold
 *   index: Point index to remove
 *
 * Returns:
 *   true if removed
 */
bool contact_manifold_remove_point(ContactManifold *manifold, u32 index);

/**
 * Get contact point
 *
 * Args:
 *   manifold: ContactManifold
 *   index: Point index
 *
 * Returns:
 *   ContactPoint pointer or NULL
 */
ContactPoint *contact_manifold_get_point(ContactManifold *manifold, u32 index);

/**
 * Get contact point count
 *
 * Args:
 *   manifold: ContactManifold
 *
 * Returns:
 *   Number of active contact points
 */
u32 contact_manifold_point_count(ContactManifold *manifold);

// ============================================================================
// Point Reduction and Clustering
// ============================================================================

/**
 * Reduce manifold to most important points
 *
 * Args:
 *   manifold: ContactManifold
 *   max_points: Maximum points to keep
 *
 * Note: Keeps points that are most separated
 */
void contact_manifold_reduce_points(ContactManifold *manifold, u32 max_points);

/**
 * Select 4 best contact points from candidates
 *
 * Args:
 *   points: Array of candidate points
 *   count: Number of candidates
 *   selected: Output array for selected points (must fit 4)
 *
 * Returns:
 *   Number of selected points (max 4)
 */
u32 contact_manifold_select_best_points(ContactPoint *points, u32 count, ContactPoint *selected);

/**
 * Cluster overlapping contact points
 *
 * Args:
 *   manifold: ContactManifold
 *   merge_distance: Distance threshold for merging
 */
void contact_manifold_cluster_points(ContactManifold *manifold, f32 merge_distance);

/**
 * Merge duplicate contact points
 *
 * Args:
 *   manifold: ContactManifold
 *
 * Returns:
 *   Number of merged points
 */
u32 contact_manifold_merge_duplicates(ContactManifold *manifold);

// ============================================================================
// Persistence and Tracking
// ============================================================================

/**
 * Update contact persistence (age tracking)
 *
 * Args:
 *   manifold: ContactManifold
 */
void contact_manifold_update_persistence(ContactManifold *manifold);

/**
 * Get persistence age of contact point
 *
 * Args:
 *   point: ContactPoint
 *
 * Returns:
 *   Number of frames this point has been active
 */
u32 contact_manifold_get_persistence(ContactPoint *point);

/**
 * Check if contact is newly formed
 *
 * Args:
 *   manifold: ContactManifold
 *
 * Returns:
 *   true if manifold is new
 */
bool contact_manifold_is_new(ContactManifold *manifold);

/**
 * Check if contact is persistent (resting)
 *
 * Args:
 *   manifold: ContactManifold
 *
 * Returns:
 *   true if contact has been present multiple frames
 */
bool contact_manifold_is_persistent(ContactManifold *manifold);

/**
 * Set contact state
 *
 * Args:
 *   manifold: ContactManifold
 *   state: ContactPointState
 */
void contact_manifold_set_state(ContactManifold *manifold, ContactPointState state);

// ============================================================================
// Face-Face Clipping
// ============================================================================

/**
 * Clip contact points for face-face contact
 *
 * Args:
 *   manifold: ContactManifold
 *   face_normal_a: Normal of face on body A
 *   face_normal_b: Normal of face on body B
 *
 * Note: Improves manifold accuracy by clipping points to face edges
 */
void contact_manifold_clip_face_face(ContactManifold *manifold, Vec3 face_normal_a, Vec3 face_normal_b);

/**
 * Sutherland-Hodgman polygon clipping
 *
 * Args:
 *   input_points: Polygon vertices to clip
 *   input_count: Number of vertices
 *   plane_normal: Clipping plane normal
 *   plane_distance: Plane distance from origin
 *   output_points: Output clipped vertices
 *   max_output: Maximum output vertices
 *
 * Returns:
 *   Number of output vertices
 */
u32 contact_manifold_sutherland_hodgman(Vec3 *input_points, u32 input_count, Vec3 plane_normal, f32 plane_distance, Vec3 *output_points, u32 max_output);

// ============================================================================
// Edge-Edge Contact
// ============================================================================

/**
 * Detect and add edge-edge contact points
 *
 * Args:
 *   manifold: ContactManifold
 *   edge_a_start: Start of edge on body A
 *   edge_a_end: End of edge on body A
 *   edge_b_start: Start of edge on body B
 *   edge_b_end: End of edge on body B
 *   tolerance: Distance tolerance for contact
 *
 * Returns:
 *   true if edge contact detected
 */
bool contact_manifold_detect_edge_edge(ContactManifold *manifold, Vec3 edge_a_start, Vec3 edge_a_end, Vec3 edge_b_start, Vec3 edge_b_end, f32 tolerance);

/**
 * Find closest points on two line segments
 *
 * Args:
 *   p1: Start of segment 1
 *   p2: End of segment 1
 *   p3: Start of segment 2
 *   p4: End of segment 2
 *   closest_p1: Output closest point on segment 1
 *   closest_p2: Output closest point on segment 2
 *
 * Returns:
 *   Distance between closest points
 */
f32 contact_manifold_closest_segment_points(Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Vec3 *closest_p1, Vec3 *closest_p2);

// ============================================================================
// Friction and Constraints
// ============================================================================

/**
 * Set friction coefficient
 *
 * Args:
 *   manifold: ContactManifold
 *   friction: Friction coefficient (typically 0-1)
 */
void contact_manifold_set_friction(ContactManifold *manifold, f32 friction);

/**
 * Set restitution (bounciness)
 *
 * Args:
 *   manifold: ContactManifold
 *   restitution: Restitution coefficient (0-1)
 */
void contact_manifold_set_restitution(ContactManifold *manifold, f32 restitution);

/**
 * Compute friction cone constraint
 *
 * Args:
 *   point: ContactPoint
 *   normal_impulse: Applied normal impulse
 *   max_friction_impulse: Output maximum friction impulse
 *
 * Note: Enforces Coulomb friction law
 */
void contact_manifold_compute_friction_cone(ContactPoint *point, f32 normal_impulse, f32 *max_friction_impulse);

/**
 * Clamp tangential impulse to friction cone
 *
 * Args:
 *   point: ContactPoint with tangent_impulse
 *   normal_impulse: Normal impulse applied
 *   friction: Friction coefficient
 *
 * Note: Modifies point->tangent_impulse
 */
void contact_manifold_clamp_friction_impulse(ContactPoint *point, f32 normal_impulse, f32 friction);

// ============================================================================
// Visualization and Diagnostics
// ============================================================================

/**
 * Visualize manifold (debug drawing)
 *
 * Args:
 *   manifold: ContactManifold to visualize
 *   buffer: Output buffer for visualization data
 *   buffer_size: Buffer size
 *
 * Returns:
 *   Number of bytes written
 */
u32 contact_manifold_visualize(ContactManifold *manifold, char *buffer, u32 buffer_size);

/**
 * Draw friction cones for all contact points
 *
 * Args:
 *   manifold: ContactManifold
 *   buffer: Output buffer
 *   buffer_size: Buffer size
 *
 * Returns:
 *   Number of bytes written
 */
u32 contact_manifold_visualize_friction_cones(ContactManifold *manifold, char *buffer, u32 buffer_size);

/**
 * Print manifold information
 *
 * Args:
 *   manifold: ContactManifold
 */
void contact_manifold_print_info(ContactManifold *manifold);

// ============================================================================
// Testing and Validation
// ============================================================================

/**
 * Validate manifold integrity
 *
 * Args:
 *   manifold: ContactManifold
 *
 * Returns:
 *   true if manifold is valid
 */
bool contact_manifold_validate(ContactManifold *manifold);

/**
 * Run comprehensive tests
 *
 * Returns:
 *   0 if all tests passed
 */
u32 contact_manifold_run_tests(void);

/**
 * Test face-face clipping accuracy
 *
 * Returns:
 *   true if clipping works correctly
 */
bool contact_manifold_test_clipping(void);

/**
 * Test edge-edge detection
 *
 * Returns:
 *   true if edge detection works
 */
bool contact_manifold_test_edge_detection(void);

#ifdef __cplusplus
}
#endif

#endif // CONTACT_MANIFOLD_H

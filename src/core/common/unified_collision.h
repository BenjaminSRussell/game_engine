/*
 * unified_collision.h
 * Unified collision system - consolidates collision detection across all subsystems
 * 
 * This header provides common collision detection utilities, data structures,
 * and functions to eliminate code duplication across cloth, physics, and other systems.
 */

#ifndef UNIFIED_COLLISION_H
#define UNIFIED_COLLISION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <float.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * COMMON COLLISION TYPES AND CONSTANTS
 * ============================================================================ */

#define UNIFIED_COLLISION_INVALID_ID 0xFFFFFFFFu
#define UNIFIED_COLLISION_MAX_PRIMITIVES 8192
#define UNIFIED_COLLISION_MAX_CONTACT_POINTS 16
#define UNIFIED_COLLISION_TOLERANCE 1e-6f
#define UNIFIED_COLLISION_BIAS_FACTOR 0.2f
#define UNIFIED_COLLISION_SLOP_FACTOR 0.005f

/* Collision primitive types */
typedef enum {
    COLLISION_PRIMITIVE_SPHERE = 0,
    COLLISION_PRIMITIVE_BOX,
    COLLISION_PRIMITIVE_CAPSULE,
    COLLISION_PRIMITIVE_MESH,
    COLLISION_PRIMITIVE_HEIGHTFIELD,
    COLLISION_PRIMITIVE_CONVEX_HULL,
    COLLISION_PRIMITIVE_COUNT
} collision_primitive_type_t;

/* Collision response types */
typedef enum {
    COLLISION_RESPONSE_NONE = 0,
    COLLISION_RESPONSE_SLIDE,
    COLLISION_RESPONSE_BOUNCE,
    COLLISION_RESPONSE_STICK,
    COLLISION_RESPONSE_COUNT
} collision_response_type_t;

/* ============================================================================
 * VECTOR MATH UTILITIES (Consolidated from multiple systems)
 * ============================================================================ */

typedef struct {
    float x, y, z;
} vec3_t;

typedef struct {
    float x, y, z, w;
} vec4_t;

typedef struct {
    float m[4][4];
} mat4_t;

/* Vector operations - consolidated from cloth_collision.c and other files */
static inline void vec3_copy(vec3_t *dst, const vec3_t *src) {
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
}

static inline void vec3_add(vec3_t *out, const vec3_t *a, const vec3_t *b) {
    out->x = a->x + b->x;
    out->y = a->y + b->y;
    out->z = a->z + b->z;
}

static inline void vec3_sub(vec3_t *out, const vec3_t *a, const vec3_t *b) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
}

static inline void vec3_mul(vec3_t *out, const vec3_t *v, float s) {
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}

static inline float vec3_dot(const vec3_t *a, const vec3_t *b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static inline float vec3_length_sq(const vec3_t *v) {
    return vec3_dot(v, v);
}

static inline float vec3_length(const vec3_t *v) {
    return sqrtf(vec3_length_sq(v));
}

static inline void vec3_normalize(vec3_t *out, const vec3_t *v) {
    float len = vec3_length(v);
    if (len > UNIFIED_COLLISION_TOLERANCE) {
        vec3_mul(out, v, 1.0f / len);
    } else {
        out->x = 0.0f;
        out->y = 1.0f;
        out->z = 0.0f;
    }
}

static inline void vec3_cross(vec3_t *out, const vec3_t *a, const vec3_t *b) {
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

static inline void vec3_lerp(vec3_t *out, const vec3_t *a, const vec3_t *b, float t) {
    out->x = a->x + t * (b->x - a->x);
    out->y = a->y + t * (b->y - a->y);
    out->z = a->z + t * (b->z - a->z);
}

/* ============================================================================
 * COLLISION PRIMITIVES
 * ============================================================================ */

typedef struct {
    collision_primitive_type_t type;
    vec3_t position;
    mat4_t transform;
    bool enabled;
    uint32_t user_data;
    float friction;
    float restitution;
    collision_response_type_t response_type;
} collision_primitive_t;

typedef struct {
    vec3_t center;
    float radius;
} collision_sphere_t;

typedef struct {
    vec3_t center;
    vec3_t extents; /* Half-sizes */
} collision_box_t;

typedef struct {
    vec3_t start;
    vec3_t end;
    float radius;
} collision_capsule_t;

typedef struct {
    vec3_t *vertices;
    uint32_t *indices;
    uint32_t vertex_count;
    uint32_t index_count;
} collision_mesh_t;

/* ============================================================================
 * CONTACT DATA
 * ============================================================================ */

typedef struct {
    vec3_t point;
    vec3_t normal;
    float penetration_depth;
    float impulse;
    bool valid;
} collision_contact_t;

typedef struct {
    collision_contact_t contacts[UNIFIED_COLLISION_MAX_CONTACT_POINTS];
    uint32_t contact_count;
    uint32_t primitive_a;
    uint32_t primitive_b;
    vec3_t relative_velocity;
    float combined_restitution;
    float combined_friction;
} collision_pair_t;

/* ============================================================================
 * COLLISION WORLD
 * ============================================================================ */

typedef struct {
    collision_primitive_t *primitives;
    uint32_t count;
    uint32_t capacity;
    collision_pair_t *pairs;
    uint32_t pair_count;
    uint32_t pair_capacity;
    bool *broadphase_pairs;
    uint32_t broadphase_size;
} collision_world_t;

/* ============================================================================
 * COLLISION STATISTICS
 * ============================================================================ */

typedef struct {
    uint32_t total_primitives;
    uint32_t active_primitives;
    uint32_t broadphase_tests;
    uint32_t narrowphase_tests;
    uint32_t collision_pairs;
    uint32_t contact_points;
    float total_broadphase_time;
    float total_narrowphase_time;
    float total_resolution_time;
} collision_stats_t;

/* ============================================================================
 * UNIFIED COLLISION API
 * ============================================================================ */

/* World management */
collision_world_t* collision_world_create(uint32_t max_primitives);
void collision_world_destroy(collision_world_t *world);
void collision_world_clear(collision_world_t *world);

/* Primitive management */
uint32_t collision_add_sphere(collision_world_t *world, const vec3_t *center, float radius);
uint32_t collision_add_box(collision_world_t *world, const vec3_t *center, const vec3_t *extents);
uint32_t collision_add_capsule(collision_world_t *world, const vec3_t *start, const vec3_t *end, float radius);
uint32_t collision_add_mesh(collision_world_t *world, const vec3_t *vertices, uint32_t vertex_count, 
                            const uint32_t *indices, uint32_t index_count);
void collision_remove_primitive(collision_world_t *world, uint32_t primitive_id);
void collision_set_primitive_transform(collision_world_t *world, uint32_t primitive_id, const mat4_t *transform);

/* Collision detection */
void collision_update_broadphase(collision_world_t *world);
void collision_update_narrowphase(collision_world_t *world);
void collision_resolve_contacts(collision_world_t *world, float dt);

/* Queries */
bool collision_ray_cast(const collision_world_t *world, const vec3_t *start, const vec3_t *direction, 
                       float max_distance, collision_contact_t *result);
bool collision_sphere_cast(const collision_world_t *world, const vec3_t *start, float radius, 
                          const vec3_t *direction, float max_distance, collision_contact_t *result);
uint32_t collision_query_aabb(const collision_world_t *world, const vec3_t *min, const vec3_t *max, 
                             uint32_t *results, uint32_t max_results);

/* Statistics */
void collision_get_stats(const collision_world_t *world, collision_stats_t *stats);
void collision_reset_stats(collision_world_t *world);

/* ============================================================================
 * COLLISION DETECTION FUNCTIONS (Consolidated from multiple systems)
 * ============================================================================ */

/* Sphere collisions */
bool collision_sphere_sphere(const collision_sphere_t *a, const collision_sphere_t *b, collision_contact_t *contact);
bool collision_sphere_box(const collision_sphere_t *sphere, const collision_box_t *box, collision_contact_t *contact);
bool collision_sphere_capsule(const collision_sphere_t *sphere, const collision_capsule_t *capsule, collision_contact_t *contact);

/* Box collisions */
bool collision_box_box(const collision_box_t *a, const collision_box_t *b, collision_contact_t *contact);
bool collision_box_capsule(const collision_box_t *box, const collision_capsule_t *capsule, collision_contact_t *contact);

/* Capsule collisions */
bool collision_capsule_capsule(const collision_capsule_t *a, const collision_capsule_t *b, collision_contact_t *contact);

/* Mesh collisions (simplified) */
bool collision_sphere_mesh(const collision_sphere_t *sphere, const collision_mesh_t *mesh, collision_contact_t *contact);
bool collision_box_mesh(const collision_box_t *box, const collision_mesh_t *mesh, collision_contact_t *contact);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/* Distance calculations */
float distance_point_to_plane(const vec3_t *point, const vec3_t *plane_normal, float plane_distance);
float distance_point_to_line(const vec3_t *point, const vec3_t *line_start, const vec3_t *line_end);
float distance_point_to_triangle(const vec3_t *point, const vec3_t *a, const vec3_t *b, const vec3_t *c);

/* Closest point calculations */
void closest_point_on_line(const vec3_t *point, const vec3_t *line_start, const vec3_t *line_end, vec3_t *closest);
void closest_point_on_triangle(const vec3_t *point, const vec3_t *a, const vec3_t *b, const vec3_t *c, vec3_t *closest);

/* Intersection tests */
bool ray_sphere_intersect(const vec3_t *ray_start, const vec3_t *ray_dir, const vec3_t *sphere_center, 
                        float sphere_radius, float *t);
bool ray_box_intersect(const vec3_t *ray_start, const vec3_t *ray_dir, const vec3_t *box_min, 
                     const vec3_t *box_max, float *t);
bool ray_plane_intersect(const vec3_t *ray_start, const vec3_t *ray_dir, const vec3_t *plane_normal, 
                        float plane_distance, float *t);

#ifdef __cplusplus
}
#endif

#endif /* UNIFIED_COLLISION_H */

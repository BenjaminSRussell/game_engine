/*
 * ray_tracer.h
 *
 * Ray tracing system for realistic lighting and reflections
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * Provides functionality for:
 * - Ray-scene intersection testing
 * - Monte Carlo path tracing
 * - Global illumination computation
 * - Reflection and refraction
 * - Shadow ray casting
 */

#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

/* 3D Vector */
typedef struct {
    float x, y, z;
} Vec3;

/* Ray structure */
typedef struct {
    Vec3 origin;
    Vec3 direction;
    float t_min;
    float t_max;
} Ray;

/* Hit information */
typedef struct {
    bool hit;
    float t;              /* Distance along ray */
    Vec3 position;        /* Intersection point */
    Vec3 normal;          /* Surface normal */
    Vec3 tangent;         /* Surface tangent */
    uint32_t material_id;
    uint32_t triangle_id;
} HitInfo;

/* Material properties */
typedef struct {
    Vec3 albedo;          /* Base color */
    float metallic;       /* Metal factor 0-1 */
    float roughness;      /* Surface roughness 0-1 */
    float emission;       /* Emission strength */
    float ior;            /* Index of refraction */
    bool transmissive;    /* Is transmissive (glass) */
} Material;

/* Triangle mesh for ray tracing */
typedef struct {
    Vec3* vertices;
    uint32_t vertex_count;
    uint32_t* indices;
    uint32_t triangle_count;
    Vec3* normals;
    uint32_t material_id;
} RayTriangleMesh;

/* BVH node for acceleration */
typedef struct BVHNode {
    Vec3 aabb_min;
    Vec3 aabb_max;
    struct BVHNode* left;
    struct BVHNode* right;
    uint32_t triangle_start;
    uint32_t triangle_count;
    bool is_leaf;
} BVHNode;

/* Ray tracing scene */
typedef struct {
    RayTriangleMesh* meshes;
    uint32_t mesh_count;
    Material* materials;
    uint32_t material_count;
    BVHNode* bvh_root;
    uint32_t max_bounces;
    uint32_t samples_per_pixel;
} RayScene;

/* Ray tracer context */
typedef struct RayTracer RayTracer;

/* Sampling modes */
typedef enum {
    SAMPLING_UNIFORM = 0,
    SAMPLING_COSINE_WEIGHTED,
    SAMPLING_IMPORTANCE,
} SamplingMode;

/* Ray tracing statistics */
typedef struct {
    uint64_t rays_cast;
    uint64_t intersections_tested;
    uint64_t hits;
    double average_intersection_time_ms;
    double total_tracing_time_ms;
} RayTracingStats;

/* ============================================================================
 * API FUNCTIONS
 * ============================================================================ */

/* Create and destroy ray tracer */
RayTracer* ray_tracer_create(void);
void ray_tracer_destroy(RayTracer* tracer);

/* Scene management */
int ray_tracer_add_mesh(
    RayTracer* tracer,
    const RayTriangleMesh* mesh
);

int ray_tracer_add_material(
    RayTracer* tracer,
    const Material* material
);

int ray_tracer_build_bvh(RayTracer* tracer);

/* Ray casting */
int ray_tracer_cast_ray(
    RayTracer* tracer,
    const Ray* ray,
    HitInfo* hit
);

int ray_tracer_trace_path(
    RayTracer* tracer,
    const Ray* ray,
    Vec3* result_color,
    uint32_t depth
);

int ray_tracer_cast_shadow_ray(
    RayTracer* tracer,
    const Vec3* from,
    const Vec3* to,
    bool* in_shadow
);

/* Batch operations */
typedef struct {
    Ray* rays;
    uint32_t ray_count;
    HitInfo* hits;
    bool* hit_flags;
} RayBatch;

int ray_tracer_cast_ray_batch(
    RayTracer* tracer,
    RayBatch* batch
);

/* Sampling utilities */
Vec3 ray_tracer_sample_cosine_hemisphere(float u1, float u2);
Vec3 ray_tracer_sample_uniform_sphere(float u1, float u2);
Vec3 ray_tracer_reflect(const Vec3* direction, const Vec3* normal);
Vec3 ray_tracer_refract(
    const Vec3* direction,
    const Vec3* normal,
    float ior_ratio,
    bool* total_internal_reflection
);

/* Fresnel and BRDF */
float ray_tracer_fresnel_schlick(float cos_theta, float f0);
Vec3 ray_tracer_evaluate_brdf(
    const Vec3* view_dir,
    const Vec3* light_dir,
    const Vec3* normal,
    const Material* material
);

/* Utility functions */
Ray ray_tracer_create_ray(const Vec3* origin, const Vec3* direction);
Vec3 ray_tracer_ray_at(const Ray* ray, float t);
float ray_tracer_vec3_length(const Vec3* v);
Vec3 ray_tracer_vec3_normalize(const Vec3* v);
Vec3 ray_tracer_vec3_add(const Vec3* a, const Vec3* b);
Vec3 ray_tracer_vec3_sub(const Vec3* a, const Vec3* b);
Vec3 ray_tracer_vec3_mul(const Vec3* v, float s);
float ray_tracer_vec3_dot(const Vec3* a, const Vec3* b);
Vec3 ray_tracer_vec3_cross(const Vec3* a, const Vec3* b);

/* Statistics */
int ray_tracer_get_statistics(
    RayTracer* tracer,
    RayTracingStats* stats
);

void ray_tracer_reset_statistics(RayTracer* tracer);

/* Configuration */
int ray_tracer_set_max_bounces(RayTracer* tracer, uint32_t bounces);
int ray_tracer_set_samples_per_pixel(RayTracer* tracer, uint32_t samples);
int ray_tracer_set_sampling_mode(RayTracer* tracer, SamplingMode mode);

#ifdef __cplusplus
}
#endif

#endif // RAY_TRACER_H

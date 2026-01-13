/*
 * ray_tracer.c
 *
 * Ray tracing system implementation
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 */

#include "ray_tracer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define RAY_TRACER_MAX_MESHES 4096
#define RAY_TRACER_MAX_MATERIALS 1024
#define EPSILON 1e-6f
#define PI 3.14159265359f

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

struct RayTracer {
    RayTriangleMesh* meshes;
    uint32_t mesh_count;
    Material* materials;
    uint32_t material_count;
    BVHNode* bvh_root;

    /* Configuration */
    uint32_t max_bounces;
    uint32_t samples_per_pixel;
    SamplingMode sampling_mode;

    /* Statistics */
    RayTracingStats stats;
};

/* ============================================================================
 * VECTOR MATH IMPLEMENTATION
 * ============================================================================ */

static float vec3_length(const Vec3* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

static Vec3 vec3_normalize(const Vec3* v) {
    float len = vec3_length(v);
    if (len < EPSILON) {
        return (Vec3){0, 0, 0};
    }
    return (Vec3){v->x / len, v->y / len, v->z / len};
}

static Vec3 vec3_add(const Vec3* a, const Vec3* b) {
    return (Vec3){a->x + b->x, a->y + b->y, a->z + b->z};
}

static Vec3 vec3_sub(const Vec3* a, const Vec3* b) {
    return (Vec3){a->x - b->x, a->y - b->y, a->z - b->z};
}

static Vec3 vec3_mul(const Vec3* v, float s) {
    return (Vec3){v->x * s, v->y * s, v->z * s};
}

static float vec3_dot(const Vec3* a, const Vec3* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static Vec3 vec3_cross(const Vec3* a, const Vec3* b) {
    return (Vec3){
        a->y * b->z - a->z * b->y,
        a->z * b->x - a->x * b->z,
        a->x * b->y - a->y * b->x
    };
}

static Vec3 vec3_lerp(const Vec3* a, const Vec3* b, float t) {
    return (Vec3){
        a->x + (b->x - a->x) * t,
        a->y + (b->y - a->y) * t,
        a->z + (b->z - a->z) * t
    };
}

/* ============================================================================
 * BVH CONSTRUCTION
 * ============================================================================ */

static BVHNode* bvh_build_node(
    RayTriangleMesh* mesh,
    uint32_t* triangle_indices,
    uint32_t start,
    uint32_t count) {

    if (!mesh || count == 0) {
        return NULL;
    }

    BVHNode* node = (BVHNode*)malloc(sizeof(BVHNode));
    if (!node) {
        return NULL;
    }

    memset(node, 0, sizeof(BVHNode));

    /* Compute AABB */
    node->aabb_min = (Vec3){FLT_MAX, FLT_MAX, FLT_MAX};
    node->aabb_max = (Vec3){-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (uint32_t i = start; i < start + count; i++) {
        uint32_t tri_idx = triangle_indices[i];
        uint32_t i0 = mesh->indices[tri_idx * 3];
        uint32_t i1 = mesh->indices[tri_idx * 3 + 1];
        uint32_t i2 = mesh->indices[tri_idx * 3 + 2];

        Vec3* v0 = &mesh->vertices[i0];
        Vec3* v1 = &mesh->vertices[i1];
        Vec3* v2 = &mesh->vertices[i2];

        node->aabb_min.x = fminf(node->aabb_min.x, fminf(v0->x, fminf(v1->x, v2->x)));
        node->aabb_min.y = fminf(node->aabb_min.y, fminf(v0->y, fminf(v1->y, v2->y)));
        node->aabb_min.z = fminf(node->aabb_min.z, fminf(v0->z, fminf(v1->z, v2->z)));

        node->aabb_max.x = fmaxf(node->aabb_max.x, fmaxf(v0->x, fmaxf(v1->x, v2->x)));
        node->aabb_max.y = fmaxf(node->aabb_max.y, fmaxf(v0->y, fmaxf(v1->y, v2->y)));
        node->aabb_max.z = fmaxf(node->aabb_max.z, fmaxf(v0->z, fmaxf(v1->z, v2->z)));
    }

    /* Create leaf if small enough */
    if (count <= 4) {
        node->is_leaf = true;
        node->triangle_start = triangle_indices[start];
        node->triangle_count = count;
        return node;
    }

    /* Split along longest axis */
    Vec3 extent = vec3_sub(&node->aabb_max, &node->aabb_min);
    int split_axis = 0;
    if (extent.y > extent.x) split_axis = 1;
    if (extent.z > ((split_axis == 0) ? extent.x : extent.y)) split_axis = 2;

    float split_pos = 0.5f * (
        (split_axis == 0) ? (node->aabb_min.x + node->aabb_max.x) :
        (split_axis == 1) ? (node->aabb_min.y + node->aabb_max.y) :
                           (node->aabb_min.z + node->aabb_max.z)
    );

    uint32_t mid = start;
    for (uint32_t i = start; i < start + count; i++) {
        uint32_t tri_idx = triangle_indices[i];
        uint32_t i0 = mesh->indices[tri_idx * 3];
        Vec3 centroid = mesh->vertices[i0];

        float pos = (split_axis == 0) ? centroid.x :
                   (split_axis == 1) ? centroid.y : centroid.z;

        if (pos < split_pos) {
            uint32_t tmp = triangle_indices[mid];
            triangle_indices[mid] = triangle_indices[i];
            triangle_indices[i] = tmp;
            mid++;
        }
    }

    if (mid == start || mid == start + count) {
        mid = start + count / 2;
    }

    node->left = bvh_build_node(mesh, triangle_indices, start, mid - start);
    node->right = bvh_build_node(mesh, triangle_indices, mid, start + count - mid);

    return node;
}

/* ============================================================================
 * RAY INTERSECTION TESTING
 * ============================================================================ */

static bool ray_triangle_intersect(
    const Ray* ray,
    const Vec3* v0,
    const Vec3* v1,
    const Vec3* v2,
    float* t,
    Vec3* normal) {

    Vec3 edge1 = vec3_sub(v1, v0);
    Vec3 edge2 = vec3_sub(v2, v0);
    Vec3 ray_cross_edge2 = vec3_cross(&ray->direction, &edge2);
    float det = vec3_dot(&edge1, &ray_cross_edge2);

    if (fabsf(det) < EPSILON) {
        return false;  /* Ray parallel to triangle */
    }

    float inv_det = 1.0f / det;
    Vec3 to_origin = vec3_sub(&ray->origin, v0);
    float u = vec3_dot(&to_origin, &ray_cross_edge2) * inv_det;

    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    Vec3 origin_cross_edge1 = vec3_cross(&to_origin, &edge1);
    float v = vec3_dot(&ray->direction, &origin_cross_edge1) * inv_det;

    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    *t = vec3_dot(&edge2, &origin_cross_edge1) * inv_det;

    if (*t < ray->t_min || *t > ray->t_max) {
        return false;
    }

    /* Compute normal */
    *normal = vec3_cross(&edge1, &edge2);
    *normal = vec3_normalize(normal);

    return true;
}

static bool ray_aabb_intersect(const Ray* ray, const Vec3* aabb_min, const Vec3* aabb_max) {
    float tx_min = (aabb_min->x - ray->origin.x) / (ray->direction.x + EPSILON);
    float tx_max = (aabb_max->x - ray->origin.x) / (ray->direction.x + EPSILON);
    if (tx_min > tx_max) {
        float tmp = tx_min;
        tx_min = tx_max;
        tx_max = tmp;
    }

    float ty_min = (aabb_min->y - ray->origin.y) / (ray->direction.y + EPSILON);
    float ty_max = (aabb_max->y - ray->origin.y) / (ray->direction.y + EPSILON);
    if (ty_min > ty_max) {
        float tmp = ty_min;
        ty_min = ty_max;
        ty_max = tmp;
    }

    float tz_min = (aabb_min->z - ray->origin.z) / (ray->direction.z + EPSILON);
    float tz_max = (aabb_max->z - ray->origin.z) / (ray->direction.z + EPSILON);
    if (tz_min > tz_max) {
        float tmp = tz_min;
        tz_min = tz_max;
        tz_max = tmp;
    }

    float t_enter = fmaxf(fmaxf(tx_min, ty_min), tz_min);
    float t_exit = fminf(fminf(tx_max, ty_max), tz_max);

    return t_enter <= t_exit && t_exit >= 0.0f;
}

static void ray_intersect_bvh_recursive(
    const Ray* ray,
    const BVHNode* node,
    RayTriangleMesh* mesh,
    HitInfo* best_hit) {

    if (!node || !ray_aabb_intersect(ray, &node->aabb_min, &node->aabb_max)) {
        return;
    }

    if (node->is_leaf) {
        for (uint32_t i = 0; i < node->triangle_count; i++) {
            uint32_t tri_idx = node->triangle_start + i;
            uint32_t i0 = mesh->indices[tri_idx * 3];
            uint32_t i1 = mesh->indices[tri_idx * 3 + 1];
            uint32_t i2 = mesh->indices[tri_idx * 3 + 2];

            Vec3 v0 = mesh->vertices[i0];
            Vec3 v1 = mesh->vertices[i1];
            Vec3 v2 = mesh->vertices[i2];

            float t;
            Vec3 normal;
            if (ray_triangle_intersect(ray, &v0, &v1, &v2, &t, &normal)) {
                if (t < best_hit->t) {
                    best_hit->t = t;
                    best_hit->position = ray_tracer_ray_at(ray, t);
                    best_hit->normal = normal;
                    best_hit->triangle_id = tri_idx;
                    best_hit->hit = true;
                }
            }
        }
        return;
    }

    ray_intersect_bvh_recursive(ray, node->left, mesh, best_hit);
    ray_intersect_bvh_recursive(ray, node->right, mesh, best_hit);
}

/* ============================================================================
 * RANDOM NUMBER GENERATION
 * ============================================================================ */

static uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static float random_float(uint32_t* seed) {
    return (float)(xorshift32(seed) & 0x7FFFFFFF) / 0x7FFFFFFF;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

RayTracer* ray_tracer_create(void) {
    RayTracer* tracer = (RayTracer*)malloc(sizeof(RayTracer));
    if (!tracer) {
        return NULL;
    }

    memset(tracer, 0, sizeof(RayTracer));

    tracer->meshes = (RayTriangleMesh*)calloc(RAY_TRACER_MAX_MESHES, sizeof(RayTriangleMesh));
    if (!tracer->meshes) {
        free(tracer);
        return NULL;
    }

    tracer->materials = (Material*)calloc(RAY_TRACER_MAX_MATERIALS, sizeof(Material));
    if (!tracer->materials) {
        free(tracer->meshes);
        free(tracer);
        return NULL;
    }

    tracer->max_bounces = 4;
    tracer->samples_per_pixel = 1;
    tracer->sampling_mode = SAMPLING_COSINE_WEIGHTED;

    return tracer;
}

void ray_tracer_destroy(RayTracer* tracer) {
    if (!tracer) {
        return;
    }

    free(tracer->meshes);
    free(tracer->materials);
    free(tracer->bvh_root);
    free(tracer);
}

int ray_tracer_add_mesh(RayTracer* tracer, const RayTriangleMesh* mesh) {
    if (!tracer || !mesh || tracer->mesh_count >= RAY_TRACER_MAX_MESHES) {
        return -1;
    }

    memcpy(&tracer->meshes[tracer->mesh_count], mesh, sizeof(RayTriangleMesh));
    tracer->mesh_count++;

    return 0;
}

int ray_tracer_add_material(RayTracer* tracer, const Material* material) {
    if (!tracer || !material || tracer->material_count >= RAY_TRACER_MAX_MATERIALS) {
        return -1;
    }

    memcpy(&tracer->materials[tracer->material_count], material, sizeof(Material));
    tracer->material_count++;

    return 0;
}

int ray_tracer_build_bvh(RayTracer* tracer) {
    if (!tracer || tracer->mesh_count == 0) {
        return -1;
    }

    /* For now, build BVH for first mesh only */
    RayTriangleMesh* mesh = &tracer->meshes[0];

    uint32_t* triangle_indices = (uint32_t*)malloc(mesh->triangle_count * sizeof(uint32_t));
    if (!triangle_indices) {
        return -2;
    }

    for (uint32_t i = 0; i < mesh->triangle_count; i++) {
        triangle_indices[i] = i;
    }

    tracer->bvh_root = bvh_build_node(mesh, triangle_indices, 0, mesh->triangle_count);
    free(triangle_indices);

    return tracer->bvh_root ? 0 : -3;
}

int ray_tracer_cast_ray(RayTracer* tracer, const Ray* ray, HitInfo* hit) {
    if (!tracer || !ray || !hit) {
        return -1;
    }

    memset(hit, 0, sizeof(HitInfo));
    hit->t = FLT_MAX;

    if (tracer->mesh_count == 0) {
        return -2;
    }

    if (tracer->bvh_root) {
        ray_intersect_bvh_recursive(ray, tracer->bvh_root, &tracer->meshes[0], hit);
    }

    tracer->stats.rays_cast++;
    if (hit->hit) {
        tracer->stats.hits++;
    }

    return hit->hit ? 0 : -3;
}

int ray_tracer_cast_shadow_ray(RayTracer* tracer, const Vec3* from, const Vec3* to, bool* in_shadow) {
    if (!tracer || !from || !to || !in_shadow) {
        return -1;
    }

    Vec3 dir = vec3_sub(to, from);
    float dist = vec3_length(&dir);
    dir = vec3_normalize(&dir);

    Ray shadow_ray = {
        .origin = *from,
        .direction = dir,
        .t_min = EPSILON,
        .t_max = dist - EPSILON
    };

    HitInfo hit;
    int result = ray_tracer_cast_ray(tracer, &shadow_ray, &hit);

    *in_shadow = (result == 0 && hit.hit);
    return 0;
}

int ray_tracer_trace_path(RayTracer* tracer, const Ray* ray, Vec3* result_color, uint32_t depth) {
    if (!tracer || !ray || !result_color || depth >= tracer->max_bounces) {
        *result_color = (Vec3){0, 0, 0};
        return 0;
    }

    HitInfo hit;
    if (ray_tracer_cast_ray(tracer, ray, &hit) != 0) {
        /* Sky color */
        *result_color = (Vec3){0.5f, 0.7f, 1.0f};
        return 0;
    }

    /* Simple Lambertian shading for now */
    Material* material = (hit.material_id < tracer->material_count) ?
                        &tracer->materials[hit.material_id] :
                        &tracer->materials[0];

    *result_color = material->albedo;

    /* Add simple indirect lighting */
    if (depth < tracer->max_bounces - 1) {
        Vec3 indirect = (Vec3){0, 0, 0};
        uint32_t samples = 4;

        for (uint32_t i = 0; i < samples; i++) {
            Vec3 bounce_dir = ray_tracer_sample_cosine_hemisphere(
                (float)rand() / RAND_MAX,
                (float)rand() / RAND_MAX
            );

            Ray bounce_ray = {
                .origin = hit.position,
                .direction = bounce_dir,
                .t_min = EPSILON,
                .t_max = FLT_MAX
            };

            Vec3 bounce_color;
            ray_tracer_trace_path(tracer, &bounce_ray, &bounce_color, depth + 1);

            indirect.x += bounce_color.x;
            indirect.y += bounce_color.y;
            indirect.z += bounce_color.z;
        }

        float inv_samples = 1.0f / samples;
        result_color->x += indirect.x * inv_samples * 0.2f;
        result_color->y += indirect.y * inv_samples * 0.2f;
        result_color->z += indirect.z * inv_samples * 0.2f;
    }

    return 0;
}

int ray_tracer_cast_ray_batch(RayTracer* tracer, RayBatch* batch) {
    if (!tracer || !batch || !batch->rays) {
        return -1;
    }

    for (uint32_t i = 0; i < batch->ray_count; i++) {
        HitInfo hit;
        int result = ray_tracer_cast_ray(tracer, &batch->rays[i], &hit);
        batch->hits[i] = hit;
        batch->hit_flags[i] = (result == 0 && hit.hit);
    }

    return 0;
}

/* ============================================================================
 * SAMPLING AND UTILITY FUNCTIONS
 * ============================================================================ */

Vec3 ray_tracer_sample_cosine_hemisphere(float u1, float u2) {
    float r = sqrtf(u1);
    float theta = 2.0f * PI * u2;
    float x = r * cosf(theta);
    float y = r * sinf(theta);
    float z = sqrtf(fmaxf(0.0f, 1.0f - u1));
    return (Vec3){x, y, z};
}

Vec3 ray_tracer_sample_uniform_sphere(float u1, float u2) {
    float theta = 2.0f * PI * u1;
    float phi = acosf(1.0f - 2.0f * u2);
    float sin_phi = sinf(phi);
    return (Vec3){
        sin_phi * cosf(theta),
        sin_phi * sinf(theta),
        cosf(phi)
    };
}

Vec3 ray_tracer_reflect(const Vec3* direction, const Vec3* normal) {
    float dot = vec3_dot(direction, normal);
    Vec3 reflected = vec3_mul(normal, 2.0f * dot);
    return vec3_sub(direction, &reflected);
}

Vec3 ray_tracer_refract(const Vec3* direction, const Vec3* normal, float ior_ratio, bool* tir) {
    float cos_i = -vec3_dot(direction, normal);
    float sin_t_sq = ior_ratio * ior_ratio * (1.0f - cos_i * cos_i);

    if (sin_t_sq > 1.0f) {
        *tir = true;
        return ray_tracer_reflect(direction, normal);
    }

    *tir = false;
    float cos_t = sqrtf(fmaxf(0.0f, 1.0f - sin_t_sq));

    Vec3 refracted = vec3_mul(direction, ior_ratio);
    Vec3 normal_term = vec3_mul(normal, ior_ratio * cos_i - cos_t);
    return vec3_add(&refracted, &normal_term);
}

float ray_tracer_fresnel_schlick(float cos_theta, float f0) {
    float x = 1.0f - fmaxf(0.0f, fminf(1.0f, cos_theta));
    float x5 = x * x * x * x * x;
    return f0 + (1.0f - f0) * x5;
}

Vec3 ray_tracer_evaluate_brdf(const Vec3* view_dir, const Vec3* light_dir, const Vec3* normal, const Material* mat) {
    float ndotl = fmaxf(0.0f, vec3_dot(normal, light_dir));
    return vec3_mul(&mat->albedo, ndotl / PI);
}

Ray ray_tracer_create_ray(const Vec3* origin, const Vec3* direction) {
    Vec3 norm_dir = vec3_normalize(direction);
    return (Ray){
        .origin = *origin,
        .direction = norm_dir,
        .t_min = EPSILON,
        .t_max = FLT_MAX
    };
}

Vec3 ray_tracer_ray_at(const Ray* ray, float t) {
    Vec3 scaled = vec3_mul(&ray->direction, t);
    return vec3_add(&ray->origin, &scaled);
}

float ray_tracer_vec3_length(const Vec3* v) {
    return vec3_length(v);
}

Vec3 ray_tracer_vec3_normalize(const Vec3* v) {
    return vec3_normalize(v);
}

Vec3 ray_tracer_vec3_add(const Vec3* a, const Vec3* b) {
    return vec3_add(a, b);
}

Vec3 ray_tracer_vec3_sub(const Vec3* a, const Vec3* b) {
    return vec3_sub(a, b);
}

Vec3 ray_tracer_vec3_mul(const Vec3* v, float s) {
    return vec3_mul(v, s);
}

float ray_tracer_vec3_dot(const Vec3* a, const Vec3* b) {
    return vec3_dot(a, b);
}

Vec3 ray_tracer_vec3_cross(const Vec3* a, const Vec3* b) {
    return vec3_cross(a, b);
}

int ray_tracer_get_statistics(RayTracer* tracer, RayTracingStats* stats) {
    if (!tracer || !stats) {
        return -1;
    }

    memcpy(stats, &tracer->stats, sizeof(RayTracingStats));
    return 0;
}

void ray_tracer_reset_statistics(RayTracer* tracer) {
    if (!tracer) {
        return;
    }

    memset(&tracer->stats, 0, sizeof(RayTracingStats));
}

int ray_tracer_set_max_bounces(RayTracer* tracer, uint32_t bounces) {
    if (!tracer) {
        return -1;
    }

    tracer->max_bounces = bounces;
    return 0;
}

int ray_tracer_set_samples_per_pixel(RayTracer* tracer, uint32_t samples) {
    if (!tracer) {
        return -1;
    }

    tracer->samples_per_pixel = samples;
    return 0;
}

int ray_tracer_set_sampling_mode(RayTracer* tracer, SamplingMode mode) {
    if (!tracer) {
        return -1;
    }

    tracer->sampling_mode = mode;
    return 0;
}

/* End of ray_tracer.c */

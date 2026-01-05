/*
 * meshlet_builder.h
 * Meshlet generation and structures for efficient geometry processing
 */

#ifndef MESHLET_BUILDER_H
#define MESHLET_BUILDER_H

#include <stdint.h>
#include <stdbool.h>

// Simple math structures (to be replaced by engine math headers if available)
typedef struct vec3 {
    float x, y, z;
} vec3_t;

typedef struct sphere {
    vec3_t center;
    float radius;
} sphere_t;

// Meshlet structure as requested
typedef struct meshlet {
    uint32_t vertex_offset;
    uint32_t vertex_count;
    uint32_t triangle_offset;
    uint32_t triangle_count;
    vec3_t cone_apex;
    vec3_t cone_axis;
    float cone_cutoff;
    sphere_t bounding_sphere;
} meshlet_t;

// Meshlet generation limits
#define MAX_MESHLET_VERTICES 64
#define MAX_MESHLET_TRIANGLES 124

// Result of meshlet building
typedef struct meshlet_result {
    meshlet_t* meshlets;
    uint32_t count;
    uint32_t* vertices;     // Local vertex indices
    uint32_t vertex_count;
    uint8_t* triangles;     // 3 * triangle_count indices (8-bit if local)
    uint32_t triangle_count;
} meshlet_result_t;

// Generation API
meshlet_result_t* build_meshlets(const float* vertices, uint32_t vertex_stride, uint32_t vertex_count,
                               const uint32_t* indices, uint32_t index_count);

void meshlet_result_free(meshlet_result_t* result);

#endif // MESHLET_BUILDER_H

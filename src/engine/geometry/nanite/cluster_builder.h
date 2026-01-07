#ifndef CLUSTER_BUILDER_H
#define CLUSTER_BUILDER_H

#include <Metal/Metal.h>
#include <simd/simd.h>
#include "geometry/vertex/vertex_format.h" // Assuming this path

#define CLUSTER_TRIANGLE_COUNT 128
#define CLUSTER_VERTEX_COUNT 256

typedef struct mesh_cluster {
    uint32_t vertex_offset;
    uint32_t index_offset;
    uint32_t triangle_count;
    simd_float3 bounds_center;
    simd_float3 bounds_extent;
    float lod_error;           // Screen-space error threshold
    uint32_t parent_cluster;   // For DAG
    uint32_t child_clusters[8];
    uint32_t child_count;
} mesh_cluster_t;

typedef struct cluster_mesh {
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffer;
    id<MTLBuffer> cluster_buffer;
    uint32_t cluster_count;
    uint32_t total_triangles;
} cluster_mesh_t;

// Build clusters from input mesh using meshoptimizer-style algorithm
cluster_mesh_t* cluster_mesh_build(id<MTLDevice> device,
                                   const vertex_t* vertices, uint32_t vertex_count,
                                   const uint32_t* indices, uint32_t index_count);

void cluster_mesh_free(cluster_mesh_t* mesh);

#endif // CLUSTER_BUILDER_H

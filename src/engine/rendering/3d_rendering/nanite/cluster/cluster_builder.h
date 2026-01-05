/*
 * cluster_builder.h
 * Nanite-style cluster hierarchy (DAG) construction
 */

#ifndef CLUSTER_BUILDER_H
#define CLUSTER_BUILDER_H

#include <stdint.h>
#include <stdbool.h>
#include "../../geometry/meshlets/meshlet_builder.h"

// Cluster structure as requested
typedef struct cluster {
    uint32_t meshlet_offset;
    uint32_t meshlet_count;
    uint32_t parent_cluster;
    uint32_t child_clusters[8];
    float error;  // Screen-space error at which to use this LOD
    sphere_t bounds;
} cluster_t;

// Result of cluster hierarchy building
typedef struct cluster_result {
    cluster_t* clusters;
    uint32_t count;
    uint32_t root_cluster;
} cluster_result_t;

// Build DAG from meshlets
cluster_result_t* build_cluster_hierarchy(const meshlet_t* meshlets, uint32_t meshlet_count);

void cluster_result_free(cluster_result_t* result);

#endif // CLUSTER_BUILDER_H

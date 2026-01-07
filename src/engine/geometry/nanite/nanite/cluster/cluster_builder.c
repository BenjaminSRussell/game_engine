/*
 * cluster_builder.c
 * Implementation of cluster hierarchy (DAG) building
 */

#include "geometry/nanite/nanite/cluster/cluster_builder.h"
#include <stdlib.h>
#include <string.h>

// Simple bottom-up cluster hierarchy builder
// In a real Nanite implementation, this involves:
// 1. Grouping clusters (e.g. 2 or 4)
// 2. Simplifying the geometry of grouped clusters
// 3. Repeating until we reach the root
cluster_result_t* build_cluster_hierarchy(const meshlet_t* meshlets, uint32_t meshlet_count) {
    if (!meshlets || meshlet_count == 0) return NULL;

    cluster_result_t* result = (cluster_result_t*)malloc(sizeof(cluster_result_t));
    if (!result) return NULL;

    // Initial capacity: rough estimate for a DAG/Tree
    uint32_t max_clusters = meshlet_count * 2; 
    result->clusters = (cluster_t*)malloc(max_clusters * sizeof(cluster_t));
    result->count = 0;

    // Leaf Clusters: 1 meshlet per cluster for simplicity in this initial implementation
    for (uint32_t i = 0; i < meshlet_count; ++i) {
        cluster_t* c = &result->clusters[result->count++];
        c->meshlet_offset = i;
        c->meshlet_count = 1;
        c->parent_cluster = 0xFFFFFFFF;
        memset(c->child_clusters, 0xFF, sizeof(c->child_clusters));
        c->error = 0.0f; // Leaf clusters have 0 error
        c->bounds = meshlets[i].bounding_sphere;
    }

    // TODO: Implement clustering of leaf nodes and recursive simplification
    // This involves finding adjacent clusters and merging them.

    result->root_cluster = result->count > 0 ? result->count - 1 : 0;

    return result;
}

void cluster_result_free(cluster_result_t* result) {
    if (!result) return;
    if (result->clusters) free(result->clusters);
    free(result);
}

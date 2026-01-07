/*
 * cluster_dag.c
 * Implementation of cluster DAG management
 */

#include "geometry/nanite/nanite/cluster/cluster_dag.h"
#include <stdio.h>
#include <stdlib.h>

void cluster_dag_traverse_depth_first(const cluster_result_t* dag, uint32_t cluster_idx, cluster_visitor_t visitor, void* user_data) {
    if (!dag || cluster_idx >= dag->count || !visitor) return;

    const cluster_t* cluster = &dag->clusters[cluster_idx];
    visitor(cluster, user_data);

    for (int i = 0; i < 8; ++i) {
        uint32_t child_idx = cluster->child_clusters[i];
        if (child_idx != 0xFFFFFFFF) {
            cluster_dag_traverse_depth_first(dag, child_idx, visitor, user_data);
        }
    }
}

bool cluster_dag_serialize(const cluster_result_t* dag, const char* filename) {
    if (!dag || !filename) return false;

    FILE* f = fopen(filename, "wb");
    if (!f) return false;

    fwrite(&dag->count, sizeof(uint32_t), 1, f);
    fwrite(&dag->root_cluster, sizeof(uint32_t), 1, f);
    fwrite(dag->clusters, sizeof(cluster_t), dag->count, f);

    fclose(f);
    return true;
}

cluster_result_t* cluster_dag_deserialize(const char* filename) {
    if (!filename) return NULL;

    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;

    cluster_result_t* dag = (cluster_result_t*)malloc(sizeof(cluster_result_t));
    if (!dag) { fclose(f); return NULL; }

    fread(&dag->count, sizeof(uint32_t), 1, f);
    fread(&dag->root_cluster, sizeof(uint32_t), 1, f);
    
    dag->clusters = (cluster_t*)malloc(dag->count * sizeof(cluster_t));
    if (!dag->clusters) { free(dag); fclose(f); return NULL; }

    fread(dag->clusters, sizeof(cluster_t), dag->count, f);

    fclose(f);
    return dag;
}

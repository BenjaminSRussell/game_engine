/*
 * cluster_dag.h
 * Cluster DAG data structure and traversal
 */

#ifndef CLUSTER_DAG_H
#define CLUSTER_DAG_H

#include "geometry/nanite/nanite/cluster/cluster_builder.h"

// Traversal callback
typedef void (*cluster_visitor_t)(const cluster_t* cluster, void* user_data);

// DAG operations
void cluster_dag_traverse_depth_first(const cluster_result_t* dag, uint32_t cluster_idx, cluster_visitor_t visitor, void* user_data);

// Serialization
bool cluster_dag_serialize(const cluster_result_t* dag, const char* filename);
cluster_result_t* cluster_dag_deserialize(const char* filename);

#endif // CLUSTER_DAG_H

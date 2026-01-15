/*
 * cluster_bounds.h
 * Computation of bounding volumes for Nanite clusters
 */

#ifndef CLUSTER_BOUNDS_H
#define CLUSTER_BOUNDS_H

#include "geometry/nanite/nanite/cluster/cluster_builder.h"

// Computes the bounding sphere for a cluster based on its children's bounds
void compute_cluster_bounds(cluster_t* cluster, const cluster_t* all_clusters);

#endif // CLUSTER_BOUNDS_H

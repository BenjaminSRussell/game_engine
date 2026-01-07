/*
 * cluster_lod.h
 * Cluster LOD selection and error metric computation
 */

#ifndef CLUSTER_LOD_H
#define CLUSTER_LOD_H

#include "cluster_builder.h"

// Computes the projected screen-space error for a cluster
float compute_cluster_projected_error(const cluster_t* c, vec3_t view_pos, float view_fov, float screen_height);

// Selection status
typedef enum cluster_lod_status {
    CLUSTER_LOD_SELECT,    // Select this cluster
    CLUSTER_LOD_REFINE,    // Refine to children
    CLUSTER_LOD_COARSEN    // Too detailed, use parent
} cluster_lod_status_t;

cluster_lod_status_t evaluate_cluster_lod(const cluster_t* c, float threshold, vec3_t view_pos, float view_fov, float screen_height);

#endif // CLUSTER_LOD_H

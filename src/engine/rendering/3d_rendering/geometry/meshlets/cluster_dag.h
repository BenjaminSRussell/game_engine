/*
 * cluster_dag.h
 * Cluster DAG construction
 *
 * Part of the Geometry subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef GEOMETRY_CLUSTER_DAG_H
#define GEOMETRY_CLUSTER_DAG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct geometry_cluster_dag_handle {
    uint32_t id;
} geometry_cluster_dag_handle_t;

typedef struct geometry_cluster_dag_desc {
    uint32_t flags;
    void* user_data;
} geometry_cluster_dag_desc_t;

typedef struct geometry_cluster_dag_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} geometry_cluster_dag_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int geometry_cluster_dag_init(void);
void geometry_cluster_dag_shutdown(void);

/* Lifecycle */
int geometry_cluster_dag_create(geometry_cluster_dag_handle_t* out_handle, const geometry_cluster_dag_desc_t* desc);
void geometry_cluster_dag_destroy(geometry_cluster_dag_handle_t handle);

/* Operations */
int geometry_cluster_dag_update(geometry_cluster_dag_handle_t handle, const void* data, size_t size);
bool geometry_cluster_dag_is_valid(geometry_cluster_dag_handle_t handle);
int geometry_cluster_dag_get_info(geometry_cluster_dag_handle_t handle, geometry_cluster_dag_info_t* out_info);
void geometry_cluster_dag_mark_dirty(geometry_cluster_dag_handle_t handle);
int geometry_cluster_dag_process_pending(void);

/* Statistics */
uint32_t geometry_cluster_dag_get_count(void);
size_t geometry_cluster_dag_get_memory_usage(void);
void geometry_cluster_dag_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* GEOMETRY_CLUSTER_DAG_H */

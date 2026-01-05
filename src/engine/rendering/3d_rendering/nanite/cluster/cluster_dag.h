/*
 * cluster_dag.h
 * Cluster DAG construction
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_CLUSTER_DAG_H
#define NANITE_CLUSTER_DAG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_cluster_dag_handle {
    uint32_t id;
} nanite_cluster_dag_handle_t;

typedef struct nanite_cluster_dag_desc {
    uint32_t flags;
    void* user_data;
} nanite_cluster_dag_desc_t;

typedef struct nanite_cluster_dag_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_cluster_dag_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_cluster_dag_init(void);
void nanite_cluster_dag_shutdown(void);

/* Lifecycle */
int nanite_cluster_dag_create(nanite_cluster_dag_handle_t* out_handle, const nanite_cluster_dag_desc_t* desc);
void nanite_cluster_dag_destroy(nanite_cluster_dag_handle_t handle);

/* Operations */
int nanite_cluster_dag_update(nanite_cluster_dag_handle_t handle, const void* data, size_t size);
bool nanite_cluster_dag_is_valid(nanite_cluster_dag_handle_t handle);
int nanite_cluster_dag_get_info(nanite_cluster_dag_handle_t handle, nanite_cluster_dag_info_t* out_info);
void nanite_cluster_dag_mark_dirty(nanite_cluster_dag_handle_t handle);
int nanite_cluster_dag_process_pending(void);

/* Statistics */
uint32_t nanite_cluster_dag_get_count(void);
size_t nanite_cluster_dag_get_memory_usage(void);
void nanite_cluster_dag_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_CLUSTER_DAG_H */

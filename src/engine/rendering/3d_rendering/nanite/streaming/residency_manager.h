/*
 * residency_manager.h
 * Memory residency
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_RESIDENCY_MANAGER_H
#define NANITE_RESIDENCY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_residency_manager_handle {
    uint32_t id;
} nanite_residency_manager_handle_t;

typedef struct nanite_residency_manager_desc {
    uint32_t flags;
    void* user_data;
} nanite_residency_manager_desc_t;

typedef struct nanite_residency_manager_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_residency_manager_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_residency_manager_init(void);
void nanite_residency_manager_shutdown(void);

/* Lifecycle */
int nanite_residency_manager_create(nanite_residency_manager_handle_t* out_handle, const nanite_residency_manager_desc_t* desc);
void nanite_residency_manager_destroy(nanite_residency_manager_handle_t handle);

/* Operations */
int nanite_residency_manager_update(nanite_residency_manager_handle_t handle, const void* data, size_t size);
bool nanite_residency_manager_is_valid(nanite_residency_manager_handle_t handle);
int nanite_residency_manager_get_info(nanite_residency_manager_handle_t handle, nanite_residency_manager_info_t* out_info);
void nanite_residency_manager_mark_dirty(nanite_residency_manager_handle_t handle);
int nanite_residency_manager_process_pending(void);

/* Statistics */
uint32_t nanite_residency_manager_get_count(void);
size_t nanite_residency_manager_get_memory_usage(void);
void nanite_residency_manager_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_RESIDENCY_MANAGER_H */

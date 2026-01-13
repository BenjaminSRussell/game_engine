/*
 * budget_tracker.h
 * Memory budget monitoring
 *
 * Part of the Core subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef CORE_BUDGET_TRACKER_H
#define CORE_BUDGET_TRACKER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef enum CoreBudgetBackend {
    CORE_BUDGET_BACKEND_GENERIC = 0,
    CORE_BUDGET_BACKEND_VULKAN,
    CORE_BUDGET_BACKEND_METAL,
    CORE_BUDGET_BACKEND_D3D12,
    CORE_BUDGET_BACKEND_COUNT
} CoreBudgetBackend;

typedef struct core_budget_tracker_handle {
    uint32_t id;
} core_budget_tracker_handle_t;

typedef struct core_budget_tracker_desc {
    uint32_t flags;
    CoreBudgetBackend backend;
    size_t limit; /* 0 for unlimited */
    void* user_data;
} core_budget_tracker_desc_t;

typedef struct core_budget_tracker_info {
    uint32_t id;
    uint32_t flags;
    CoreBudgetBackend backend;
    size_t current_usage;
    size_t peak_usage;
    size_t limit;
    bool initialized;
} core_budget_tracker_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int core_budget_tracker_init(void);
void core_budget_tracker_shutdown(void);

/* Lifecycle */
int core_budget_tracker_create(core_budget_tracker_handle_t* out_handle, const core_budget_tracker_desc_t* desc);
void core_budget_tracker_destroy(core_budget_tracker_handle_t handle);

/* Operations */
/* Updates the current usage. data pointer is optional (can be NULL) if only tracking size. */
int core_budget_tracker_update(core_budget_tracker_handle_t handle, const void* data, size_t size);
bool core_budget_tracker_is_valid(core_budget_tracker_handle_t handle);
int core_budget_tracker_get_info(core_budget_tracker_handle_t handle, core_budget_tracker_info_t* out_info);
void core_budget_tracker_mark_dirty(core_budget_tracker_handle_t handle);
int core_budget_tracker_process_pending(void);

/* Statistics */
uint32_t core_budget_tracker_get_count(void);
size_t core_budget_tracker_get_memory_usage(void);
void core_budget_tracker_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BUDGET_TRACKER_H */

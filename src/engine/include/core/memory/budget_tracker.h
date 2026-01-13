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
 * ERROR CODES
 * ============================================================================ */

typedef enum core_budget_error {
    CORE_BUDGET_SUCCESS = 0,
    CORE_BUDGET_ERROR_NOT_INITIALIZED = -1,
    CORE_BUDGET_ERROR_INVALID_ARGS = -2,
    CORE_BUDGET_ERROR_OUT_OF_MEMORY = -3,
    CORE_BUDGET_ERROR_LIMIT_EXCEEDED = -4,
    CORE_BUDGET_ERROR_ALREADY_EXISTS = -5,
    CORE_BUDGET_ERROR_NOT_FOUND = -6,
    CORE_BUDGET_ERROR_INTERNAL = -7
} core_budget_error_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct core_budget_tracker_handle {
    uint32_t id;
} core_budget_tracker_handle_t;

typedef struct core_budget_tracker_desc {
    uint32_t flags;
    size_t limit;       /* 0 for unlimited */
    const char* name;   /* Name for debugging */
    void* user_data;
} core_budget_tracker_desc_t;

typedef struct core_budget_tracker_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
    char name[64];
} core_budget_tracker_info_t;

typedef struct core_budget_tracker_status {
    size_t current_usage;
    size_t peak_usage;
    size_t limit;
    uint32_t allocation_count;
    char name[64];
} core_budget_tracker_status_t;

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
/* Record an allocation of 'size' bytes. Returns error if limit exceeded. */
int core_budget_tracker_allocate(core_budget_tracker_handle_t handle, size_t size);
/* Record a deallocation of 'size' bytes. */
int core_budget_tracker_deallocate(core_budget_tracker_handle_t handle, size_t size);

/* Legacy/Generic Update - Sets associated data (does not affect budget usage directly unless implemented so) */
int core_budget_tracker_update(core_budget_tracker_handle_t handle, const void* data, size_t size);

bool core_budget_tracker_is_valid(core_budget_tracker_handle_t handle);
int core_budget_tracker_get_info(core_budget_tracker_handle_t handle, core_budget_tracker_info_t* out_info);
int core_budget_tracker_get_status(core_budget_tracker_handle_t handle, core_budget_tracker_status_t* out_status);

void core_budget_tracker_mark_dirty(core_budget_tracker_handle_t handle);
int core_budget_tracker_process_pending(void);

/* Statistics */
uint32_t core_budget_tracker_get_count(void);
/* Returns total memory tracked across all trackers */
size_t core_budget_tracker_get_memory_usage(void);
void core_budget_tracker_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BUDGET_TRACKER_H */

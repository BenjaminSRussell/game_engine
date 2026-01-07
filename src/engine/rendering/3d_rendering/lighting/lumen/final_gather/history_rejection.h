/*
 * history_rejection.h
 * History rejection
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_HISTORY_REJECTION_H
#define LUMEN_HISTORY_REJECTION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_history_rejection_handle {
    uint32_t id;
} lumen_history_rejection_handle_t;

typedef struct lumen_history_rejection_desc {
    uint32_t flags;
    void* user_data;
} lumen_history_rejection_desc_t;

typedef struct lumen_history_rejection_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_history_rejection_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_history_rejection_init(void);
void lumen_history_rejection_shutdown(void);

/* Lifecycle */
int lumen_history_rejection_create(lumen_history_rejection_handle_t* out_handle, const lumen_history_rejection_desc_t* desc);
void lumen_history_rejection_destroy(lumen_history_rejection_handle_t handle);

/* Operations */
int lumen_history_rejection_update(lumen_history_rejection_handle_t handle, const void* data, size_t size);
bool lumen_history_rejection_is_valid(lumen_history_rejection_handle_t handle);
int lumen_history_rejection_get_info(lumen_history_rejection_handle_t handle, lumen_history_rejection_info_t* out_info);
void lumen_history_rejection_mark_dirty(lumen_history_rejection_handle_t handle);
int lumen_history_rejection_process_pending(void);

/* Statistics */
uint32_t lumen_history_rejection_get_count(void);
size_t lumen_history_rejection_get_memory_usage(void);
void lumen_history_rejection_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_HISTORY_REJECTION_H */

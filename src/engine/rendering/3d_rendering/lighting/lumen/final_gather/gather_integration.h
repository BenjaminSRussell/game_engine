/*
 * gather_integration.h
 * Gather integration
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_GATHER_INTEGRATION_H
#define LUMEN_GATHER_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_gather_integration_handle {
    uint32_t id;
} lumen_gather_integration_handle_t;

typedef struct lumen_gather_integration_desc {
    uint32_t flags;
    void* user_data;
} lumen_gather_integration_desc_t;

typedef struct lumen_gather_integration_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_gather_integration_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_gather_integration_init(void);
void lumen_gather_integration_shutdown(void);

/* Lifecycle */
int lumen_gather_integration_create(lumen_gather_integration_handle_t* out_handle, const lumen_gather_integration_desc_t* desc);
void lumen_gather_integration_destroy(lumen_gather_integration_handle_t handle);

/* Operations */
int lumen_gather_integration_update(lumen_gather_integration_handle_t handle, const void* data, size_t size);
bool lumen_gather_integration_is_valid(lumen_gather_integration_handle_t handle);
int lumen_gather_integration_get_info(lumen_gather_integration_handle_t handle, lumen_gather_integration_info_t* out_info);
void lumen_gather_integration_mark_dirty(lumen_gather_integration_handle_t handle);
int lumen_gather_integration_process_pending(void);

/* Statistics */
uint32_t lumen_gather_integration_get_count(void);
size_t lumen_gather_integration_get_memory_usage(void);
void lumen_gather_integration_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_GATHER_INTEGRATION_H */

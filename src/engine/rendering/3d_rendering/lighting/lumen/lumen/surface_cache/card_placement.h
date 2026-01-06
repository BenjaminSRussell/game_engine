/*
 * card_placement.h
 * Card placement strategy
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_CARD_PLACEMENT_H
#define LUMEN_CARD_PLACEMENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_card_placement_handle {
    uint32_t id;
} lumen_card_placement_handle_t;

typedef struct lumen_card_placement_desc {
    uint32_t flags;
    void* user_data;
} lumen_card_placement_desc_t;

typedef struct lumen_card_placement_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_card_placement_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_card_placement_init(void);
void lumen_card_placement_shutdown(void);

/* Lifecycle */
int lumen_card_placement_create(lumen_card_placement_handle_t* out_handle, const lumen_card_placement_desc_t* desc);
void lumen_card_placement_destroy(lumen_card_placement_handle_t handle);

/* Operations */
int lumen_card_placement_update(lumen_card_placement_handle_t handle, const void* data, size_t size);
bool lumen_card_placement_is_valid(lumen_card_placement_handle_t handle);
int lumen_card_placement_get_info(lumen_card_placement_handle_t handle, lumen_card_placement_info_t* out_info);
void lumen_card_placement_mark_dirty(lumen_card_placement_handle_t handle);
int lumen_card_placement_process_pending(void);

/* Statistics */
uint32_t lumen_card_placement_get_count(void);
size_t lumen_card_placement_get_memory_usage(void);
void lumen_card_placement_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_CARD_PLACEMENT_H */

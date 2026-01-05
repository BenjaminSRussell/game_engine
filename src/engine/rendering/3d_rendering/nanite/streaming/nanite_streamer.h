/*
 * nanite_streamer.h
 * Nanite data streaming
 *
 * Part of the Nanite subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef NANITE_NANITE_STREAMER_H
#define NANITE_NANITE_STREAMER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct nanite_nanite_streamer_handle {
    uint32_t id;
} nanite_nanite_streamer_handle_t;

typedef struct nanite_nanite_streamer_desc {
    uint32_t flags;
    void* user_data;
} nanite_nanite_streamer_desc_t;

typedef struct nanite_nanite_streamer_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} nanite_nanite_streamer_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int nanite_nanite_streamer_init(void);
void nanite_nanite_streamer_shutdown(void);

/* Lifecycle */
int nanite_nanite_streamer_create(nanite_nanite_streamer_handle_t* out_handle, const nanite_nanite_streamer_desc_t* desc);
void nanite_nanite_streamer_destroy(nanite_nanite_streamer_handle_t handle);

/* Operations */
int nanite_nanite_streamer_update(nanite_nanite_streamer_handle_t handle, const void* data, size_t size);
bool nanite_nanite_streamer_is_valid(nanite_nanite_streamer_handle_t handle);
int nanite_nanite_streamer_get_info(nanite_nanite_streamer_handle_t handle, nanite_nanite_streamer_info_t* out_info);
void nanite_nanite_streamer_mark_dirty(nanite_nanite_streamer_handle_t handle);
int nanite_nanite_streamer_process_pending(void);

/* Statistics */
uint32_t nanite_nanite_streamer_get_count(void);
size_t nanite_nanite_streamer_get_memory_usage(void);
void nanite_nanite_streamer_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* NANITE_NANITE_STREAMER_H */

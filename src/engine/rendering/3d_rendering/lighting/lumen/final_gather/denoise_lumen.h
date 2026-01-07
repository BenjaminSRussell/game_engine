/*
 * denoise_lumen.h
 * Lumen denoising
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_DENOISE_LUMEN_H
#define LUMEN_DENOISE_LUMEN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_denoise_lumen_handle {
    uint32_t id;
} lumen_denoise_lumen_handle_t;

typedef struct lumen_denoise_lumen_desc {
    uint32_t flags;
    void* user_data;
} lumen_denoise_lumen_desc_t;

typedef struct lumen_denoise_lumen_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} lumen_denoise_lumen_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_denoise_lumen_init(void);
void lumen_denoise_lumen_shutdown(void);

/* Lifecycle */
int lumen_denoise_lumen_create(lumen_denoise_lumen_handle_t* out_handle, const lumen_denoise_lumen_desc_t* desc);
void lumen_denoise_lumen_destroy(lumen_denoise_lumen_handle_t handle);

/* Operations */
int lumen_denoise_lumen_update(lumen_denoise_lumen_handle_t handle, const void* data, size_t size);
bool lumen_denoise_lumen_is_valid(lumen_denoise_lumen_handle_t handle);
int lumen_denoise_lumen_get_info(lumen_denoise_lumen_handle_t handle, lumen_denoise_lumen_info_t* out_info);
void lumen_denoise_lumen_mark_dirty(lumen_denoise_lumen_handle_t handle);
int lumen_denoise_lumen_process_pending(void);

/* Statistics */
uint32_t lumen_denoise_lumen_get_count(void);
size_t lumen_denoise_lumen_get_memory_usage(void);
void lumen_denoise_lumen_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_DENOISE_LUMEN_H */

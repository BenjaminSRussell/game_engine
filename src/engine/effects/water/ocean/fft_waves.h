/*
 * fft_waves.h
 * FFT wave simulation
 *
 * Part of the Water subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef WATER_FFT_WAVES_H
#define WATER_FFT_WAVES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "engine/include/math/math_all.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct water_fft_waves_handle {
    uint32_t id;
} water_fft_waves_handle_t;

typedef struct water_fft_waves_desc {
    uint32_t flags;
    void* user_data;
} water_fft_waves_desc_t;

typedef struct water_fft_waves_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} water_fft_waves_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int water_fft_waves_init(void);
void water_fft_waves_shutdown(void);

/* Lifecycle */
int water_fft_waves_create(water_fft_waves_handle_t* out_handle, const water_fft_waves_desc_t* desc);
void water_fft_waves_destroy(water_fft_waves_handle_t handle);

/* Operations */
int water_fft_waves_update(water_fft_waves_handle_t handle, float time);
bool water_fft_waves_is_valid(water_fft_waves_handle_t handle);
int water_fft_waves_get_info(water_fft_waves_handle_t handle, water_fft_waves_info_t* out_info);
void water_fft_waves_mark_dirty(water_fft_waves_handle_t handle);
int water_fft_waves_process_pending(void);
void water_fft_waves_get_resources(water_fft_waves_handle_t handle, void** out_displacement, void** out_derivatives);

/* Statistics */
uint32_t water_fft_waves_get_count(void);
size_t water_fft_waves_get_memory_usage(void);
void water_fft_waves_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* WATER_FFT_WAVES_H */

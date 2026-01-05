/*
 * car_paint.h
 * Car paint model
 *
 * Part of the Shading subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef SHADING_CAR_PAINT_H
#define SHADING_CAR_PAINT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct shading_car_paint_handle {
    uint32_t id;
} shading_car_paint_handle_t;

typedef struct shading_car_paint_desc {
    uint32_t flags;
    void* user_data;
} shading_car_paint_desc_t;

typedef struct shading_car_paint_info {
    uint32_t id;
    uint32_t flags;
    bool initialized;
} shading_car_paint_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int shading_car_paint_init(void);
void shading_car_paint_shutdown(void);

/* Lifecycle */
int shading_car_paint_create(shading_car_paint_handle_t* out_handle, const shading_car_paint_desc_t* desc);
void shading_car_paint_destroy(shading_car_paint_handle_t handle);

/* Operations */
int shading_car_paint_update(shading_car_paint_handle_t handle, const void* data, size_t size);
bool shading_car_paint_is_valid(shading_car_paint_handle_t handle);
int shading_car_paint_get_info(shading_car_paint_handle_t handle, shading_car_paint_info_t* out_info);
void shading_car_paint_mark_dirty(shading_car_paint_handle_t handle);
int shading_car_paint_process_pending(void);

/* Statistics */
uint32_t shading_car_paint_get_count(void);
size_t shading_car_paint_get_memory_usage(void);
void shading_car_paint_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* SHADING_CAR_PAINT_H */

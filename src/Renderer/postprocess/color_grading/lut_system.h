/*
 * lut_system.h
 * 3D LUT (Look-Up Table) color grading system
 * 
 * Supports loading, blending, and applying 3D LUTs for color grading.
 * Typical LUT sizes are 32x32x32 or 64x64x64.
 * 
 * Part of the Post-Processing system
 * Advanced 3D Rendering Engine
 */

#ifndef LUT_SYSTEM_H
#define LUT_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define LUT_SIZE_SMALL 16
#define LUT_SIZE_MEDIUM 32
#define LUT_SIZE_LARGE 64
#define LUT_DEFAULT_SIZE LUT_SIZE_MEDIUM

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lut_3d {
    float* data;                // RGB data (size^3 * 3 floats)
    uint32_t size;              // Size per dimension (16, 32, or 64)
    char name[64];              // LUT name
} lut_3d_t;

typedef struct lut_system lut_system_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lut_system_init(void);
void lut_system_shutdown(void);

/* Lifecycle */
lut_system_t* lut_system_create(void);
void lut_system_destroy(lut_system_t* system);

/* LUT Management */
lut_3d_t* lut_3d_create_identity(uint32_t size);
lut_3d_t* lut_3d_create_from_file(const char* filepath);
lut_3d_t* lut_3d_create_from_data(const float* data, uint32_t size);
void lut_3d_destroy(lut_3d_t* lut);

/* LUT Operations */
int lut_3d_save_to_file(const lut_3d_t* lut, const char* filepath);
lut_3d_t* lut_3d_blend(const lut_3d_t* lut_a, const lut_3d_t* lut_b, float blend_factor);

/* Color Application */
void lut_3d_apply_color(const lut_3d_t* lut, float r, float g, float b, 
                       float* out_r, float* out_g, float* out_b);
void lut_3d_apply_buffer(const lut_3d_t* lut, const float* input_rgb, 
                        float* output_rgb, uint32_t pixel_count);

/* System-level LUT Management */
int lut_system_load_lut(lut_system_t* system, const char* name, const char* filepath);
int lut_system_set_active_lut(lut_system_t* system, const char* name);
const lut_3d_t* lut_system_get_active_lut(const lut_system_t* system);
void lut_system_clear_luts(lut_system_t* system);

/* LUT Blending */
void lut_system_blend_to_lut(lut_system_t* system, const char* target_name, 
                            float blend_duration_seconds);
float lut_system_update_blend(lut_system_t* system, float delta_time);

/* Statistics */
uint32_t lut_system_get_lut_count(const lut_system_t* system);
size_t lut_system_get_memory_usage(const lut_system_t* system);

#ifdef __cplusplus
}
#endif

#endif /* LUT_SYSTEM_H */

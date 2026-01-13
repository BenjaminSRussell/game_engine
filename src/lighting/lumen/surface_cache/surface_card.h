/*
 * surface_card.h
 * Surface card generation
 *
 * Part of the Lumen subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LUMEN_SURFACE_CARD_H
#define LUMEN_SURFACE_CARD_H

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <xmmintrin.h>
#elif defined(__arm64__) || defined(__aarch64__)
#include <arm_neon.h>
typedef float32x4_t __m128;
#else
typedef float __m128[4];
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <include/math/math.h>
#include "lighting/lumen/surface_cache/surface_atlas.h"
#include "lighting/lumen/tracing_lumen/trace_cache.h"
#include "include/math/mat4.h"

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct lumen_surface_card {
    vec3_t position;
    vec3_t normal;
    vec3_t extent;
    lumen_surface_atlas_region_t atlas_region;
    uint32_t mesh_id;
    bool active;
} lumen_surface_card_t;

typedef struct lumen_surface_card_handle {
    uint32_t id;
} lumen_surface_card_handle_t;

typedef struct lumen_surface_card_desc {
    vec3_t position;
    vec3_t normal;
    vec3_t extent;
    uint32_t mesh_id;
    uint32_t flags;
} lumen_surface_card_desc_t;

typedef struct lumen_surface_card_info {
    uint32_t id;
    vec3_t position;
    bool active;
    bool initialized;
} lumen_surface_card_info_t;

/* ============================================================================
 * API
 * ============================================================================ */

/* Initialization */
int lumen_surface_card_init(void);
void lumen_surface_card_shutdown(void);

/* Lifecycle */
int lumen_surface_card_create(lumen_surface_card_handle_t* out_handle, const lumen_surface_card_desc_t* desc);
void lumen_surface_card_destroy(lumen_surface_card_handle_t handle);

/* Operations */
int lumen_surface_card_generate_for_mesh(uint32_t mesh_id, lumen_surface_card_handle_t* out_handles, uint32_t* out_count);
int lumen_surface_card_update(lumen_surface_card_handle_t handle, const void* data, size_t size);
bool lumen_surface_card_is_valid(lumen_surface_card_handle_t handle);
int lumen_surface_card_get_info(lumen_surface_card_handle_t handle, lumen_surface_card_info_t* out_info);
void lumen_surface_card_mark_dirty(lumen_surface_card_handle_t handle);
int lumen_surface_card_process_pending(void);

/* Statistics */
uint32_t lumen_surface_card_get_count(void);
size_t lumen_surface_card_get_memory_usage(void);
void lumen_surface_card_debug_print(void);

#ifdef __cplusplus
}
#endif

#endif /* LUMEN_SURFACE_CARD_H */

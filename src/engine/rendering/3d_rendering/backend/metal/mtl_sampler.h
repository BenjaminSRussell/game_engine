/*
 * mtl_sampler.h
 * Metal sampler states
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_SAMPLER_H
#define PLATFORM_MTL_SAMPLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

typedef struct metal_device metal_device_t;

/* ============================================================================
 * ENUMS
 * ============================================================================ */

typedef enum metal_filter_mode {
    METAL_FILTER_NEAREST = 0,
    METAL_FILTER_LINEAR = 1
} metal_filter_mode_t;

typedef enum metal_address_mode {
    METAL_ADDRESS_CLAMP_TO_EDGE = 0,
    METAL_ADDRESS_REPEAT = 1,
    METAL_ADDRESS_MIRROR_REPEAT = 2,
    METAL_ADDRESS_CLAMP_TO_ZERO = 3,
    METAL_ADDRESS_CLAMP_TO_BORDER = 4
} metal_address_mode_t;

typedef enum metal_compare_function {
    METAL_COMPARE_NEVER = 0,
    METAL_COMPARE_LESS = 1,
    METAL_COMPARE_EQUAL = 2,
    METAL_COMPARE_LESS_EQUAL = 3,
    METAL_COMPARE_GREATER = 4,
    METAL_COMPARE_NOT_EQUAL = 5,
    METAL_COMPARE_GREATER_EQUAL = 6,
    METAL_COMPARE_ALWAYS = 7
} metal_compare_function_t;

typedef enum metal_border_color {
    METAL_BORDER_COLOR_TRANSPARENT_BLACK = 0,
    METAL_BORDER_COLOR_OPAQUE_BLACK = 1,
    METAL_BORDER_COLOR_OPAQUE_WHITE = 2
} metal_border_color_t;

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct metal_sampler {
#ifdef __OBJC__
    id<MTLSamplerState> sampler;
#else
    void* sampler;
#endif
    uint32_t hash;  // For deduplication
    char label[64];
} metal_sampler_t;

typedef struct metal_sampler_desc {
    metal_filter_mode_t min_filter;
    metal_filter_mode_t mag_filter;
    metal_filter_mode_t mip_filter;
    
    metal_address_mode_t address_mode_u;
    metal_address_mode_t address_mode_v;
    metal_address_mode_t address_mode_w;
    
    float min_lod;
    float max_lod;
    float lod_bias;
    
    uint32_t max_anisotropy;  // 1 = no aniso, max typically 16
    
    metal_compare_function_t compare_function;
    bool compare_enabled;  // For shadow sampling
    
    metal_border_color_t border_color;
    
    bool normalized_coordinates;  // true = [0,1], false = [0, size]
    
    const char* label;
} metal_sampler_desc_t;

typedef struct metal_sampler_cache_stats {
    uint32_t total_samplers;
    uint32_t cache_hits;
    uint32_t cache_misses;
    size_t memory_used;
} metal_sampler_cache_stats_t;

/* ============================================================================
 * API - Sampler Lifecycle
 * ============================================================================ */

/* Create sampler (with automatic caching/deduplication) */
metal_sampler_t* metal_sampler_create(metal_device_t* device, const metal_sampler_desc_t* desc);

/* Destroy sampler (decrements ref count, may not actually destroy if cached) */
void metal_sampler_destroy(metal_sampler_t* sampler);

/* ============================================================================
 * API - Predefined Samplers
 * ============================================================================ */

/* Point sampling, clamped */
metal_sampler_t* metal_sampler_create_point_clamp(metal_device_t* device);

/* Linear filtering, clamped */
metal_sampler_t* metal_sampler_create_linear_clamp(metal_device_t* device);

/* Linear filtering, repeating */
metal_sampler_t* metal_sampler_create_linear_repeat(metal_device_t* device);

/* Anisotropic filtering */
metal_sampler_t* metal_sampler_create_anisotropic(metal_device_t* device, uint32_t max_aniso);

/* Shadow sampler (comparison enabled) */
metal_sampler_t* metal_sampler_create_shadow(metal_device_t* device);

/* ============================================================================
 * API - Sampler Cache
 * ============================================================================ */

/* Clear sampler cache (destroys all cached samplers) */
void metal_sampler_cache_clear(void);

/* Get cache statistics */
metal_sampler_cache_stats_t metal_sampler_cache_get_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_SAMPLER_H */

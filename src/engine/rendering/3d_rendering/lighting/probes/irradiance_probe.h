/*
 * irradiance_probe.h
 * Irradiance probe sampling
 *
 * Part of the Lighting subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef LIGHTING_IRRADIANCE_PROBE_H
#define LIGHTING_IRRADIANCE_PROBE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <simd/simd.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

/* Forward declarations */
typedef struct metal_device metal_device_t;
// Handle likely already defined in backend but redefining opaque here for safety if headers not included
#ifndef METAL_DEVICE_DEFINED
typedef struct metal_device metal_device_t;
#endif

typedef struct irradiance_probe {
    simd_float3 position;
    simd_float4 sh_coefficients[9];  // L0, L1, L2 spherical harmonics (9 coefficients)
    float influence_radius;
    float padding[3]; // Align to 16 bytes if needed, though this struct is large
} irradiance_probe_t;

typedef struct probe_grid {
    irradiance_probe_t* probes;
    uint32_t probe_count;

    simd_float3 bounds_min;
    simd_float3 bounds_max;
    simd_uint3 resolution;

#ifdef __OBJC__
    id<MTLBuffer> probe_buffer;
    id<MTLTexture> probe_texture;  // 3D texture for GPU sampling (SH coefficients)
#else
    void* probe_buffer;
    void* probe_texture;
#endif
} probe_grid_t;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Creates a new probe grid.
 * @param dev The metal device to use for resource creation.
 * @param min The minimum bounds of the grid.
 * @param max The maximum bounds of the grid.
 * @param resolution The number of probes in each dimension (x, y, z).
 * @return A pointer to the created probe grid, or NULL on failure.
 */
probe_grid_t* probe_grid_create(metal_device_t* dev, simd_float3 min, simd_float3 max, simd_uint3 resolution);

/**
 * Destroys a probe grid and releases its resources.
 * @param grid The grid to destroy.
 */
void probe_grid_destroy(probe_grid_t* grid);

/**
 * Updates the probe grid resources on the GPU.
 * Call this when probe data (SH coefficients) changes.
 * @param grid The grid to update.
 */
void probe_grid_update_gpu(probe_grid_t* grid);

#ifdef __cplusplus
}
#endif

#endif /* LIGHTING_IRRADIANCE_PROBE_H */

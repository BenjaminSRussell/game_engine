/*
 * deferred_lighting.h
 * Deferred lighting pass API
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef DEFERRED_LIGHTING_H
#define DEFERRED_LIGHTING_H

#include <stdint.h>
#include <stdbool.h>
#include <simd/simd.h>
#include <Metal/Metal.h>

// Forward declarations
struct Camera;
struct gbuffer; // Assuming gbuffer_t is struct gbuffer based on context, or use void* if unknown
typedef struct gbuffer gbuffer_t; // We might need to include gbuffer_layout.h or forward declare

/**
 * Light data structure matching the Metal shader
 */
typedef struct light {
    vector_float3 position;
    float radius;
    vector_float3 color;
    float intensity;
} light_t;

/**
 * Deferred lighting pass context
 */
typedef struct deferred_lighting deferred_lighting_t;

/**
 * @brief Create a new deferred lighting pass instance
 * @param device Metal device
 * @param color_format Pixel format of the lighting accumulation buffer
 * @param depth_format Pixel format of the depth buffer (can be MTLPixelFormatInvalid if not used)
 * @return Pointer to new instance or NULL on failure
 */
deferred_lighting_t* rendering_deferred_lighting_create(id<MTLDevice> device,
                                                      MTLPixelFormat color_format,
                                                      MTLPixelFormat depth_format);

/**
 * @brief Destroy a deferred lighting pass instance
 * @param dl Instance to destroy
 */
void rendering_deferred_lighting_destroy(deferred_lighting_t* dl);

/**
 * @brief Execute the deferred lighting pass
 * @param dl Deferred lighting instance
 * @param gb G-buffer containing textures
 * @param encoder Render command encoder
 * @param lights Array of lights
 * @param light_count Number of lights
 * @param camera Camera for view/proj matrices
 */
void rendering_deferred_lighting_execute(deferred_lighting_t* dl, 
                                       gbuffer_t* gb,
                                       id<MTLRenderCommandEncoder> encoder,
                                       light_t* lights, 
                                       uint32_t light_count,
                                       struct Camera* camera);

#endif /* DEFERRED_LIGHTING_H */

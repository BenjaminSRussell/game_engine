/**
 * =================================================================================================
 *                              G-BUFFER SYSTEM
 * =================================================================================================
 *
 * Purpose: Defines the Geometric Buffer (G-Buffer) used for deferred rendering.
 * Stores surface properties like albedo, normal, material attributes, and depth.
 */

#pragma once

#include <core/types.h>

/* =================================================================================================
 *                                    TYPE DEFINITIONS
 * =================================================================================================
 */

typedef struct {
    // Render target dimensions
    u32 width;
    u32 height;

    /* G-Buffer Textures (Render Targets) */
    
    // RT0: Albedo (RGB) + Translucency (A)
    // Format: RGBA8_UNORM
    void *texture_albedo;

    // RT1: Normal (Encoded) + Motion Vectors (Optional in this channel?)
    // Format: RG16F (Octahedral encoded normal)
    void *texture_normal;

    // RT2: Material Properties
    // R: Metallic, G: Roughness, B: AO, A: Custom/Flags
    // Format: RGBA8_UNORM
    void *texture_material;

    // RT3: Emissive
    // Format: R11G11B10F or RGBA16F (HDR)
    void *texture_emissive;
    
    // RT4: Velocity (Motion Vectors)
    // Format: RG16F
    void *texture_velocity;

    // Depth Stencil Buffer
    // Format: D32F_S8 or D24_S8
    void *texture_depth;

    // Framebuffer handle encapsulating these attachments
    void *framebuffer;

} GBuffer;

/* =================================================================================================
 *                                    PUBLIC API
 * =================================================================================================
 */

/**
 * Creates a new G-Buffer with the specified dimensions.
 */
GBuffer* gbuffer_create(u32 width, u32 height);

/**
 * Destroys the G-Buffer and releases all associated resources.
 */
void gbuffer_destroy(GBuffer *gbuffer);

/**
 * Resizes the G-Buffer attachments. 
 * Should be called when the window/viewport size changes.
 */
void gbuffer_resize(GBuffer *gbuffer, u32 width, u32 height);

/**
 * Binds the G-Buffer for writing (geometry pass).
 */
void gbuffer_bind(GBuffer *gbuffer);

/**
 * Unbinds the G-Buffer.
 */
void gbuffer_unbind(GBuffer *gbuffer);

/**
 * Binds the G-Buffer textures for reading (lighting pass).
 * Usually binds them to specific texture slots.
 */
void gbuffer_bind_textures(GBuffer *gbuffer, u32 start_slot);

/*
 * oit_wboit.h
 * Weighted Blended Order-Independent Transparency (WBOIT)
 *
 * Implements technique described by McGuire and Bavoil (2013).
 * Uses two render targets: Accumulation (weighted color) and Reveal (alpha coverage).
 */

#ifndef RENDERING_FORWARD_OIT_WBOIT_H
#define RENDERING_FORWARD_OIT_WBOIT_H

#include <core/types.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct OITContext OITContext;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Creates the WBOIT context and internal buffers.
 */
OITContext* oit_wboit_create(u32 width, u32 height);

/**
 * Destroys the WBOIT context.
 */
void oit_wboit_destroy(OITContext* ctx);

/**
 * Resizes the WBOIT buffers.
 */
void oit_wboit_resize(OITContext* ctx, u32 width, u32 height);

/**
 * Clears the accumulation and reveal buffers.
 * MUST be called before rendering transparent geometry.
 * Accumulation -> vec4(0, 0, 0, 0)
 * Reveal -> 1.0 (exposed)
 */
void oit_wboit_begin_pass(OITContext* ctx);

/**
 * Composites the transparent result onto the backbuffer (or current render target).
 * Should be called after all transparent geometry is drawn.
 * Fullscreen pass that reads Accum and Reveal textures.
 */
void oit_wboit_composite(OITContext* ctx);

/**
 * Gets the Accumulation Texture (RGBA16F).
 */
void* oit_wboit_get_accum_texture(OITContext* ctx);

/**
 * Gets the Reveal Texture (R8 or R16F).
 */
void* oit_wboit_get_reveal_texture(OITContext* ctx);

#endif /* RENDERING_FORWARD_OIT_WBOIT_H */

/**
 * =================================================================================================
 *                              FRAMEBUFFER API
 * =================================================================================================
 *
 * Purpose: Provides Multiple Render Target (MRT) support for deferred rendering.
 * Allows binding multiple color attachments and a depth/stencil attachment.
 */

#pragma once

#include <core/types.h>
#include <stdbool.h>

/* =================================================================================================
 *                                    TYPE DEFINITIONS
 * =================================================================================================
 */

/**
 * Opaque framebuffer handle
 */
typedef struct Framebuffer Framebuffer;

/**
 * Maximum number of color attachments (MRTs)
 */
#define FRAMEBUFFER_MAX_COLOR_ATTACHMENTS 8

/* =================================================================================================
 *                                    PUBLIC API
 * =================================================================================================
 */

/**
 * Creates a new framebuffer with the specified dimensions.
 * 
 * @param width Framebuffer width in pixels
 * @param height Framebuffer height in pixels
 * @return Framebuffer handle, or NULL on failure
 */
Framebuffer* framebuffer_create(u32 width, u32 height);

/**
 * Destroys a framebuffer and releases all resources.
 * Does NOT destroy attached textures - caller is responsible for those.
 * 
 * @param fb Framebuffer to destroy
 */
void framebuffer_destroy(Framebuffer *fb);

/**
 * Attaches a color texture to the framebuffer at the specified slot.
 * 
 * @param fb Framebuffer handle
 * @param slot Color attachment index (0 to FRAMEBUFFER_MAX_COLOR_ATTACHMENTS-1)
 * @param texture Texture handle (must be a color format)
 */
void framebuffer_attach_color(Framebuffer *fb, u32 slot, void *texture);

/**
 * Attaches a depth/stencil texture to the framebuffer.
 * 
 * @param fb Framebuffer handle
 * @param texture Texture handle (must be a depth or depth/stencil format)
 */
void framebuffer_attach_depth(Framebuffer *fb, void *texture);

/**
 * Validates framebuffer completeness.
 * Checks that all attachments are compatible (same size, valid formats).
 * 
 * @param fb Framebuffer handle
 * @return true if framebuffer is complete and ready to use, false otherwise
 */
bool framebuffer_validate(Framebuffer *fb);

/**
 * Binds the framebuffer for rendering (sets as active render target).
 * 
 * @param fb Framebuffer handle
 */
void framebuffer_bind(Framebuffer *fb);

/**
 * Unbinds the framebuffer and restores the default framebuffer.
 */
void framebuffer_unbind(void);

/**
 * Clears all color attachments to the specified color.
 * 
 * @param fb Framebuffer handle
 * @param r Red component (0.0 - 1.0)
 * @param g Green component (0.0 - 1.0)
 * @param b Blue component (0.0 - 1.0)
 * @param a Alpha component (0.0 - 1.0)
 */
void framebuffer_clear_color(Framebuffer *fb, f32 r, f32 g, f32 b, f32 a);

/**
 * Clears the depth attachment.
 * 
 * @param fb Framebuffer handle
 * @param depth Depth value (typically 1.0 for far plane)
 */
void framebuffer_clear_depth(Framebuffer *fb, f32 depth);

/**
 * Gets the width of the framebuffer.
 * 
 * @param fb Framebuffer handle
 * @return Width in pixels
 */
u32 framebuffer_get_width(Framebuffer *fb);

/**
 * Gets the height of the framebuffer.
 * 
 * @param fb Framebuffer handle
 * @return Height in pixels
 */
u32 framebuffer_get_height(Framebuffer *fb);

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

/**
 * Framebuffer resize callback function type
 */
typedef void (*FramebufferResizeCallback)(Framebuffer *fb, u32 old_width, u32 old_height, 
                                        u32 new_width, u32 new_height, void *user_data);

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

/**
 * Resizes the framebuffer to new dimensions.
 * Automatically recreates textures if needed.
 * 
 * @param fb Framebuffer handle
 * @param width New width in pixels
 * @param height New height in pixels
 * @return true if resize was successful, false otherwise
 */
bool framebuffer_resize(Framebuffer *fb, u32 width, u32 height);

/**
 * Sets a resize callback that will be called when the framebuffer is resized.
 * 
 * @param fb Framebuffer handle
 * @param callback Callback function to call on resize
 * @param user_data User data to pass to callback
 */
void framebuffer_set_resize_callback(Framebuffer *fb, FramebufferResizeCallback callback, 
                                     void *user_data);

/**
 * Checks if the framebuffer needs to be resized (dimensions changed).
 * 
 * @param fb Framebuffer handle
 * @return true if framebuffer dimensions have changed since last resize
 */
bool framebuffer_needs_resize(Framebuffer *fb);

/**
 * Gets the aspect ratio of the framebuffer.
 * 
 * @param fb Framebuffer handle
 * @return Aspect ratio (width / height)
 */
f32 framebuffer_get_aspect_ratio(Framebuffer *fb);

/**
 * Blits (copies) framebuffer contents to another framebuffer.
 * 
 * @param src Source framebuffer
 * @param dst Destination framebuffer
 * @param filter Filtering mode (0 = nearest, 1 = linear)
 * @return true if blit was successful, false otherwise
 */
bool framebuffer_blit(Framebuffer *src, Framebuffer *dst, u32 filter);

/**
 * Reads pixel data from a color attachment.
 * 
 * @param fb Framebuffer handle
 * @param slot Color attachment index
 * @param x X coordinate to read from
 * @param y Y coordinate to read from
 * @param width Width of region to read
 * @param height Height of region to read
 * @param format Pixel format of output data
 * @param data Output buffer to store pixel data
 * @return true if read was successful, false otherwise
 */
bool framebuffer_read_pixels(Framebuffer *fb, u32 slot, u32 x, u32 y, 
                            u32 width, u32 height, u32 format, void *data);

/**
 * Gets the number of active color attachments.
 * 
 * @param fb Framebuffer handle
 * @return Number of attached color textures
 */
u32 framebuffer_get_color_attachment_count(Framebuffer *fb);

/**
 * Gets the texture handle for a specific color attachment.
 * 
 * @param fb Framebuffer handle
 * @param slot Color attachment index
 * @return Texture handle, or NULL if no attachment at that slot
 */
void* framebuffer_get_color_texture(Framebuffer *fb, u32 slot);

/**
 * Gets the depth texture handle.
 * 
 * @param fb Framebuffer handle
 * @return Depth texture handle, or NULL if no depth attachment
 */
void* framebuffer_get_depth_texture(Framebuffer *fb);

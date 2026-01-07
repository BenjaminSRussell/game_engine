/*
 * mtl_render_pass.h
 * Metal render pass optimization helpers
 *
 * Part of the Platform subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef PLATFORM_MTL_RENDER_PASS_H
#define PLATFORM_MTL_RENDER_PASS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef void* mtl_render_pass_descriptor_t; /* Maps to MTLRenderPassDescriptor */

typedef enum mtl_load_action {
    MTL_LOAD_ACTION_DONT_CARE = 0,
    MTL_LOAD_ACTION_LOAD = 1,
    MTL_LOAD_ACTION_CLEAR = 2
} mtl_load_action_t;

typedef enum mtl_store_action {
    MTL_STORE_ACTION_DONT_CARE = 0,
    MTL_STORE_ACTION_STORE = 1,
    MTL_STORE_ACTION_MULTISAMPLE_RESOLVE = 2,
    MTL_STORE_ACTION_STORE_AND_MULTISAMPLE_RESOLVE = 3,
    MTL_STORE_ACTION_UNKNOWN = 4,
    MTL_STORE_ACTION_CUSTOM_SAMPLE_POSITIONS = 5
} mtl_store_action_t;

typedef enum mtl_store_action_options {
    MTL_STORE_ACTION_OPTION_NONE = 0,
    MTL_STORE_ACTION_OPTION_CUSTOM_SAMPLE_POSITIONS = (1 << 0)
} mtl_store_action_options_t;

typedef struct mtl_clear_color {
    double red;
    double green;
    double blue;
    double alpha;
} mtl_clear_color_t;

/* ============================================================================
 * RENDER PASS DESCRIPTOR API
 * ============================================================================ */

/**
 * Creates a new render pass descriptor.
 * @return The render pass descriptor.
 */
mtl_render_pass_descriptor_t metal_render_pass_descriptor_create(void);

/**
 * Destroys a render pass descriptor.
 * @param descriptor The descriptor to destroy.
 */
void metal_render_pass_descriptor_destroy(mtl_render_pass_descriptor_t descriptor);

/* ============================================================================
 * COLOR ATTACHMENT CONFIGURATION
 * ============================================================================ */

/**
 * Sets the texture for a color attachment.
 * @param descriptor The render pass descriptor.
 * @param attachment_index The color attachment index (0-7).
 * @param texture The texture (MTLTexture).
 */
void metal_render_pass_set_color_attachment_texture(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, void* texture);

/**
 * Sets the load action for a color attachment.
 * @param descriptor The render pass descriptor.
 * @param attachment_index The color attachment index.
 * @param load_action The load action.
 */
void metal_render_pass_set_color_attachment_load_action(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, mtl_load_action_t load_action);

/**
 * Sets the store action for a color attachment.
 * @param descriptor The render pass descriptor.
 * @param attachment_index The color attachment index.
 * @param store_action The store action.
 */
void metal_render_pass_set_color_attachment_store_action(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, mtl_store_action_t store_action);

/**
 * Sets the clear color for a color attachment.
 * @param descriptor The render pass descriptor.
 * @param attachment_index The color attachment index.
 * @param clear_color The clear color.
 */
void metal_render_pass_set_color_attachment_clear_color(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, mtl_clear_color_t clear_color);

/**
 * Sets store action options for a color attachment (for optimization hints).
 * @param descriptor The render pass descriptor.
 * @param attachment_index The color attachment index.
 * @param options Store action options.
 */
void metal_render_pass_set_color_attachment_store_action_options(mtl_render_pass_descriptor_t descriptor, unsigned int attachment_index, mtl_store_action_options_t options);

/* ============================================================================
 * DEPTH/STENCIL ATTACHMENT CONFIGURATION
 * ============================================================================ */

/**
 * Sets the depth attachment texture.
 * @param descriptor The render pass descriptor.
 * @param texture The depth texture (MTLTexture).
 */
void metal_render_pass_set_depth_attachment_texture(mtl_render_pass_descriptor_t descriptor, void* texture);

/**
 * Sets the depth attachment load action.
 * @param descriptor The render pass descriptor.
 * @param load_action The load action.
 */
void metal_render_pass_set_depth_attachment_load_action(mtl_render_pass_descriptor_t descriptor, mtl_load_action_t load_action);

/**
 * Sets the depth attachment store action.
 * @param descriptor The render pass descriptor.
 * @param store_action The store action.
 */
void metal_render_pass_set_depth_attachment_store_action(mtl_render_pass_descriptor_t descriptor, mtl_store_action_t store_action);

/**
 * Sets the depth clear value.
 * @param descriptor The render pass descriptor.
 * @param clear_depth The clear depth value.
 */
void metal_render_pass_set_depth_attachment_clear_depth(mtl_render_pass_descriptor_t descriptor, double clear_depth);

/**
 * Sets the stencil attachment texture.
 * @param descriptor The render pass descriptor.
 * @param texture The stencil texture (MTLTexture).
 */
void metal_render_pass_set_stencil_attachment_texture(mtl_render_pass_descriptor_t descriptor, void* texture);

/**
 * Sets the stencil attachment load action.
 * @param descriptor The render pass descriptor.
 * @param load_action The load action.
 */
void metal_render_pass_set_stencil_attachment_load_action(mtl_render_pass_descriptor_t descriptor, mtl_load_action_t load_action);

/**
 * Sets the stencil attachment store action.
 * @param descriptor The render pass descriptor.
 * @param store_action The store action.
 */
void metal_render_pass_set_stencil_attachment_store_action(mtl_render_pass_descriptor_t descriptor, mtl_store_action_t store_action);

/**
 * Sets the stencil clear value.
 * @param descriptor The render pass descriptor.
 * @param clear_stencil The clear stencil value.
 */
void metal_render_pass_set_stencil_attachment_clear_stencil(mtl_render_pass_descriptor_t descriptor, uint32_t clear_stencil);

/* ============================================================================
 * MULTI-VIEW RENDERING
 * ============================================================================ */

/**
 * Sets the render target array length for multi-view rendering.
 * @param descriptor The render pass descriptor.
 * @param length The number of render targets (views).
 */
void metal_render_pass_set_render_target_array_length(mtl_render_pass_descriptor_t descriptor, unsigned long length);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_MTL_RENDER_PASS_H */

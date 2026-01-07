/*
 * forward_renderer.h
 * Forward Rendering System Core
 *
 * Part of the Rendering subsystem
 * Advanced 3D Rendering Engine
 */

#ifndef RENDERING_FORWARD_RENDERER_H
#define RENDERING_FORWARD_RENDERER_H

#include <core/types.h>
#include <renderer/renderer.h>
#include <renderer/camera.h>
#include <core/scene.h>

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct ForwardRenderer ForwardRenderer;

typedef struct ForwardRendererConfig {
    u32 width;
    u32 height;
    bool enable_depth_prepass;
    bool enable_transparency;
    bool enable_shadows;
} ForwardRendererConfig;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Creates a new Forward Renderer instance.
 */
ForwardRenderer* forward_renderer_create(const ForwardRendererConfig* config);

/**
 * Destroys the Forward Renderer instance.
 */
void forward_renderer_destroy(ForwardRenderer* renderer);

/**
 * Resizes the internal render targets.
 */
void forward_renderer_resize(ForwardRenderer* renderer, u32 width, u32 height);

/**
 * Prepares the renderer for a new frame.
 */
void forward_renderer_begin_frame(ForwardRenderer* renderer, const camera_t* camera);

/**
 * Submits the opaque geometry pass.
 * This can optionally perform a depth pre-pass.
 */
void forward_renderer_render_opaque(ForwardRenderer* renderer, const scene_t* scene);

/**
 * Submits the transparent geometry pass.
 * This handles sorting and alpha blending.
 */
void forward_renderer_render_transparent(ForwardRenderer* renderer, const scene_t* scene);

/**
 * Finishes the frame rendering.
 */
void forward_renderer_end_frame(ForwardRenderer* renderer);

/**
 * Gets the output texture (for compositing or display).
 */
void* forward_renderer_get_output(ForwardRenderer* renderer);

/**
 * Gets the depth buffer (can be shared with deferred renderer).
 */
void* forward_renderer_get_depth_buffer(ForwardRenderer* renderer);

#endif /* RENDERING_FORWARD_RENDERER_H */

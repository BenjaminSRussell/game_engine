/*
 * forward_batching.h
 * Forward Renderer Batching System
 *
 * Handles grouping of draw calls to minimize state changes.
 */

#ifndef RENDERING_FORWARD_BATCHING_H
#define RENDERING_FORWARD_BATCHING_H

#include <core/types.h>
#include <core/scene.h>
#include "shader_variants.h"
#include "transparency.h"

/* ============================================================================
 * TYPES
 * ============================================================================ */

typedef struct {
    u32 mesh_id;
    u32 material_id;
    ShaderVariantKey shader_key;
    BlendMode blend_mode;
    f32 distance_to_camera; // For sorting
} ForwardDrawCommand;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Initializes batching system.
 */
void forward_batching_init(void);

/**
 * Resets the batcher for a new frame.
 */
void forward_batching_begin_frame(void);

/**
 * Submits a draw command to the batcher.
 */
void forward_batching_submit(const ForwardDrawCommand* cmd);

/**
 * Sorts and groups submitted commands.
 * Opaque: Front-to-back, grouped by state.
 * Transparent: Back-to-front.
 */
void forward_batching_sort(bool is_transparent);

/**
 * Executes the batched draw calls using the provided renderer context.
 */
void forward_batching_flush(void* renderer_context);

#endif /* RENDERING_FORWARD_BATCHING_H */

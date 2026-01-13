/**
 * =================================================================================================
 *                              G-BUFFER PASS DESCRIPTOR
 * =================================================================================================
 *
 * Purpose: Configuration and state for the G-Buffer rendering pass.
 */

#pragma once

#include <core/types.h>
#include "rendering/gbuffer/gbuffer.h"

/**
 * Descriptor for a G-Buffer render pass
 */
typedef struct {
    GBuffer *gbuffer;
    
    // Clear colors for each attachment
    f32 clear_albedo[4];
    f32 clear_normal[4];
    f32 clear_material[4];
    f32 clear_emissive[4];
    f32 clear_velocity[4];
    f32 clear_depth;
    
    // Render State
    bool depth_test_enabled;
    bool depth_write_enabled;
    // ... potentially cull mode, etc.
} GBufferPassDescriptor;

/**
 * Internal state for the G-Buffer pass
 */
typedef struct GBufferPass GBufferPass;

/**
 * Creates a G-Buffer render pass based on the descriptor.
 * 
 * @param desc Configuration for the pass
 * @return Handle to the created pass
 */
GBufferPass* gbuffer_pass_create(const GBufferPassDescriptor *desc);

/**
 * Destroys a G-Buffer render pass.
 */
void gbuffer_pass_destroy(GBufferPass *pass);

/**
 * Begins the G-Buffer pass, binding the framebuffer and setting up render state.
 */
void gbuffer_pass_begin(GBufferPass *pass);

/**
 * Ends the G-Buffer pass, unbinding the framebuffer.
 */
void gbuffer_pass_end(GBufferPass *pass);

/**
 * Executes the standard G-buffer pass (convenience function).
 */
void pass_gbuffer_execute(void);

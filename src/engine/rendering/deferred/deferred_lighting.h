/**
 * =================================================================================================
 *                              DEFERRED LIGHTING SYSTEM
 * =================================================================================================
 *
 * Purpose: Manages the lighting pass of the deferred rendering pipeline.
 * Computes lighting for every pixel using G-Buffer data.
 */

#pragma once

#include <core/types.h>
#include "../gbuffer/gbuffer.h"

/* =================================================================================================
 *                                    PUBLIC API
 * =================================================================================================
 */

/**
 * Initializes the deferred lighting system resources (shaders, pipelines, etc.).
 */
void deferred_lighting_init(void);

/**
 * Shuts down the deferred lighting system.
 */
void deferred_lighting_shutdown(void);

/**
 * Executes the deferred lighting pass.
 * 
 * @param gbuffer The G-Buffer containing the scene geometry data.
 * @param output_target The render target to write final lit color to.
 */
void deferred_lighting_execute(GBuffer *gbuffer, void *output_target);

/**
 * Updates lighting configuration (e.g. environment maps, global settings).
 */
void deferred_lighting_update_settings(void *settings);

#ifndef RENDER_SYSTEMS_H
#define RENDER_SYSTEMS_H

#include "ecs/ecs.h"

/**
 * ECS Rendering Systems
 *
 * These systems process rendering components:
 * - Culling System: Frustum culling for visible meshes
 * - Render Submission System: Submit draw calls to renderer
 * - Camera System: Update camera matrices
 * - Light System: Update light data
 */

/**
 * Register all rendering systems with ECS world
 * Call after registering rendering components
 */
void register_rendering_systems(World *world);

/**
 * Culling system - marks visible/invisible meshes
 * Runs in parallel for performance
 */
void culling_system(SystemContext *ctx);

/**
 * Render submission system - submits visible meshes to renderer
 * Runs after culling
 */
void render_submission_system(SystemContext *ctx);

/**
 * Camera system - updates camera matrices
 */
void camera_system(SystemContext *ctx);

/**
 * Light system - updates light data
 */
void light_system(SystemContext *ctx);

#endif // RENDER_SYSTEMS_H

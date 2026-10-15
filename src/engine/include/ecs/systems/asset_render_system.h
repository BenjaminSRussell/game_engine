#ifndef ASSET_RENDER_SYSTEM_H
#define ASSET_RENDER_SYSTEM_H

#include <stdint.h>

// Forward declarations
typedef struct World World;
typedef struct AssetManager AssetManager;
typedef struct Asset Asset;

/**
 * AssetRenderSystem - ECS system for rendering asset instances
 * 
 * Queries entities with AssetInstanceComponent + TransformComponent
 * and submits them to the renderer.
 */

/**
 * Renderer submission callback. update() calls it once per visible asset
 * instance whose source asset is set. `transform` is the instance's world
 * matrix (T * R * S), 16 floats in column-major order.
 */
typedef void (*AssetRenderSubmitFn)(const Asset *asset, const float transform[16],
                                    uint32_t render_flags, void *user_data);

// System lifecycle
void asset_render_system_init(World *world, AssetManager *manager);
void asset_render_system_update(float dt);
void asset_render_system_shutdown();

// Registers the renderer's submission callback (NULL clears it). Shutdown clears it too.
void asset_render_system_set_submit_callback(AssetRenderSubmitFn fn, void *user_data);

#endif // ASSET_RENDER_SYSTEM_H

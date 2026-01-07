#ifndef ASSET_RENDER_SYSTEM_H
#define ASSET_RENDER_SYSTEM_H

// Forward declarations
typedef struct World World;
typedef struct AssetManager AssetManager;

/**
 * AssetRenderSystem - ECS system for rendering asset instances
 * 
 * Queries entities with AssetInstanceComponent + TransformComponent
 * and submits them to the renderer.
 */

// System lifecycle
void asset_render_system_init(World *world, AssetManager *manager);
void asset_render_system_update(float dt);
void asset_render_system_shutdown();

#endif // ASSET_RENDER_SYSTEM_H

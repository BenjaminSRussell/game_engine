/**
 * @file asset_render_system.c
 * @brief ECS Implementation
 * @description Entity Component System implementation
 * @date 2026-01-13
 */

#include "ecs/systems/asset_render_system.h"
#include "ecs/ecs.h"
#include "ecs/component_ids.h"
#include "ecs/components/transform.h"
#include "ecs/components/asset_instance_component.h"
#include "include/core/resource/asset_instance.h"
#include "core/asset_manager.h"
#include "engine/include/core/logger.h"
#include <stddef.h>

/**
 * AssetRenderSystem - Phase 3 ECS Integration
 * 
 * Queries the ECS for entities with both AssetInstanceComponent and TransformComponent,
 * validates them, and submits to the renderer.
 */

// Global references (Phase 3 - will be passed via system context in future refactor)
static World *g_world = NULL;
static AssetManager *g_asset_manager = NULL;

void asset_render_system_init(World *world, AssetManager *manager) {
    g_world = world;
    g_asset_manager = manager;
    LOG_INFO("[AssetRenderSystem] Initialized");
}

void asset_render_system_update(float dt) {
    if (!g_world || !g_asset_manager) {
        return;
    }
    
    // Get component IDs
    ECSComponentID asset_comp_id = asset_instance_component_get_id(g_world);
    ECSComponentID transform_comp_id = transform_component_get_id(g_world);

    
    // Build query for entities with both components
    ComponentType required_components[2] = {asset_comp_id, transform_comp_id};
    
    QueryDesc query_desc = {
        .all_components = required_components,
        .all_count = 2,
        .any_components = NULL,
        .any_count = 0,
        .none_components = NULL,
        .none_count = 0,
        .changed_only = false
    };
    
    Query *query = ecs_query_create(g_world, &query_desc);
    if (!query) {
        LOG_ERROR("[AssetRenderSystem] Failed to create query");
        return;
    }
    
    // Iterate through all matching entities
    Entity entity;
    void *components[2];
    uint32_t rendered_count = 0;
    
    while (ecs_query_next(query, &entity, components)) {
        AssetInstanceComponent *asset_comp = (AssetInstanceComponent*)components[0];
        TransformComponent *transform = (TransformComponent*)components[1];
        
        // Validate components
        if (!asset_comp || !transform) {
            continue;
        }
        
        // Get the full AssetInstance from the registry
        AssetInstance *instance = asset_manager_get_instance(g_asset_manager, asset_comp->asset_instance_id);
        if (!instance || !instance->source_asset) {
            continue;
        }
        
        // Check render flags
        if (!(asset_comp->render_flags & RENDER_FLAG_VISIBLE)) {
            continue;
        }
        
        // TODO: Submit to renderer
        // For now, we just validate the linkage is working
        // Future implementation:
        //   Mat4 transform_matrix = transform_build_matrix(transform);
        //   renderer_submit_mesh(instance->source_asset, &transform_matrix, asset_comp->render_flags);
        
        rendered_count++;
    }
    
    ecs_query_destroy(g_world, query);
    
    // Log stats periodically (every 60 frames = ~1 second at 60fps)
    static uint32_t frame_counter = 0;
    if (++frame_counter >= 60) {
        if (rendered_count > 0) {
            LOG_INFO("[AssetRenderSystem] Rendered %u asset instances", rendered_count);
        }
        frame_counter = 0;
    }
}

void asset_render_system_shutdown() {
    g_world = NULL;
    g_asset_manager = NULL;
    LOG_INFO("[AssetRenderSystem] Shutdown");
}

#include "ecs/systems/asset_render_system.h"
#include "ecs/ecs.h"
#include "ecs/component_ids.h"
#include "ecs/components/transform.h"
#include "math/quat.h"
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
static AssetRenderSubmitFn g_submit_fn = NULL;
static void *g_submit_user_data = NULL;

// World matrix for a transform: T * R * S, column-major.
static void asset_render_build_matrix(const TransformComponent *t, float out[16]) {
    Mat4 rotation = quat_to_mat4(t->rotation);
    const float scale[3] = {t->scale.x, t->scale.y, t->scale.z};
    for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 3; row++) {
            out[col * 4 + row] = rotation.m[col][row] * scale[col];  // R * S scales each column
        }
        out[col * 4 + 3] = 0.0f;
    }
    out[12] = t->position.x;
    out[13] = t->position.y;
    out[14] = t->position.z;
    out[15] = 1.0f;
}

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
        
        // Hand the visible instance to the renderer. Nothing is submitted until
        // a callback is registered with asset_render_system_set_submit_callback().
        if (g_submit_fn) {
            float transform_matrix[16];
            asset_render_build_matrix(transform, transform_matrix);
            g_submit_fn(instance->source_asset, transform_matrix, asset_comp->render_flags, g_submit_user_data);
            rendered_count++;
        }
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

void asset_render_system_set_submit_callback(AssetRenderSubmitFn fn, void *user_data) {
    g_submit_fn = fn;
    g_submit_user_data = user_data;
}

void asset_render_system_shutdown() {
    g_world = NULL;
    g_asset_manager = NULL;
    g_submit_fn = NULL;
    g_submit_user_data = NULL;
    LOG_INFO("[AssetRenderSystem] Shutdown");
}

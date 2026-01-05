#include "engine_bridge.h"
#include <core/memory.h>
#include <ecs/ecs.h>
#include <stdlib.h>
#include <string.h>

// Renderer Bridge Implementation
RendererBridge* renderer_bridge_create(void* metal_device) {
    RendererBridge* bridge = malloc(sizeof(RendererBridge));
    if (!bridge) return NULL;
    
    bridge->metal_device = metal_device;
    bridge->command_queue = NULL; // Will be set by Metal renderer
    bridge->render_frame = NULL;
    bridge->resize_viewport = NULL;
    
    return bridge;
}

void renderer_bridge_destroy(RendererBridge* bridge) {
    if (bridge) {
        free(bridge);
    }
}

void renderer_bridge_render_frame(RendererBridge* bridge, void* viewport, float delta_time) {
    if (bridge && bridge->render_frame) {
        bridge->render_frame(viewport, delta_time);
    }
}

void renderer_bridge_resize_viewport(RendererBridge* bridge, uint32_t width, uint32_t height) {
    if (bridge && bridge->resize_viewport) {
        bridge->resize_viewport(width, height);
    }
}

// ECS Bridge Implementation
Entity* ecs_bridge_get_all_entities(World* world, uint32_t* count) {
    if (!world || !count) return NULL;
    
    // Allocate array for entities
    uint32_t max_id = ecs_get_max_entity_id(world);
    Entity* entities = malloc(sizeof(Entity) * max_id);
    uint32_t valid_count = 0;
    
    // Collect all valid entities
    for (uint32_t i = 1; i < max_id; i++) {
        Entity entity = ecs_get_entity_by_id(world, i);
        if (ecs_is_valid(world, entity)) {
            entities[valid_count++] = entity;
        }
    }
    
    *count = valid_count;
    return entities;
}

void* ecs_bridge_get_component_data(World* world, Entity entity, ECSComponentID type) {
    return ecs_get_component(world, entity, type);
}

const char* ecs_bridge_get_component_name(World* world, ECSComponentID type) {
    const ComponentInfo* info = ecs_get_component_info(world, type);
    return info ? info->name : "Unknown";
}

// Memory Bridge Implementation
MemorySnapshot* memory_bridge_get_snapshots(uint32_t* count) {
    return memory_profiler_get_snapshots(count);
}

uint64_t memory_bridge_get_total_used(void) {
    return memory_get_total_used();
}

void memory_bridge_start_profiling(void) {
    memory_profiler_start_recording();
}

void memory_bridge_stop_profiling(void) {
    memory_profiler_stop_recording();
}

// GOAP Bridge Implementation (stub for now)
GOAPState ecs_bridge_get_goap_state(World* world, Entity entity) {
    GOAPState state = {0};
    state.current_goal = "Idle";
    state.plan_step_count = 0;
    state.planning_time_ms = 0.0f;
    state.is_planning = false;
    
    // TODO: Integrate with actual GOAP system in ai/goap.c
    
    return state;
}

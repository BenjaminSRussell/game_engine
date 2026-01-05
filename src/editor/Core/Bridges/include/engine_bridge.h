#ifndef ENGINE_BRIDGE_H
#define ENGINE_BRIDGE_H

#include <stdint.h>
#include <stdbool.h>

// Include engine headers for type definitions
#include <ecs/ecs.h>
#include <core/memory.h>

#ifdef __cplusplus
extern "C" {
#endif

// Renderer API for Metal viewport integration
typedef struct RendererBridge {
    void* metal_device;
    void* command_queue;
    void (*render_frame)(void* viewport, float delta_time);
    void (*resize_viewport)(uint32_t width, uint32_t height);
} RendererBridge;

RendererBridge* renderer_bridge_create(void* metal_device);
void renderer_bridge_destroy(RendererBridge* bridge);
void renderer_bridge_render_frame(RendererBridge* bridge, void* viewport, float delta_time);
void renderer_bridge_resize_viewport(RendererBridge* bridge, uint32_t width, uint32_t height);

// ECS API for entity inspection
Entity* ecs_bridge_get_all_entities(World* world, uint32_t* count);
void* ecs_bridge_get_component_data(World* world, Entity entity, ECSComponentID type);
const char* ecs_bridge_get_component_name(World* world, ECSComponentID type);

// Memory profiler API for memory viewer
MemorySnapshot* memory_bridge_get_snapshots(uint32_t* count);
uint64_t memory_bridge_get_total_used(void);
void memory_bridge_start_profiling(void);
void memory_bridge_stop_profiling(void);

// AI/GOAP API for debugging
typedef struct GOAPState {
    const char* current_goal;
    uint32_t plan_step_count;
    float planning_time_ms;
    bool is_planning;
} GOAPState;

GOAPState ecs_bridge_get_goap_state(World* world, Entity entity);

#ifdef __cplusplus
}
#endif

#endif // ENGINE_BRIDGE_H

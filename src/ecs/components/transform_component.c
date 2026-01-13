/**
 * @file transform_component.c
 * @brief ECS Implementation
 * @description Entity Component System implementation
 * @date 2026-01-13
 */

#include "ecs/components/transform.h"
#include "ecs/ecs.h"
#include <string.h>

static ECSComponentID g_transform_component_id = 0;

void transform_component_register(World *world) {
    if (g_transform_component_id != 0) {
        return; // Already registered
    }
    
    ComponentInfo info = {
        .name = "TransformComponent",
        .size = sizeof(TransformComponent),
        .alignment = 16,  // 16-byte alignment for potential SIMD optimization
        .is_tag = false,
        .default_constructor = NULL,
        .destructor = NULL,
        .serialize = NULL,
        .deserialize = NULL
    };
    
    g_transform_component_id = ecs_register_component(world, &info);
}

ECSComponentID transform_component_get_id(World *world) {
    if (g_transform_component_id == 0) {
        transform_component_register(world);
    }
    return g_transform_component_id;
}

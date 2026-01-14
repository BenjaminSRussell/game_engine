#include "ecs/components/asset_instance_component.h"
#include "ecs/ecs.h"

static ECSComponentID g_asset_instance_component_id = 0;

void asset_instance_component_register(World *world) {
    if (g_asset_instance_component_id != 0) {
        return; // Already registered
    }
    
    ComponentInfo info = {
        .name = "AssetInstanceComponent",
        .size = sizeof(AssetInstanceComponent),
        .alignment = 8,
        .is_tag = false,
        .default_constructor = NULL,
        .destructor = NULL,
        .serialize = NULL,
        .deserialize = NULL
    };
    
    g_asset_instance_component_id = ecs_register_component(world, &info);
}

ECSComponentID asset_instance_component_get_id(World *world) {
    if (g_asset_instance_component_id == 0) {
        asset_instance_component_register(world);
    }
    return g_asset_instance_component_id;
}

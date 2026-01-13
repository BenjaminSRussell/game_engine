#include <player/food_spoilage.h>
#include <inventory/inventory.h>
#include "engine/include/core/logger.h"

void food_spoilage_system_update(Player* player, const ItemRegistry* item_registry, f32 delta_time) {
    if (!player || !item_registry) {
        return;
    }

    for (u32 i = 0; i < MAX_INVENTORY_SLOTS; ++i) {
        InventorySlot* slot = &player->inventory.slots[i];
        if (slot->item_id == 0 || slot->count == 0) {
            continue;
        }

        const ExtendedItemDefinition* item_def = item_registry_get(item_registry, slot->item_id);
        if (item_def && item_is_food(item_def) && item_def->properties.food.spoil_time > 0) {
            slot->spoil_progress += delta_time / item_def->properties.food.spoil_time;
            if (slot->spoil_progress >= 1.0f) {
                LOG_INFO("Item '%s' spoiled in slot %u.", item_def->base.name, i);
                slot->item_id = ITEM_SPOILED_FOOD;
                slot->spoil_progress = 0.0f;
                // a good place to also reset quality, etc. if we want spoiled food to be uniform
            }
        }
    }
}

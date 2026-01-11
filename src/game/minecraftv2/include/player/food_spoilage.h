#ifndef FOOD_SPOILAGE_H
#define FOOD_SPOILAGE_H

#include <player/player.h>
#include <inventory/item_registry.h>

void food_spoilage_system_update(Player* player, const ItemRegistry* item_registry, f32 delta_time);

#endif // FOOD_SPOILAGE_H

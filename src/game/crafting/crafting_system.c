#include "game/crafting/crafting_system.h"
#include <stdlib.h>

void crafting_init() {}

void crafting_register_recipe(void *recipe) {}

int crafting_can_craft(void *ingredients, void *recipe) {
    return 1; // Can craft
}

void crafting_execute(void *ingredients, void *recipe, void *output_item) {
    // Consume ingredients and create output item
}

void crafting_discover_recipe(void *player, void *recipe) {}

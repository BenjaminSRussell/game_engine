#include <ui/hud_tooltip.h>
#include <ui/hud.h>
#include <stdio.h>

void hud_tooltip_draw(HUDSystem* hud, const ExtendedItemDefinition* item) {
    if (!hud || !item || !item_is_food(item)) {
        return;
    }

    const FoodProperties* food = &item->properties.food;
    char buffer[256];

    // TODO: This is a stub. Replace with actual UI drawing code.
    // For now, we'll just log the tooltip info.
    
    snprintf(buffer, sizeof(buffer), "Nutrition: %.1f", food->hunger_restored * food->quality);
    LOG_INFO("Tooltip: %s", buffer);

    snprintf(buffer, sizeof(buffer), "Saturation: %.1f", food->hunger_restored * food->saturation_modifier * food->quality);
    LOG_INFO("Tooltip: %s", buffer);

    if (food->spoil_time > 0) {
        snprintf(buffer, sizeof(buffer), "Spoils in: %.0f sec", food->spoil_time);
        LOG_INFO("Tooltip: %s", buffer);
    }
}

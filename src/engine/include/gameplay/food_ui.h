#ifndef FOOD_UI_H
#define FOOD_UI_H

#include "ui/ui_manager.h"
#include "gameplay/food_system.h"
#include <math/vec2.h>

typedef struct {
    UIManager* ui_manager;
    UIElement* tooltip_panel;
    UIElement* name_label;
    UIElement* description_label;
    UIElement* stats_label;
    UIElement* nutrients_label;
    bool is_visible;
} food_ui_context_t;

// Create the food UI system (specifically the tooltip)
food_ui_context_t* food_ui_create(UIManager* ui_manager);

// Destroy the food UI system
void food_ui_destroy(food_ui_context_t* context);

// Show the tooltip for a specific food item at a screen position
void food_ui_show_tooltip(food_ui_context_t* context, uint32_t food_id, Vec2 position);

// Hide the tooltip
void food_ui_hide_tooltip(food_ui_context_t* context);

#endif // FOOD_UI_H

#include "gameplay/food_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static UIElement* create_label(UIManager* ui, const char* id, Vec2 pos, Vec2 size, const char* text, Vec4 color, float font_size) {
    UIElementDesc desc = {
        .type = UI_ELEMENT_LABEL,
        .id = id,
        .position = pos,
        .size = size,
        .text = text,
        .color = color,
        .font_size = font_size,
        .visible = true,
        .enabled = true
    };
    return ui->create_element(ui, &desc);
}

food_ui_context_t* food_ui_create(UIManager* ui_manager) {
    if (!ui_manager) return NULL;

    food_ui_context_t* ctx = malloc(sizeof(food_ui_context_t));
    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(food_ui_context_t));
    ctx->ui_manager = ui_manager;

    return ctx;
}

void food_ui_destroy(food_ui_context_t* context) {
    if (!context) return;

    if (context->ui_manager) {
        if (context->tooltip_panel) context->ui_manager->destroy_element(context->ui_manager, context->tooltip_panel);
        if (context->name_label) context->ui_manager->destroy_element(context->ui_manager, context->name_label);
        if (context->description_label) context->ui_manager->destroy_element(context->ui_manager, context->description_label);
        if (context->stats_label) context->ui_manager->destroy_element(context->ui_manager, context->stats_label);
        if (context->nutrients_label) context->ui_manager->destroy_element(context->ui_manager, context->nutrients_label);
    }

    free(context);
}

void food_ui_show_tooltip(food_ui_context_t* context, uint32_t food_id, Vec2 position) {
    if (!context || !context->ui_manager) return;

    const food_item_t* item = food_get_item(food_id);
    if (!item) {
        food_ui_hide_tooltip(context);
        return;
    }

    // Cleanup old elements
    if (context->tooltip_panel) context->ui_manager->destroy_element(context->ui_manager, context->tooltip_panel);
    if (context->name_label) context->ui_manager->destroy_element(context->ui_manager, context->name_label);
    if (context->description_label) context->ui_manager->destroy_element(context->ui_manager, context->description_label);
    if (context->stats_label) context->ui_manager->destroy_element(context->ui_manager, context->stats_label);
    if (context->nutrients_label) context->ui_manager->destroy_element(context->ui_manager, context->nutrients_label);

    // Format Text
    char stats_buf[256];
    snprintf(stats_buf, sizeof(stats_buf),
        "Health: +%.1f  Stamina: +%.1f\nHunger: -%.1f  Thirst: -%.1f",
        item->health_restore, item->stamina_restore, item->hunger_reduction, item->thirst_reduction);

    char nutrients_buf[1024];
    strcpy(nutrients_buf, "Nutrients:\n");
    for(uint32_t i=0; i<item->nutrient_count; i++) {
        char line[128];
        const nutrient_value_t* n = &item->nutrients[i];
        const char* type_name = "Unknown";
        switch(n->type) {
            case NUTRIENT_PROTEIN: type_name = "Protein"; break;
            case NUTRIENT_CARBS: type_name = "Carbs"; break;
            case NUTRIENT_FAT: type_name = "Fat"; break;
            case NUTRIENT_VITAMINS: type_name = "Vitamins"; break;
            case NUTRIENT_MINERALS: type_name = "Minerals"; break;
            case NUTRIENT_WATER: type_name = "Water"; break;
            case NUTRIENT_FIBER: type_name = "Fiber"; break;
            case NUTRIENT_ANTIOXIDANTS: type_name = "Antiox"; break;
        }
        snprintf(line, sizeof(line), "- %s: %.1f\n", type_name, n->amount);
        strcat(nutrients_buf, line);
    }

    // Layout
    float width = 250.0f;
    float padding = 10.0f;
    float current_y = position.y + padding;
    float x = position.x + padding;

    // Estimate height
    float height = padding * 2;
    height += 25; // Name
    height += 40; // Desc
    height += 40; // Stats
    height += 20 * (item->nutrient_count + 1); // Nutrients

    UIElementDesc panel_desc = {
        .type = UI_ELEMENT_PANEL,
        .id = "food_tooltip_panel",
        .position = position,
        .size = {width, height},
        .background_color = {0.1f, 0.1f, 0.1f, 0.95f},
        .visible = true,
        .enabled = false
    };
    context->tooltip_panel = context->ui_manager->create_element(context->ui_manager, &panel_desc);

    // Create Labels
    context->name_label = create_label(context->ui_manager, "food_tooltip_name",
        (Vec2){x, current_y}, (Vec2){width-20, 25}, item->name, (Vec4){1,1,1,1}, 20.0f);
    current_y += 30;

    context->description_label = create_label(context->ui_manager, "food_tooltip_desc",
        (Vec2){x, current_y}, (Vec2){width-20, 40}, item->description, (Vec4){0.8f,0.8f,0.8f,1}, 14.0f);
    current_y += 45;

    context->stats_label = create_label(context->ui_manager, "food_tooltip_stats",
        (Vec2){x, current_y}, (Vec2){width-20, 40}, stats_buf, (Vec4){0.7f,1.0f,0.7f,1}, 14.0f);
    current_y += 45;

    context->nutrients_label = create_label(context->ui_manager, "food_tooltip_nutr",
        (Vec2){x, current_y}, (Vec2){width-20, 20.0f * (item->nutrient_count + 1)}, nutrients_buf, (Vec4){0.7f,0.7f,1.0f,1}, 14.0f);

    context->is_visible = true;
}

void food_ui_hide_tooltip(food_ui_context_t* context) {
    if (!context || !context->ui_manager || !context->is_visible) return;

    if (context->tooltip_panel) context->ui_manager->destroy_element(context->ui_manager, context->tooltip_panel);
    if (context->name_label) context->ui_manager->destroy_element(context->ui_manager, context->name_label);
    if (context->description_label) context->ui_manager->destroy_element(context->ui_manager, context->description_label);
    if (context->stats_label) context->ui_manager->destroy_element(context->ui_manager, context->stats_label);
    if (context->nutrients_label) context->ui_manager->destroy_element(context->ui_manager, context->nutrients_label);

    context->tooltip_panel = NULL;
    context->name_label = NULL;
    context->description_label = NULL;
    context->stats_label = NULL;
    context->nutrients_label = NULL;

    context->is_visible = false;
}

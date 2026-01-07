// TODO: Implement menu rendering batching system.
// TODO: Add menu rendering caching system.
// TODO: Implement menu rendering optimization.
// TODO: Add menu rendering statistics tracking.
// TODO: Implement menu rendering debugging visualization.
// TODO: Add menu rendering performance profiling.
// TODO: Implement menu rendering configuration system.
// TODO: Add menu rendering unit testing framework.
// TODO: Implement menu rendering documentation system.
// TODO: Add menu rendering optimization suggestions.
#include "../include/ui/menu_renderer.h"
#include "../../../engine/include/common.h"
#include "../../../engine/include/core/logger.h"
#include "../../../engine/include/input/controls.h"
#include <stdio.h>
#include <string.h>

static u32 g_screen_width = 0;
static u32 g_screen_height = 0;
static bool g_initialized = false;

void menu_renderer_init(u32 screen_width, u32 screen_height) {
  g_screen_width = screen_width;
  g_screen_height = screen_height;
  g_initialized = true;
  LOG_INFO("Menu renderer initialized (%ux%u)", screen_width, screen_height);
}

void menu_draw_filled_rect(f32 x, f32 y, f32 w, f32 h, Color color) {
  if (!g_initialized)
    return;

  // Console-based rendering for testing
  // In full implementation, this would use Vulkan to draw rectangles
  printf("[DRAW_RECT] x=%.0f y=%.0f w=%.0f h=%.0f color=(%d,%d,%d,%d)\n", x, y,
         w, h, color.r, color.g, color.b, color.a);
}

void menu_draw_outlined_rect(f32 x, f32 y, f32 w, f32 h, Color color,
                             f32 thickness) {
  if (!g_initialized)
    return;

  printf("[DRAW_OUTLINE] x=%.0f y=%.0f w=%.0f h=%.0f thickness=%.1f "
         "color=(%d,%d,%d,%d)\n",
         x, y, w, h, thickness, color.r, color.g, color.b, color.a);
}

void menu_draw_text(const char *text, f32 x, f32 y, Color color, f32 size) {
  if (!g_initialized || !text)
    return;

  printf("[DRAW_TEXT] \"%s\" at (%.0f,%.0f) size=%.1f color=(%d,%d,%d,%d)\n",
         text, x, y, size, color.r, color.g, color.b, color.a);
}

void menu_draw_button(MenuButton *button) {
  if (!button)
    return;

  // Choose color based on button state
  Color bg_color = COLOR_GRAY;
  Color text_color = COLOR_WHITE;

  if (button->hovered) {
    bg_color = COLOR_LIGHT_GRAY;
  }
  if (button->clicked) {
    bg_color = COLOR_DARK_GRAY;
  }

  // Draw button background
  menu_draw_filled_rect(button->position.x, button->position.y, button->size.x,
                        button->size.y, bg_color);

  // Draw button border
  menu_draw_outlined_rect(button->position.x, button->position.y,
                          button->size.x, button->size.y, COLOR_WHITE, 2.0f);

  // Draw button text (centered)
  f32 text_x =
      button->position.x + button->size.x / 2.0f - 20.0f; // Rough centering
  f32 text_y = button->position.y + button->size.y / 2.0f - 10.0f;
  menu_draw_text(button->text, text_x, text_y, text_color, 16.0f);
}

void menu_draw_panel(MenuPanel *panel) {
  if (!panel || !panel->visible)
    return;

  // Draw panel background
  Color panel_bg = (Color){40, 40, 50, 200}; // Semi-transparent dark blue
  menu_draw_filled_rect(panel->position.x, panel->position.y, panel->size.x,
                        panel->size.y, panel_bg);

  // Draw panel border
  menu_draw_outlined_rect(panel->position.x, panel->position.y, panel->size.x,
                          panel->size.y, COLOR_LIGHT_GRAY, 3.0f);

  // Draw all buttons in panel
  for (u32 i = 0; i < panel->button_count; i++) {
    menu_draw_button(&panel->buttons[i]);
  }
}

void menu_renderer_draw(MenuSystem *menu) {
  if (!menu || !g_initialized)
    return;

  printf("\n========== RENDERING MENU ==========\n");

  if (menu->state == MENU_STATE_MAIN) {
    // Draw main menu background
    menu_draw_filled_rect(0, 0, (f32)g_screen_width, (f32)g_screen_height,
                          (Color){20, 20, 30, 255});

    // Draw title
    menu_draw_text("MINECRAFT CLONE", (f32)g_screen_width / 2.0f - 100.0f,
                   100.0f, COLOR_WHITE, 32.0f);

    // Draw main menu panel
    if (menu->panel_count > 0) {
      menu_draw_panel(&menu->panels[0]);
    }

  } else if (menu->state == MENU_STATE_WORLD_SELECT) {
    // Draw world selection background
    menu_draw_filled_rect(0, 0, (f32)g_screen_width, (f32)g_screen_height,
                          (Color){20, 20, 30, 255});

    // Draw title
    menu_draw_text("SELECT WORLD", (f32)g_screen_width / 2.0f - 80.0f, 50.0f,
                   COLOR_WHITE, 24.0f);

    // Draw world list
    f32 list_y = 150.0f;
    if (menu->world_select.count > 0) {
      for (u32 i = 0; i < menu->world_select.count; i++) {
        Color item_color = (i == menu->world_select.selected_index)
                               ? COLOR_LIGHT_GRAY
                               : COLOR_GRAY;

        menu_draw_filled_rect(100.0f, list_y, 600.0f, 60.0f, item_color);
        menu_draw_outlined_rect(100.0f, list_y, 600.0f, 60.0f, COLOR_WHITE,
                                2.0f);

        char world_info[128];
        snprintf(world_info, sizeof(world_info), "%s (Seed: %u)",
                 menu->world_select.world_names[i],
                 menu->world_select.world_seeds[i]);
        menu_draw_text(world_info, 120.0f, list_y + 20.0f, COLOR_WHITE, 16.0f);

        list_y += 70.0f;
      }
      // Draw "Create New World" button (always visible in world select)
      menu_draw_filled_rect(100.0f, list_y + 20.0f, 600.0f, 60.0f, COLOR_GREEN);
      menu_draw_outlined_rect(100.0f, list_y + 20.0f, 600.0f, 60.0f,
                              COLOR_WHITE, 2.0f);
      menu_draw_text("Create New World", 300.0f, list_y + 40.0f, COLOR_WHITE,
                     18.0f);
    } else {
      menu_draw_text("No saved worlds found", 200.0f, list_y, COLOR_LIGHT_GRAY,
                     16.0f);
      // Draw "Create New World" button
      menu_draw_filled_rect(100.0f, list_y + 20.0f, 600.0f, 60.0f, COLOR_GREEN);
      menu_draw_outlined_rect(100.0f, list_y + 20.0f, 600.0f, 60.0f,
                              COLOR_WHITE, 2.0f);
      menu_draw_text("Create New World", 300.0f, list_y + 40.0f, COLOR_WHITE,
                     18.0f);
    }

  } else if (menu->state == MENU_STATE_WORLD_CREATE) {
    // Draw world creation background
    menu_draw_filled_rect(0, 0, (f32)g_screen_width, (f32)g_screen_height,
                          (Color){20, 20, 30, 255});

    // Draw title
    menu_draw_text("CREATE NEW WORLD", (f32)g_screen_width / 2.0f - 100.0f,
                   50.0f, COLOR_WHITE, 24.0f);

    f32 y = 120.0f;

    // World Name Input
    menu_draw_text("World Name:", 100.0f, y, COLOR_LIGHT_GRAY, 16.0f);
    menu_draw_filled_rect(250.0f, y - 10.0f, 400.0f, 30.0f, COLOR_DARK_GRAY);
    menu_draw_outlined_rect(250.0f, y - 10.0f, 400.0f, 30.0f, COLOR_WHITE,
                            1.0f);
    menu_draw_text(menu->world_create.name, 260.0f, y, COLOR_WHITE, 16.0f);
    y += 50.0f;

    // Seed Input
    menu_draw_text("Seed:", 100.0f, y, COLOR_LIGHT_GRAY, 16.0f);
    menu_draw_filled_rect(250.0f, y - 10.0f, 400.0f, 30.0f, COLOR_DARK_GRAY);
    menu_draw_outlined_rect(250.0f, y - 10.0f, 400.0f, 30.0f, COLOR_WHITE,
                            1.0f);
    char seed_str[32];
    snprintf(seed_str, sizeof(seed_str), "%u", menu->world_create.seed);
    menu_draw_text(seed_str, 260.0f, y, COLOR_WHITE, 16.0f);
    y += 50.0f;

    // Difficulty
    menu_draw_text("Difficulty:", 100.0f, y, COLOR_LIGHT_GRAY, 16.0f);
    const char *diff_names[] = {"Peaceful", "Easy", "Normal", "Hard"};
    menu_draw_text(diff_names[menu->world_create.difficulty % 4], 260.0f, y,
                   COLOR_WHITE, 16.0f);
    menu_draw_text("[<]  [>]", 400.0f, y, COLOR_GRAY, 16.0f);
    y += 50.0f;

    // Game Mode
    menu_draw_text("Game Mode:", 100.0f, y, COLOR_LIGHT_GRAY, 16.0f);
    menu_draw_text(menu->world_create.creative_mode ? "Creative" : "Survival",
                   260.0f, y, COLOR_WHITE, 16.0f);
    menu_draw_text("[Toggle]", 400.0f, y, COLOR_GRAY, 16.0f);
    y += 80.0f;

    // Buttons
    // Create
    menu_draw_filled_rect(200.0f, y, 150.0f, 40.0f, COLOR_GREEN);
    menu_draw_text("Create", 240.0f, y + 25.0f, COLOR_WHITE, 18.0f);

    // Cancel
    menu_draw_filled_rect(400.0f, y, 150.0f, 40.0f, COLOR_RED);
    menu_draw_text("Cancel", 440.0f, y + 25.0f, COLOR_WHITE, 18.0f);

  } else if (menu->state == MENU_STATE_SETTINGS) {
    menu_draw_filled_rect(0, 0, (f32)g_screen_width, (f32)g_screen_height,
                          (Color){20, 20, 30, 255});

    menu_draw_text("SETTINGS", (f32)g_screen_width / 2.0f - 60.0f, 60.0f,
                   COLOR_WHITE, 24.0f);
    menu_draw_text("Controls Profiles", 100.0f, 110.0f, COLOR_LIGHT_GRAY,
                   16.0f);

    const f32 list_x = 100.0f;
    const f32 list_y = 140.0f;
    const f32 list_w = 600.0f;
    const f32 row_h = 40.0f;
    u32 count = menu->controls.profiles ? menu->controls.profiles->count : 0;

    if (menu->controls.profiles && count > 0) {
      for (u32 i = 0; i < count; i++) {
        f32 row_top = list_y + row_h * (f32)i;
        Color row_color =
            (i == menu->controls.selected_index) ? COLOR_GREEN : COLOR_GRAY;
        menu_draw_filled_rect(list_x, row_top, list_w, row_h, row_color);
        menu_draw_outlined_rect(list_x, row_top, list_w, row_h, COLOR_WHITE,
                                1.0f);

        const char *name = menu->controls.profiles->profiles[i].name;
        bool is_active =
            (strcmp(name, menu->controls.profiles->active_profile) == 0);
        char label[64];
        if (is_active) {
          snprintf(label, sizeof(label), "%s (active)", name);
          menu_draw_text(label, list_x + 16.0f, row_top + 24.0f, COLOR_WHITE,
                         16.0f);
        } else {
          menu_draw_text(name, list_x + 16.0f, row_top + 24.0f, COLOR_WHITE,
                         16.0f);
        }
      }
    } else {
      menu_draw_text("No profiles loaded", list_x + 16.0f, list_y + 24.0f,
                     COLOR_LIGHT_GRAY, 16.0f);
    }

    f32 buttons_y = list_y + row_h * (f32)MAX(count, 1) + 30.0f;
    menu_draw_filled_rect(100.0f, buttons_y, 140.0f, 40.0f, COLOR_GREEN);
    menu_draw_text("Save", 140.0f, buttons_y + 25.0f, COLOR_WHITE, 18.0f);
    menu_draw_filled_rect(560.0f, buttons_y, 140.0f, 40.0f, COLOR_RED);
    menu_draw_text("Back", 600.0f, buttons_y + 25.0f, COLOR_WHITE, 18.0f);

    if (menu->controls.message_timer > 0.0f) {
      menu_draw_text(menu->controls.message, 100.0f, buttons_y - 10.0f,
                     COLOR_GREEN, 14.0f);
    }
  } else if (menu->state == MENU_STATE_IN_GAME) {
    menu_draw_text("Loading world...", (f32)g_screen_width / 2.0f - 80.0f,
                   (f32)g_screen_height / 2.0f, COLOR_WHITE, 20.0f);
  }

  printf("====================================\n\n");
}

void menu_renderer_free(void) {
  g_initialized = false;
  LOG_INFO("Menu renderer freed");
}

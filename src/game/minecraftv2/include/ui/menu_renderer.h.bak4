#ifndef MENU_RENDERER_H
#define MENU_RENDERER_H

#include "../game_common.h"
#include "../ui/menu.h"

// Predefined colors
#define COLOR_BLACK ((Color){0, 0, 0, 255})
#define COLOR_WHITE ((Color){255, 255, 255, 255})
#define COLOR_GRAY ((Color){128, 128, 128, 255})
#define COLOR_LIGHT_GRAY ((Color){192, 192, 192, 255})
#define COLOR_DARK_GRAY ((Color){64, 64, 64, 255})
#define COLOR_BLUE ((Color){0, 100, 200, 255})
#define COLOR_GREEN ((Color){0, 200, 0, 255})
#define COLOR_RED ((Color){200, 0, 0, 255})

// Initialize menu renderer
void menu_renderer_init(u32 screen_width, u32 screen_height);

// Basic drawing functions
void menu_draw_filled_rect(f32 x, f32 y, f32 w, f32 h, Color color);
void menu_draw_outlined_rect(f32 x, f32 y, f32 w, f32 h, Color color,
                             f32 thickness);
void menu_draw_text(const char *text, f32 x, f32 y, Color color, f32 size);

// High-level menu drawing
void menu_draw_button(MenuButton *button);
void menu_draw_panel(MenuPanel *panel);
void menu_renderer_draw(MenuSystem *menu);

// Cleanup
void menu_renderer_free(void);

#endif // MENU_RENDERER_H

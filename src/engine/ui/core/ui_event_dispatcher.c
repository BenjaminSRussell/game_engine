/**
 * @file ui_event_dispatcher.c
 * @brief UI Event Routing System.
 *
 * Handles Mouse/Keyboard input, Hit Testing, and Focus management.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <ui/core/ui_event_dispatcher.h>

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

bool ui_rect_contains(UIWidget *w, float x, float y) {
  return x >= w->final_x && x <= (w->final_x + w->final_w) && y >= w->final_y &&
         y <= (w->final_y + w->final_h);
}

/**
 * @brief Finds the deepest widget under cursor.
 */
UIWidget *ui_hit_test(UIWidget *root, float x, float y) {
  if (!ui_rect_contains(root, x, y))
    return NULL;

  // Check children in reverse Z-order (topmost first)
  for (int i = root->child_count - 1; i >= 0; i--) {
    UIWidget *hit = ui_hit_test(root->children[i], x, y);
    if (hit)
      return hit;
  }

  return root;
}

void ui_dispatch_mouse_event(UIWidget *root, float x, float y, int button,
                             bool down) {
  UIWidget *target = ui_hit_test(root, x, y);

  // Bubble up event?
  while (target) {
    // bool handled = target->on_mouse_event(...);
    // if (handled) break;
    target = target->parent;
  }

  // Handle Hover states
  // ...
}

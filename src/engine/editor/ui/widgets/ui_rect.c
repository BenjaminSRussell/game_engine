/*
 * ui_rect.c
 * Rectangle rendering implementation
 */

#include "editor/ui/widgets/ui_rect.h"
#include <core/logger.h>
#include <core/memory.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================
 */

#define UI_RENDERING_UI_RECT_MAX_COUNT 4096
#define UI_RENDERING_UI_RECT_DEFAULT_CAPACITY 256

/* ============================================================================
 * TYPES
 * ============================================================================
 */

typedef struct ui_rendering_ui_rect_internal {
  uint32_t id;
  uint32_t flags;
  Vec2 position;
  Vec2 size;
  Vec4 color;
  bool initialized;
  bool dirty;
  uint64_t frame_updated;
} ui_rendering_ui_rect_internal_t;

typedef struct ui_rendering_ui_rect_context {
  ui_rendering_ui_rect_internal_t *items;
  uint32_t count;
  uint32_t capacity;
  bool initialized;
} ui_rendering_ui_rect_context_t;

static ui_rendering_ui_rect_context_t g_ui_rect_ctx = {0};

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

int ui_rendering_ui_rect_init(void) {
  if (g_ui_rect_ctx.initialized) {
    return 0;
  }

  g_ui_rect_ctx.capacity = UI_RENDERING_UI_RECT_DEFAULT_CAPACITY;
  g_ui_rect_ctx.items = (ui_rendering_ui_rect_internal_t *)calloc(
      g_ui_rect_ctx.capacity, sizeof(ui_rendering_ui_rect_internal_t));
  if (!g_ui_rect_ctx.items) {
    LOG_ERROR("Failed to allocate UI Rect items");
    return -1;
  }

  g_ui_rect_ctx.count = 0;
  g_ui_rect_ctx.initialized = true;
  LOG_INFO("UI Rect system initialized");

  return 0;
}

void ui_rendering_ui_rect_shutdown(void) {
  if (!g_ui_rect_ctx.initialized) {
    return;
  }

  if (g_ui_rect_ctx.items) {
    free(g_ui_rect_ctx.items);
    g_ui_rect_ctx.items = NULL;
  }
  g_ui_rect_ctx.count = 0;
  g_ui_rect_ctx.capacity = 0;
  g_ui_rect_ctx.initialized = false;
}

int ui_rendering_ui_rect_create(ui_rendering_ui_rect_handle_t *out_handle,
                                const ui_rendering_ui_rect_desc_t *desc) {
  if (!out_handle || !desc) {
    return -1;
  }

  if (!g_ui_rect_ctx.initialized) {
    return -2;
  }

  if (g_ui_rect_ctx.count >= g_ui_rect_ctx.capacity) {
    // Expand
    uint32_t new_capacity = g_ui_rect_ctx.capacity * 2;
    ui_rendering_ui_rect_internal_t *new_items =
        (ui_rendering_ui_rect_internal_t *)realloc(
            g_ui_rect_ctx.items,
            new_capacity * sizeof(ui_rendering_ui_rect_internal_t));
    if (!new_items) {
      LOG_ERROR("Failed to resize UI Rect array");
      return -3;
    }
    g_ui_rect_ctx.items = new_items;
    g_ui_rect_ctx.capacity = new_capacity;
  }

  uint32_t index = g_ui_rect_ctx.count++;
  ui_rendering_ui_rect_internal_t *item = &g_ui_rect_ctx.items[index];

  item->id = index;
  item->flags = desc->flags;
  item->position = (Vec2){desc->x, desc->y};
  item->size = (Vec2){desc->width, desc->height};
  item->color =
      (Vec4){desc->color[0], desc->color[1], desc->color[2], desc->color[3]};
  item->initialized = true;
  item->dirty = true;
  item->frame_updated = 0;

  out_handle->id = index;
  return 0;
}

void ui_rendering_ui_rect_destroy(ui_rendering_ui_rect_handle_t handle) {
  if (handle.id >= g_ui_rect_ctx.count) {
    return;
  }
  // Mark as invalid/free - simplified for now, usually we swap-remove
  g_ui_rect_ctx.items[handle.id].initialized = false;
}

int ui_rendering_ui_rect_update(ui_rendering_ui_rect_handle_t handle,
                                const ui_rendering_ui_rect_desc_t *desc) {
  if (handle.id >= g_ui_rect_ctx.count)
    return -1;

  ui_rendering_ui_rect_internal_t *item = &g_ui_rect_ctx.items[handle.id];
  if (!item->initialized)
    return -2;

  item->position = (Vec2){desc->x, desc->y};
  item->size = (Vec2){desc->width, desc->height};
  item->color =
      (Vec4){desc->color[0], desc->color[1], desc->color[2], desc->color[3]};
  item->dirty = true;

  return 0;
}

// Temporary render function to expose to Canvas
// In a real implementation this would generate vertex data
void ui_rendering_ui_rect_get_render_data(ui_rendering_ui_rect_handle_t handle,
                                          Vec2 *pos, Vec2 *size, Vec4 *color) {
  if (handle.id >= g_ui_rect_ctx.count)
    return;
  ui_rendering_ui_rect_internal_t *item = &g_ui_rect_ctx.items[handle.id];

  if (pos)
    *pos = item->position;
  if (size)
    *size = item->size;
  if (color)
    *color = item->color;
}

uint32_t ui_rendering_ui_rect_get_count(void) { return g_ui_rect_ctx.count; }

#include "scroll_view.h"
#include "core/logger.h"
#include "core/memory.h"
#include <math.h>
#include <string.h>

#define SCROLL_BAR_WIDTH 12.0f
#define SCROLL_SENSITIVITY 20.0f

typedef struct {
  Widget *content;
  Vec2 scroll_offset;
  bool show_h_scrollbar;
  bool show_v_scrollbar;
  float scrollbar_width;
} ScrollViewData;

static void scroll_view_layout(Widget *widget, float available_width,
                               float available_height);
static void scroll_view_destroy(Widget *widget);
static bool scroll_view_handle_event(Widget *widget, UIEvent *event);

Widget *scroll_view_create(const char *name, Vec2 size) {
  Widget *widget = widget_create(name);
  if (!widget)
    return NULL;

  ScrollViewData *data = memory_alloc(sizeof(ScrollViewData));
  if (!data) {
    LOG_ERROR("Failed to allocate scroll view data");
    widget_destroy(widget);
    return NULL;
  }

  memset(data, 0, sizeof(ScrollViewData));
  data->scrollbar_width = SCROLL_BAR_WIDTH;
  data->show_h_scrollbar = true; // Default to showing scrollbars
  data->show_v_scrollbar = true;

  widget->user_data = data;
  widget->layout = scroll_view_layout;
  widget->destroy = scroll_view_destroy;
  widget->handle_event = scroll_view_handle_event;

  // Set default size
  widget_set_size(widget, size);

  return widget;
}

static void scroll_view_destroy(Widget *widget) {
  if (widget && widget->user_data) {
    memory_free(widget->user_data);
    widget->user_data = NULL;
  }
}

static void scroll_view_layout(Widget *widget, float available_width,
                               float available_height) {
  ScrollViewData *data = (ScrollViewData *)widget->user_data;
  if (!data || !data->content)
    return;

  float viewport_w = widget->size.x;
  float viewport_h = widget->size.y;

  // Adjust viewport for scrollbars
  // Note: This logic assumes scrollbars take up space inside the widget size
  // For a robust implementation, we might overlay them or reserve space only if
  // needed (auto). Here we reserve if enabled.
  float content_view_w = viewport_w;
  float content_view_h = viewport_h;

  if (data->show_v_scrollbar)
    content_view_w -= data->scrollbar_width;
  if (data->show_h_scrollbar)
    content_view_h -= data->scrollbar_width;

  // Determine layout constraints for child
  float child_avail_w = data->show_h_scrollbar ? 1e9f : content_view_w;
  float child_avail_h = data->show_v_scrollbar ? 1e9f : content_view_h;

  // Layout the content
  if (data->content->layout) {
    data->content->layout(data->content, child_avail_w, child_avail_h);
  }

  // If content doesn't have layout (leaf), we assume its size is already set or
  // we don't touch it? Widget system usually requires calling layout.

  Vec2 content_size = data->content->size;

  // Clamp scroll offset
  float max_scroll_x = fmaxf(0.0f, content_size.x - content_view_w);
  float max_scroll_y = fmaxf(0.0f, content_size.y - content_view_h);

  data->scroll_offset.x =
      fmaxf(0.0f, fminf(data->scroll_offset.x, max_scroll_x));
  data->scroll_offset.y =
      fmaxf(0.0f, fminf(data->scroll_offset.y, max_scroll_y));

  // Update content position
  // We access position directly to avoid triggering unnecessary invalidations
  // if possible, but widget_set_position handles logic. Note: setting position
  // to negative offset moves content up/left.
  Vec2 new_pos = {-data->scroll_offset.x, -data->scroll_offset.y};
  widget_set_position(data->content, new_pos);
}

static bool scroll_view_handle_event(Widget *widget, UIEvent *event) {
  ScrollViewData *data = (ScrollViewData *)widget->user_data;
  if (!data)
    return false;

  if (event->type == UI_EVENT_MOUSE_WHEEL) {
    float delta = event->mouse.wheel_delta * SCROLL_SENSITIVITY;

    // Vertical scroll by default
    // TODO: Handle horizontal with Shift

    Vec2 old_offset = data->scroll_offset;

    // Scroll up/down
    data->scroll_offset.y -= delta;

    // Re-clamp in layout or here?
    // Layout will clamp, but we want immediate feedback.
    // We can just set dirty and let layout handle it, but we also want to
    // consume event only if we actually scrolled? For now, always consume wheel
    // if we have scrollbars.

    if (old_offset.y != data->scroll_offset.y) {
      widget_invalidate_layout(widget);
      return true;
    }
  }

  // TODO: Handle scrollbar dragging

  return false;
}

void scroll_view_set_content(Widget *scroll_view, Widget *content) {
  if (!scroll_view || !scroll_view->user_data)
    return;
  ScrollViewData *data = (ScrollViewData *)scroll_view->user_data;

  if (data->content) {
    widget_remove_child(scroll_view, data->content);
  }

  data->content = content;
  if (content) {
    widget_add_child(scroll_view, content);
  }
}

Widget *scroll_view_get_content(const Widget *scroll_view) {
  if (!scroll_view || !scroll_view->user_data)
    return NULL;
  ScrollViewData *data = (ScrollViewData *)scroll_view->user_data;
  return data->content;
}

void scroll_view_set_scroll_offset(Widget *scroll_view, Vec2 offset) {
  if (!scroll_view || !scroll_view->user_data)
    return;
  ScrollViewData *data = (ScrollViewData *)scroll_view->user_data;

  if (data->scroll_offset.x != offset.x || data->scroll_offset.y != offset.y) {
    data->scroll_offset = offset;
    widget_invalidate_layout(scroll_view);
  }
}

Vec2 scroll_view_get_scroll_offset(const Widget *scroll_view) {
  if (!scroll_view || !scroll_view->user_data)
    return (Vec2){0, 0};
  ScrollViewData *data = (ScrollViewData *)scroll_view->user_data;
  return data->scroll_offset;
}

void scroll_view_set_show_scrollbars(Widget *scroll_view, bool horizontal,
                                     bool vertical) {
  if (!scroll_view || !scroll_view->user_data)
    return;
  ScrollViewData *data = (ScrollViewData *)scroll_view->user_data;

  if (data->show_h_scrollbar != horizontal ||
      data->show_v_scrollbar != vertical) {
    data->show_h_scrollbar = horizontal;
    data->show_v_scrollbar = vertical;
    widget_invalidate_layout(scroll_view);
  }
}

void scroll_view_get_show_scrollbars(const Widget *scroll_view,
                                     bool *horizontal, bool *vertical) {
  if (!scroll_view || !scroll_view->user_data)
    return;
  ScrollViewData *data = (ScrollViewData *)scroll_view->user_data;

  if (horizontal)
    *horizontal = data->show_h_scrollbar;
  if (vertical)
    *vertical = data->show_v_scrollbar;
}

void scroll_view_scroll_to(Widget *scroll_view, Vec2 position) {
  scroll_view_set_scroll_offset(scroll_view, position);
}

void scroll_view_scroll_by(Widget *scroll_view, Vec2 delta) {
  Vec2 current = scroll_view_get_scroll_offset(scroll_view);
  current.x += delta.x;
  current.y += delta.y;
  scroll_view_set_scroll_offset(scroll_view, current);
}

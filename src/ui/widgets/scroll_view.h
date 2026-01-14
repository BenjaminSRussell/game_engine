#ifndef UI_SCROLL_VIEW_H
#define UI_SCROLL_VIEW_H

#include "widget.h"
#include "include/math/math_all.h"

#ifdef __cplusplus
extern "C" {
#endif

// ScrollView specific creation
Widget* scroll_view_create(const char* name, Vec2 size);

// ScrollView API
void scroll_view_set_content(Widget* scroll_view, Widget* content);
Widget* scroll_view_get_content(const Widget* scroll_view);

void scroll_view_set_scroll_offset(Widget* scroll_view, Vec2 offset);
Vec2 scroll_view_get_scroll_offset(const Widget* scroll_view);

void scroll_view_set_show_scrollbars(Widget* scroll_view, bool horizontal, bool vertical);
void scroll_view_get_show_scrollbars(const Widget* scroll_view, bool* horizontal, bool* vertical);

// ScrollView scrolling
void scroll_view_scroll_to(Widget* scroll_view, Vec2 position);
void scroll_view_scroll_by(Widget* scroll_view, Vec2 delta);

#ifdef __cplusplus
}
#endif

#endif // UI_SCROLL_VIEW_H

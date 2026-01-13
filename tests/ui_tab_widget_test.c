#include "ui/widgets/tab_widget.h"

#include <stdio.h>

static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return g_failed;                                                         \
    }                                                                          \
  } while (0)

int run_ui_tab_widget_tests(void) {
  TabWidget *tabs = tab_widget_create("MainTabs");
  EXPECT(tabs != NULL, "tab widget create");

  Widget *page1 = widget_create("Page1");
  Widget *page2 = widget_create("Page2");
  EXPECT(page1 != NULL && page2 != NULL, "pages");

  tab_widget_add_tab(tabs, "Tab 1", page1);
  tab_widget_add_tab(tabs, "Tab 2", page2);

  EXPECT(tab_widget_get_active_tab(tabs) == 0, "first tab active");
  EXPECT(page1->visible == true, "page 1 visible");
  EXPECT(page2->visible == false, "page 2 hidden");

  tab_widget_set_active_tab(tabs, 1);
  EXPECT(tab_widget_get_active_tab(tabs) == 1, "second tab active");
  EXPECT(page1->visible == false, "page 1 hidden after switch");
  EXPECT(page2->visible == true, "page 2 visible after switch");

  widget_set_size((Widget *)tabs, (Vec2){800, 600});
  EXPECT(tabs->widget.layout != NULL, "tab layout callback");
  tabs->widget.layout((Widget *)tabs, 800, 600);

  Vec2 header_size = widget_get_size(tabs->header_container);
  EXPECT(header_size.y == 30.0f, "header height");
  EXPECT(header_size.x == 800.0f, "header width");

  Vec2 content_pos = widget_get_position(tabs->content_container);
  EXPECT(content_pos.y == 30.0f, "content below header");

  widget_destroy((Widget *)tabs);
  return g_failed;
}

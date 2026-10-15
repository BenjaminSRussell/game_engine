/* Proves ui_label draws its text through the installed graphics backend:
 * alignment, word wrap, max_lines with ellipsis, shadow order, color and
 * format. Issue #122. Compiles ui_label.c and widget.c directly with allocator
 * and logger stubs because add_engine_test links Engine, which does not build.
 */

#include "core/logging/unified_logger.h"
#include "include/core/memory.h"
#include "ui/widgets/ui_label.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags,
                           const char *file, int line, const char *function) {
  (void)strategy;
  (void)flags;
  (void)file;
  (void)line;
  (void)function;
  return calloc(1, size ? size : 1);
}

void *unified_memory_realloc(void *ptr, size_t new_size, MemoryFlags flags,
                             const char *file, int line, const char *function) {
  (void)flags;
  (void)file;
  (void)line;
  (void)function;
  return realloc(ptr, new_size);
}

void unified_memory_free(void *ptr, const char *file, int line,
                         const char *function) {
  (void)file;
  (void)line;
  (void)function;
  free(ptr);
}

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

static int g_failed = 0;

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

/* Records every run the label sends to the backend. */
#define MAX_DRAWS 32
#define MAX_TEXT 32

typedef struct {
  char text[MAX_TEXT];
  uint32_t length;
  Vec2 position;
  float font_size;
  Vec4 color;
  LabelTextFormat format;
} DrawCall;

static DrawCall g_draws[MAX_DRAWS];
static int g_draw_count = 0;
static int g_context_marker = 0;
static void *g_seen_context = NULL;

static void record_draw(void *context, const char *text, uint32_t length,
                        Vec2 position, float font_size, Vec4 color,
                        LabelTextFormat format) {
  g_seen_context = context;
  if (g_draw_count >= MAX_DRAWS) return;

  DrawCall *draw = &g_draws[g_draw_count++];
  uint32_t copied = length < (uint32_t)(MAX_TEXT - 1) ? length : (uint32_t)(MAX_TEXT - 1);
  memcpy(draw->text, text, copied);
  draw->text[copied] = '\0';
  draw->length = length;
  draw->position = position;
  draw->font_size = font_size;
  draw->color = color;
  draw->format = format;
}

static void start_recording(void) {
  g_draw_count = 0;
  g_seen_context = NULL;
  ui_label_set_text_backend(record_draw, &g_context_marker);
}

static bool close_to(float a, float b) { return a - b < 0.01f && b - a < 0.01f; }

static bool draw_matches(int index, const char *text, float x, float y) {
  if (index < 0 || index >= g_draw_count) return false;
  const DrawCall *draw = &g_draws[index];
  return strcmp(draw->text, text) == 0 && close_to(draw->position.x, x) &&
         close_to(draw->position.y, y);
}

static bool color_matches(Vec4 a, Vec4 b) {
  return close_to(a.x, b.x) && close_to(a.y, b.y) && close_to(a.z, b.z) &&
         close_to(a.w, b.w);
}

/* Font 10 gives 6 units per character and 14 units per line. The label sits at
 * (10, 20), so its content box starts there. */
static UILabel *make_label(const char *text, float width, float height) {
  UILabel *label = ui_label_create("label", text);
  widget_set_position(&label->base, (Vec2){10.0f, 20.0f});
  widget_set_size(&label->base, (Vec2){width, height});
  ui_label_set_font_size(label, 10.0f);
  ui_label_set_vertical_alignment(label, LABEL_VALIGN_TOP);
  return label;
}

static void render(UILabel *label) { label->base.render(&label->base); }

static void test_left_top_single_line(void) {
  UILabel *label = make_label("Hello", 200.0f, 20.0f);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 1, "one line should draw one run");
  EXPECT(draw_matches(0, "Hello", 10.0f, 20.0f),
         "left/top run should start at the label origin");
  EXPECT(g_draws[0].font_size == 10.0f, "font size should reach the backend");
  EXPECT(g_seen_context == &g_context_marker, "backend context should be passed back");
  ui_label_destroy(label);
}

static void test_center_alignment(void) {
  UILabel *label = make_label("Hello", 200.0f, 20.0f);
  ui_label_set_alignment(label, LABEL_ALIGN_CENTER);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 1, "centred line should draw one run");
  EXPECT(draw_matches(0, "Hello", 95.0f, 20.0f),
         "30 units of text centred in 200 units should start at x=95");
  ui_label_destroy(label);
}

static void test_right_alignment(void) {
  UILabel *label = make_label("Hello", 200.0f, 20.0f);
  ui_label_set_alignment(label, LABEL_ALIGN_RIGHT);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 1, "right-aligned line should draw one run");
  EXPECT(draw_matches(0, "Hello", 180.0f, 20.0f),
         "30 units of text right-aligned in 200 units should start at x=180");
  ui_label_destroy(label);
}

static void test_vertical_center(void) {
  UILabel *label = make_label("Hello", 200.0f, 20.0f);
  ui_label_set_vertical_alignment(label, LABEL_VALIGN_CENTER);
  start_recording();
  render(label);

  EXPECT(draw_matches(0, "Hello", 10.0f, 23.0f),
         "one 14-unit line centred in 20 units should start at y=23");
  ui_label_destroy(label);
}

static void test_vertical_bottom(void) {
  UILabel *label = make_label("Hello", 200.0f, 20.0f);
  ui_label_set_vertical_alignment(label, LABEL_VALIGN_BOTTOM);
  start_recording();
  render(label);

  EXPECT(draw_matches(0, "Hello", 10.0f, 26.0f),
         "one 14-unit line at the bottom of 20 units should start at y=26");
  ui_label_destroy(label);
}

static void test_newline_without_wrap(void) {
  UILabel *label = make_label("ab\ncd", 200.0f, 40.0f);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 2, "newline should draw two runs");
  EXPECT(draw_matches(0, "ab", 10.0f, 20.0f), "first line should be at the top");
  EXPECT(draw_matches(1, "cd", 10.0f, 34.0f),
         "second line should sit one line height (font + 4) lower");
  EXPECT(ui_label_get_line_count(label) == 2, "line count should match the drawn lines");
  ui_label_destroy(label);
}

static void test_word_wrap_breaks_at_fit_width(void) {
  /* Width 25 holds 4 characters at 6 units each. */
  UILabel *label = make_label("abcdefghij", 25.0f, 60.0f);
  ui_label_set_word_wrap(label, true);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 3, "10 characters at 4 per line should draw three lines");
  EXPECT(draw_matches(0, "abcd", 10.0f, 20.0f), "first wrapped line");
  EXPECT(draw_matches(1, "efgh", 10.0f, 34.0f), "second wrapped line");
  EXPECT(draw_matches(2, "ij", 10.0f, 48.0f), "last wrapped line");
  EXPECT(ui_label_get_line_count(label) == 3, "line count should match the wrapped lines");
  ui_label_destroy(label);
}

static void test_max_lines_with_ellipsis(void) {
  UILabel *label = make_label("abcdefghij", 25.0f, 60.0f);
  ui_label_set_word_wrap(label, true);
  ui_label_set_max_lines(label, 2);
  ui_label_set_ellipsis(label, true);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 3, "two visible lines plus one ellipsis run");
  EXPECT(draw_matches(0, "abcd", 10.0f, 20.0f), "first visible line should be whole");
  EXPECT(draw_matches(1, "e", 10.0f, 34.0f),
         "last visible line should keep room for the ellipsis");
  EXPECT(draw_matches(2, "...", 16.0f, 34.0f), "ellipsis should follow the kept text");
  EXPECT(ui_label_get_line_count(label) == 2, "line count should respect max_lines");
  ui_label_destroy(label);
}

static void test_ellipsis_clips_wide_line(void) {
  UILabel *label = make_label("abcdefghij", 25.0f, 20.0f);
  ui_label_set_ellipsis(label, true);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 2, "clipped line should draw kept text and an ellipsis");
  EXPECT(draw_matches(0, "a", 10.0f, 20.0f), "kept text should leave room for the ellipsis");
  EXPECT(draw_matches(1, "...", 16.0f, 20.0f), "ellipsis should follow the kept text");
  ui_label_destroy(label);
}

static void test_shadow_draws_before_text(void) {
  UILabel *label = make_label("Hi", 200.0f, 20.0f);
  Vec4 shadow = {0.0f, 0.0f, 0.0f, 0.5f};
  ui_label_set_shadow(label, 2.0f, 3.0f, shadow, true);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 2, "shadow and text should each draw once");
  EXPECT(draw_matches(0, "Hi", 12.0f, 23.0f), "shadow should be offset by (2, 3)");
  EXPECT(color_matches(g_draws[0].color, shadow), "shadow run should use the shadow color");
  EXPECT(draw_matches(1, "Hi", 10.0f, 20.0f), "text should draw last at the label origin");
  EXPECT(color_matches(g_draws[1].color, label->text_color),
         "text run should use the text color");
  ui_label_destroy(label);
}

static void test_shadow_runs_precede_all_text(void) {
  UILabel *label = make_label("a\nb", 200.0f, 40.0f);
  Vec4 shadow = {0.0f, 0.0f, 0.0f, 0.5f};
  ui_label_set_shadow(label, 1.0f, 1.0f, shadow, true);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 4, "two lines with a shadow should draw four runs");
  EXPECT(draw_matches(0, "a", 11.0f, 21.0f), "first shadow run");
  EXPECT(draw_matches(1, "b", 11.0f, 35.0f), "second shadow run");
  EXPECT(draw_matches(2, "a", 10.0f, 20.0f), "text runs should follow every shadow run");
  EXPECT(draw_matches(3, "b", 10.0f, 34.0f), "second text run");
  EXPECT(color_matches(g_draws[0].color, shadow) && color_matches(g_draws[1].color, shadow),
         "both shadow runs should use the shadow color");
  ui_label_destroy(label);
}

static void test_hidden_label_draws_nothing(void) {
  UILabel *label = make_label("Hello", 200.0f, 20.0f);
  widget_set_visible(&label->base, false);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 0, "hidden label should draw nothing");
  ui_label_destroy(label);
}

static void test_empty_text_draws_nothing(void) {
  UILabel *label = make_label("", 200.0f, 20.0f);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 0, "empty text should draw nothing");
  EXPECT(ui_label_get_line_count(label) == 1, "empty text is one empty line");
  ui_label_destroy(label);
}

static void test_missing_backend_draws_nothing(void) {
  UILabel *label = make_label("Hello", 200.0f, 20.0f);
  start_recording();
  ui_label_set_text_backend(NULL, NULL);
  render(label);

  EXPECT(g_draw_count == 0, "label without a backend should draw nothing and not crash");
  ui_label_destroy(label);
}

static void test_color_and_format_reach_backend(void) {
  UILabel *label = make_label("Hi", 200.0f, 20.0f);
  Vec4 color = {0.25f, 0.5f, 0.75f, 1.0f};
  ui_label_set_text_color(label, color);
  ui_label_set_format(label, LABEL_FORMAT_BOLD);
  start_recording();
  render(label);

  EXPECT(g_draw_count == 1, "one line should draw one run");
  EXPECT(color_matches(g_draws[0].color, color), "text color should reach the backend");
  EXPECT(g_draws[0].format == LABEL_FORMAT_BOLD, "format should reach the backend");
  ui_label_destroy(label);
}

int main(void) {
  test_left_top_single_line();
  test_center_alignment();
  test_right_alignment();
  test_vertical_center();
  test_vertical_bottom();
  test_newline_without_wrap();
  test_word_wrap_breaks_at_fit_width();
  test_max_lines_with_ellipsis();
  test_ellipsis_clips_wide_line();
  test_shadow_draws_before_text();
  test_shadow_runs_precede_all_text();
  test_hidden_label_draws_nothing();
  test_empty_text_draws_nothing();
  test_missing_backend_draws_nothing();
  test_color_and_format_reach_backend();

  if (g_failed) {
    fprintf(stderr, "%d ui_label test(s) failed\n", g_failed);
    return 1;
  }
  printf("ui_label render tests passed\n");
  return 0;
}

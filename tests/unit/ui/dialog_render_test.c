/* Dialog shell rendering through the registered draw backend.
 * Issue #123. Compiles dialog.c with its widget, button, label, text-field and
 * modal collaborators, and stubs the allocator and logger, because
 * add_engine_test links Engine and a framework file that is not in the tree.
 */

#include "core/logging/unified_logger.h"
#include "include/core/memory.h"
#include "ui/widgets/dialog.h"

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

/* Records every draw call the dialog shell makes, in paint order. */

#define MAX_CALLS 16

typedef struct {
  int is_text;
  Rect rect;      /* fill calls */
  float radius;   /* fill calls */
  Vec2 anchor;    /* text calls */
  char text[128]; /* text calls */
  Vec4 color;
} DrawCall;

static DrawCall g_calls[MAX_CALLS];
static int g_call_count;
static int g_failed;

static void record_fill(void *user_data, Rect rect, float radius, Vec4 color) {
  (void)user_data;
  if (g_call_count < MAX_CALLS) {
    DrawCall *call = &g_calls[g_call_count];
    memset(call, 0, sizeof *call);
    call->rect = rect;
    call->radius = radius;
    call->color = color;
  }
  g_call_count++;
}

static void record_text(void *user_data, const char *text, Vec2 position, Vec4 color) {
  (void)user_data;
  if (g_call_count < MAX_CALLS) {
    DrawCall *call = &g_calls[g_call_count];
    memset(call, 0, sizeof *call);
    call->is_text = 1;
    call->anchor = position;
    call->color = color;
    snprintf(call->text, sizeof call->text, "%s", text);
  }
  g_call_count++;
}

static const DialogDrawBackend kRecorder = {record_fill, record_text, NULL};

#define EXPECT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);            \
      g_failed++;                                                              \
      return;                                                                  \
    }                                                                          \
  } while (0)

static int rect_eq(Rect a, Rect b) { return memcmp(&a, &b, sizeof a) == 0; }
static int vec2_eq(Vec2 a, Vec2 b) { return memcmp(&a, &b, sizeof a) == 0; }
static int vec4_eq(Vec4 a, Vec4 b) { return memcmp(&a, &b, sizeof a) == 0; }

/* Dialog at (100, 50). Alert type: non-modal, so no modal stack is touched. */
static UIDialog *make_dialog(const char *title, const char *message) {
  UIDialog *dialog = dialog_create("render_test", title, message, DIALOG_TYPE_ALERT);
  widget_set_position(&dialog->base, (Vec2){100.0f, 50.0f});
  widget_set_visible(&dialog->base, true);
  return dialog;
}

static void render(UIDialog *dialog) {
  g_call_count = 0;
  dialog->base.render(&dialog->base);
}

static void test_full_shell_draws_in_paint_order(void) {
  dialog_set_draw_backend(NULL);
  UIDialog *dialog = make_dialog("Title", "Body");
  dialog_set_content_bg_color(dialog, (Vec4){{0.5f, 0.5f, 0.5f, 1.0f}});
  dialog_set_title_bg_color(dialog, (Vec4){{0.75f, 0.25f, 0.25f, 1.0f}});
  dialog_set_corner_radius(dialog, 6.0f);
  dialog_set_shadow(dialog, 4.0f, (Vec4){{0.0f, 0.0f, 0.0f, 0.5f}});
  dialog_set_draw_backend(&kRecorder);

  render(dialog);

  EXPECT(g_call_count == 4, "shell should draw shadow, body, title bar and title text");

  EXPECT(!g_calls[0].is_text, "first call should be a fill (shadow)");
  EXPECT(rect_eq(g_calls[0].rect, (Rect){104.0f, 54.0f, 400.0f, 300.0f}),
         "shadow should be the body offset by shadow_offset");
  EXPECT(g_calls[0].radius == 6.0f, "shadow should use the corner radius");
  EXPECT(vec4_eq(g_calls[0].color, (Vec4){{0.0f, 0.0f, 0.0f, 0.5f}}),
         "shadow should use shadow_color");

  EXPECT(!g_calls[1].is_text, "second call should be a fill (body)");
  EXPECT(rect_eq(g_calls[1].rect, (Rect){100.0f, 50.0f, 400.0f, 300.0f}),
         "body should cover the widget bounds");
  EXPECT(g_calls[1].radius == 6.0f, "body should use the corner radius");
  EXPECT(vec4_eq(g_calls[1].color, (Vec4){{0.5f, 0.5f, 0.5f, 1.0f}}),
         "body should use content_bg_color");

  EXPECT(!g_calls[2].is_text, "third call should be a fill (title bar)");
  EXPECT(rect_eq(g_calls[2].rect, (Rect){100.0f, 50.0f, 400.0f, 30.0f}),
         "title bar should span the top edge at the title bar height");
  EXPECT(g_calls[2].radius == 0.0f, "title bar should be square");
  EXPECT(vec4_eq(g_calls[2].color, (Vec4){{0.75f, 0.25f, 0.25f, 1.0f}}),
         "title bar should use title_bg_color");

  EXPECT(g_calls[3].is_text, "fourth call should be text (title)");
  EXPECT(strcmp(g_calls[3].text, "Title") == 0, "title text should be the dialog title");
  EXPECT(vec2_eq(g_calls[3].anchor, (Vec2){108.0f, 65.0f}),
         "title text should sit at the left inset, vertically centred in the bar");
  EXPECT(vec4_eq(g_calls[3].color, (Vec4){{1.0f, 1.0f, 1.0f, 1.0f}}),
         "title text should use the widget text color");

  dialog_destroy(dialog);
  dialog_set_draw_backend(NULL);
}

static void test_zero_shadow_offset_skips_shadow(void) {
  dialog_set_draw_backend(NULL);
  UIDialog *dialog = make_dialog("Title", "Body");
  dialog_set_shadow(dialog, 0.0f, (Vec4){{0.0f, 0.0f, 0.0f, 0.5f}});
  dialog_set_draw_backend(&kRecorder);

  render(dialog);

  EXPECT(g_call_count == 3, "zero shadow offset should skip the shadow");
  EXPECT(rect_eq(g_calls[0].rect, (Rect){100.0f, 50.0f, 400.0f, 300.0f}),
         "first call should be the body");

  dialog_destroy(dialog);
  dialog_set_draw_backend(NULL);
}

static void test_empty_title_skips_text(void) {
  dialog_set_draw_backend(NULL);
  UIDialog *dialog = make_dialog("Title", "Body");
  dialog_set_messages(dialog, "", NULL);
  dialog_set_draw_backend(&kRecorder);

  render(dialog);

  EXPECT(g_call_count == 3, "empty title should draw only the shell rectangles");
  for (int i = 0; i < g_call_count && i < MAX_CALLS; i++) {
    EXPECT(!g_calls[i].is_text, "no text call should be made for an empty title");
  }

  dialog_destroy(dialog);
  dialog_set_draw_backend(NULL);
}

static void test_hidden_dialog_draws_nothing(void) {
  dialog_set_draw_backend(NULL);
  UIDialog *dialog = make_dialog("Title", "Body");
  dialog_set_draw_backend(&kRecorder);
  widget_set_visible(&dialog->base, false);

  render(dialog);

  EXPECT(g_call_count == 0, "hidden dialog should not draw");

  dialog_destroy(dialog);
  dialog_set_draw_backend(NULL);
}

static void test_unregistered_backend_draws_nothing(void) {
  dialog_set_draw_backend(&kRecorder);
  dialog_set_draw_backend(NULL);
  UIDialog *dialog = make_dialog("Title", "Body");

  render(dialog);

  EXPECT(g_call_count == 0, "dialog should not draw once the backend is unregistered");

  dialog_destroy(dialog);
}

static void test_incomplete_backend_is_rejected(void) {
  dialog_set_draw_backend(NULL);
  const DialogDrawBackend no_text = {record_fill, NULL, NULL};
  dialog_set_draw_backend(&no_text);
  UIDialog *dialog = make_dialog("Title", "Body");

  render(dialog);

  EXPECT(g_call_count == 0, "backend without draw_text should be treated as unregistered");

  dialog_destroy(dialog);
  dialog_set_draw_backend(NULL);
}

static void test_backend_is_copied_on_registration(void) {
  dialog_set_draw_backend(NULL);
  DialogDrawBackend local = kRecorder;
  dialog_set_draw_backend(&local);
  memset(&local, 0, sizeof local);
  UIDialog *dialog = make_dialog("Title", "Body");

  render(dialog);

  EXPECT(g_call_count == 4, "registered backend should survive later changes to the caller's struct");

  dialog_destroy(dialog);
  dialog_set_draw_backend(NULL);
}

static void run(const char *name, void (*test)(void)) {
  int failed_before = g_failed;
  test();
  printf("%s - %s\n", g_failed == failed_before ? "ok" : "not ok", name);
}

int main(void) {
  run("full shell draws in paint order", test_full_shell_draws_in_paint_order);
  run("zero shadow offset skips shadow", test_zero_shadow_offset_skips_shadow);
  run("empty title skips text", test_empty_title_skips_text);
  run("hidden dialog draws nothing", test_hidden_dialog_draws_nothing);
  run("unregistered backend draws nothing", test_unregistered_backend_draws_nothing);
  run("incomplete backend is rejected", test_incomplete_backend_is_rejected);
  run("backend is copied on registration", test_backend_is_copied_on_registration);

  if (g_failed) {
    fprintf(stderr, "%d dialog render check(s) failed\n", g_failed);
    return 1;
  }
  printf("dialog render tests passed\n");
  return 0;
}

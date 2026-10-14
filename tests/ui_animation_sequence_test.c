/* Chained UI animations and zoom transitions.
 * Not an Engine-linked test: Engine does not build here (missing GLFW, then
 * gpu_profiler.c). Memory and logger symbols are stubbed, same as
 * test_ui_animation_features.
 */

#include "core/logging/unified_logger.h"
#include "include/core/memory.h"
#include "ui/ui_animation.h"
#include "ui/widgets/widget.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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
    }                                                                          \
  } while (0)

static int near(float a, float b) { return fabsf(a - b) < 0.02f; }

static void test_finished_animation_starts_chain(void) {
  Widget *widget = widget_create("seq");
  UIAnimation *first = ui_animation_create(widget, UI_ANIM_OPACITY, 1.0f);
  UIAnimation *second = ui_animation_create(widget, UI_ANIM_OPACITY, 1.0f);
  EXPECT(first != NULL && second != NULL, "chain allocations");

  ui_animation_set_float(first, 0.0f, 10.0f);
  ui_animation_set_easing(first, UI_EASE_LINEAR);
  ui_animation_set_float(second, 10.0f, 40.0f);
  ui_animation_set_easing(second, UI_EASE_LINEAR);
  ui_animation_chain(first, second);

  EXPECT(!second->is_playing, "chained animation waits until the first finishes");
  ui_animation_play(first);

  ui_animation_update(0.5f);
  EXPECT(first->is_playing && !first->is_finished, "first still running");
  EXPECT(!second->is_playing, "next stays idle while the first plays");
  EXPECT(near(widget->opacity, 5.0f), "first tween halfway");

  ui_animation_update(0.5f);
  EXPECT(first->is_finished && !first->is_playing, "first finished");
  EXPECT(second->is_playing && !second->is_finished,
         "finished animation starts its chained next animation");
  EXPECT(first->next_in_sequence == NULL, "chain hands off ownership once");
  EXPECT(near(widget->opacity, 10.0f), "opacity is the first animation's end value");

  ui_animation_update(0.5f);
  EXPECT(second->is_playing, "chained animation keeps playing");
  EXPECT(near(widget->opacity, 25.0f), "chained animation advances on the next tick");

  ui_animation_shutdown();
  widget_destroy(widget);
}

static void test_zoom_parallel_size_and_position(void) {
  Widget *current = widget_create("cur");
  Widget *next = widget_create("nxt");
  widget_set_size(next, (Vec2){100.0f, 80.0f});
  widget_set_position(next, (Vec2){10.0f, 20.0f});

  ui_transition_screen(current, next, UI_TRANSITION_ZOOM, 1.0f);

  Vec2 size = widget_get_size(next);
  Vec2 pos = widget_get_position(next);
  EXPECT(near(size.x, 50.0f) && near(size.y, 40.0f), "zoom starts at half size");
  EXPECT(near(pos.x, 35.0f) && near(pos.y, 40.0f), "zoom starts centered on the final rect");
  EXPECT(near(next->opacity, 0.0f), "zoom starts transparent");

  ui_animation_update(0.5f);
  size = widget_get_size(next);
  pos = widget_get_position(next);
  EXPECT(size.x > 50.0f && size.x < 100.0f, "zoom size animates with the fade");
  EXPECT(pos.x < 35.0f && pos.x > 10.0f, "zoom position animates with the size");
  EXPECT(next->opacity > 0.0f && next->opacity < 1.0f, "zoom fade runs in parallel");

  ui_animation_update(0.5f);
  size = widget_get_size(next);
  pos = widget_get_position(next);
  EXPECT(near(size.x, 100.0f) && near(size.y, 80.0f), "zoom ends at full size");
  EXPECT(near(pos.x, 10.0f) && near(pos.y, 20.0f), "zoom ends at the final position");
  EXPECT(near(next->opacity, 1.0f), "zoom ends opaque");

  ui_animation_shutdown();
  widget_destroy(current);
  widget_destroy(next);
}

int main(void) {
  ui_animation_init();
  test_finished_animation_starts_chain();
  ui_animation_init();
  test_zoom_parallel_size_and_position();

  if (g_failed) {
    fprintf(stderr, "%d UI animation sequence check(s) failed\n", g_failed);
    return 1;
  }
  printf("UI animation sequencing and zoom checks passed\n");
  return 0;
}

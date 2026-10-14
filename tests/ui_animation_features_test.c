/* Keyframe, tween, and cubic-bezier UI animation.
 * Not an Engine-linked test: Engine does not build here (missing GLFW, then
 * gpu_profiler.c). Memory and logger symbols are stubbed, same as
 * test_widget_focus.
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

static void test_linear_tween(void) {
  Widget *widget = widget_create("fade");
  UIAnimation *anim = ui_animation_create(widget, UI_ANIM_OPACITY, 1.0f);
  EXPECT(anim != NULL, "tween allocation");
  ui_animation_set_float(anim, 0.0f, 100.0f);
  ui_animation_set_easing(anim, UI_EASE_LINEAR);
  EXPECT(anim->mode == UI_ANIM_MODE_TWEEN, "set_float selects tween mode");
  ui_animation_play(anim);
  ui_animation_update(0.5f);
  EXPECT(near(widget->opacity, 50.0f), "linear tween at halfway");
  ui_animation_update(0.5f);
  EXPECT(near(widget->opacity, 100.0f), "linear tween at end");
  ui_animation_shutdown();
  widget_destroy(widget);
}

static void test_cubic_bezier(void) {
  Widget *widget = widget_create("bezier");
  UIAnimation *anim = ui_animation_create(widget, UI_ANIM_OPACITY, 1.0f);
  ui_animation_set_float(anim, 0.0f, 1.0f);
  /* x = t^3-like late rise, y = t^3, so t=0.5 maps to about 0.125. */
  ui_animation_set_bezier_curve(anim, 0.0f, 0.0f, 1.0f, 0.0f);
  EXPECT(anim->easing == UI_EASE_CUSTOM_BEZIER, "bezier setter selects custom ease");
  ui_animation_play(anim);
  ui_animation_update(0.0f);
  EXPECT(near(widget->opacity, 0.0f), "bezier start");
  ui_animation_update(0.5f);
  EXPECT(near(widget->opacity, 0.125f), "bezier midpoint stays low");
  ui_animation_update(0.5f);
  EXPECT(near(widget->opacity, 1.0f), "bezier end");
  ui_animation_shutdown();
  widget_destroy(widget);
}

static void test_keyframes_sorted(void) {
  Widget *widget = widget_create("keys");
  UIAnimation *anim = ui_animation_create(widget, UI_ANIM_OPACITY, 1.0f);
  ui_animation_add_keyframe_float(anim, 1.0f, 80.0f, UI_EASE_LINEAR);
  ui_animation_add_keyframe_float(anim, 0.0f, 0.0f, UI_EASE_LINEAR);
  EXPECT(anim->mode == UI_ANIM_MODE_KEYFRAME, "keyframe mode");
  EXPECT(anim->keyframe_count == 2, "two keyframes");
  EXPECT(anim->keyframes[0].time == 0.0f && anim->keyframes[1].time == 1.0f,
         "keyframes sorted by time");
  ui_animation_play(anim);
  ui_animation_update(0.25f);
  EXPECT(near(widget->opacity, 20.0f), "keyframe quarter");
  ui_animation_shutdown();
  widget_destroy(widget);
}

static void test_keyframe_bezier_segment(void) {
  Widget *widget = widget_create("key-bezier");
  UIAnimation *anim = ui_animation_create(widget, UI_ANIM_OPACITY, 1.0f);
  ui_animation_set_bezier_curve(anim, 0.0f, 0.0f, 1.0f, 0.0f);
  ui_animation_add_keyframe_float(anim, 0.0f, 0.0f, UI_EASE_LINEAR);
  ui_animation_add_keyframe_float(anim, 1.0f, 1.0f, UI_EASE_CUSTOM_BEZIER);
  ui_animation_play(anim);
  ui_animation_update(0.5f);
  EXPECT(near(widget->opacity, 0.125f), "keyframe segment uses cubic bezier");
  ui_animation_shutdown();
  widget_destroy(widget);
}

static void test_property_binding(void) {
  float bound = -1.0f;
  Vec2 bound_v = {0.0f, 0.0f};
  Vec4 bound_c = {0.0f, 0.0f, 0.0f, 0.0f};
  UIAnimation *anim = ui_animation_create(NULL, UI_ANIM_OPACITY, 1.0f);
  ui_animation_set_float(anim, 2.0f, 6.0f);
  ui_animation_bind_float(anim, &bound);
  ui_animation_set_easing(anim, UI_EASE_LINEAR);
  ui_animation_play(anim);
  ui_animation_update(0.5f);
  EXPECT(near(bound, 4.0f), "bound float halfway");

  UIAnimation *anim_v = ui_animation_create(NULL, UI_ANIM_POSITION, 1.0f);
  ui_animation_set_vec2(anim_v, (Vec2){0.0f, 0.0f}, (Vec2){10.0f, 20.0f});
  ui_animation_bind_vec2(anim_v, &bound_v);
  ui_animation_set_easing(anim_v, UI_EASE_LINEAR);
  ui_animation_play(anim_v);

  UIAnimation *anim_c = ui_animation_create(NULL, UI_ANIM_BACKGROUND_COLOR, 1.0f);
  ui_animation_set_vec4(anim_c, (Vec4){0.0f, 0.0f, 0.0f, 0.0f},
                        (Vec4){1.0f, 2.0f, 3.0f, 4.0f});
  ui_animation_bind_vec4(anim_c, &bound_c);
  ui_animation_set_easing(anim_c, UI_EASE_LINEAR);
  ui_animation_play(anim_c);
  ui_animation_update(0.5f);
  EXPECT(near(bound_v.x, 5.0f) && near(bound_v.y, 10.0f), "bound vec2 halfway");
  EXPECT(near(bound_c.x, 0.5f) && near(bound_c.y, 1.0f) && near(bound_c.z, 1.5f) &&
             near(bound_c.w, 2.0f),
         "bound vec4 halfway");
  ui_animation_shutdown();
}

int main(void) {
  ui_animation_init();
  test_linear_tween();
  test_cubic_bezier();
  test_keyframes_sorted();
  test_keyframe_bezier_segment();
  test_property_binding();
  ui_animation_shutdown();

  if (g_failed) {
    fprintf(stderr, "%d UI animation feature check(s) failed\n", g_failed);
    return 1;
  }
  printf("UI animation keyframe, tween, and bezier checks passed\n");
  return 0;
}

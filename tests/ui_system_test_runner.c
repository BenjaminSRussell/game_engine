/* TabWidget, UI renderer batching/clipping, and UI atlas packing.
 * Not an Engine-linked test: Engine does not build here (missing GLFW, then
 * gpu_profiler.c). Memory and logger symbols are stubbed, same as
 * test_widget_focus. Renderer headers and widget headers both define Rect,
 * so those cases live in separate translation units.
 */

#include "core/logging/unified_logger.h"
#include "include/core/memory.h"

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

int run_ui_atlas_renderer_tests(void);
int run_ui_tab_widget_tests(void);

int main(void) {
  int failed = 0;

  failed += run_ui_atlas_renderer_tests();
  failed += run_ui_tab_widget_tests();

  if (failed) {
    fprintf(stderr, "%d UI system test group(s) failed\n", failed);
    return 1;
  }
  printf("All UI system tests passed\n");
  return 0;
}

#include <core/hot_reload.h>

#include <stdio.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format,
                        ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

HotReloadModule *hot_reload_register_module(const char *module_name,
                                            const char *library_path);

static int g_failures = 0;

static void expect(int cond, const char *msg) {
  if (!cond) {
    fprintf(stderr, "FAIL: %s\n", msg);
    g_failures++;
  }
}

static int file_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return 0;
  fclose(f);
  return 1;
}

/* 1 if an uncommented line contains needle, 0 if not, -1 if the file is missing. */
static int uncommented_lists(const char *cmake_path, const char *needle) {
  FILE *f = fopen(cmake_path, "r");
  char line[2048];
  int found = 0;

  if (!f) {
    fprintf(stderr, "FAIL: cannot open %s\n", cmake_path);
    return -1;
  }

  while (fgets(line, sizeof line, f)) {
    const char *p = line;
    while (*p == ' ' || *p == '\t')
      p++;
    if (*p == '#' || *p == '\n' || *p == '\0')
      continue;
    if (strstr(p, needle))
      found = 1;
  }

  fclose(f);
  return found;
}

int main(void) {
  char legacy[1024];
  char real_src[1024];
  char sources[1024];
  HotReloadConfig cfg;
  int listed;

  snprintf(legacy, sizeof legacy, "%s/src/engine/core/legacy/hot_reload.c",
           ENGINE_SOURCE_DIR);
  snprintf(real_src, sizeof real_src, "%s/src/engine/core/hot_reload.c",
           ENGINE_SOURCE_DIR);
  snprintf(sources, sizeof sources, "%s/cmake/sources.cmake", ENGINE_SOURCE_DIR);

  expect(!file_exists(legacy), "legacy hot_reload.c stub should be removed");
  expect(file_exists(real_src), "real hot_reload.c should remain");

  listed = uncommented_lists(sources, "src/engine/core/legacy/hot_reload.c");
  expect(listed == 0, "ENGINE_SOURCES must not compile the legacy stub");
  listed = uncommented_lists(sources, "src/engine/core/hot_reload.c");
  expect(listed == 1, "ENGINE_SOURCES must keep the real hot_reload.c");

  memset(&cfg, 0, sizeof cfg);
  cfg.watch_path = ".";
  expect(hot_reload_init(cfg), "hot_reload_init");
  hot_reload_update();
  expect(hot_reload_register_module("missing",
                                    "no-such-hot-reload-module.so") == NULL,
         "missing module must not load");
  hot_reload_shutdown();

  memset(&cfg, 0, sizeof cfg);
  expect(hot_reload_init(cfg), "hot_reload_init after shutdown");
  hot_reload_shutdown();

  if (g_failures) {
    fprintf(stderr, "%d failure(s)\n", g_failures);
    return 1;
  }

  printf("test_hot_reload ok\n");
  return 0;
}

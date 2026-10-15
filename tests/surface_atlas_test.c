/*
 * surface_atlas_test.c
 * Shelf placement in the Lumen surface atlas. Compiles surface_atlas.c
 * directly and does not link Engine.
 */

#include "include/core/logger.h"
#include "lighting/lightmaps/lightmap_packer.h"
#include "lighting/lumen/surface_cache/surface_atlas.h"

#include <stdio.h>
#include <stdlib.h>

static int g_packer_inits;
static int g_packer_shutdowns;

int lighting_lightmap_packer_init(void) {
  g_packer_inits++;
  return 0;
}

void lighting_lightmap_packer_shutdown(void) { g_packer_shutdowns++; }

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

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static void expect_init(uint32_t size, const char *label) {
  if (surface_atlas_init(size) != 0) {
    fprintf(stderr, "FAIL: %s init\n", label);
    exit(1);
  }
  printf("PASS: %s\n", label);
}

static void expect_reject(uint32_t w, uint32_t h, const char *label) {
  uint32_t x = 0xFFFFFFFFu;
  uint32_t y = 0xFFFFFFFEu;

  if (surface_atlas_allocate(w, h, &x, &y)) {
    fprintf(stderr, "FAIL: %s allocated\n", label);
    exit(1);
  }
  if (x != 0xFFFFFFFFu || y != 0xFFFFFFFEu) {
    fprintf(stderr, "FAIL: %s wrote %u,%u\n", label, x, y);
    exit(1);
  }
  printf("PASS: %s\n", label);
}

static void expect_at(uint32_t w, uint32_t h, uint32_t expect_x,
                      uint32_t expect_y, const char *label) {
  uint32_t x = 7;
  uint32_t y = 9;

  if (!surface_atlas_allocate(w, h, &x, &y)) {
    fprintf(stderr, "FAIL: %s rejected\n", label);
    exit(1);
  }
  if (x != expect_x || y != expect_y) {
    fprintf(stderr, "FAIL: %s at %u,%u expected %u,%u\n", label, x, y, expect_x,
            expect_y);
    exit(1);
  }
  printf("PASS: %s\n", label);
}

int main(void) {
  int inits_after_first;

  expect_reject(1, 1, "allocate before init");
  surface_atlas_shutdown();
  if (g_packer_inits != 0 || g_packer_shutdowns != 0) {
    fail("packer touched before init");
  }

  expect_init(64, "init 64");
  inits_after_first = g_packer_inits;
  if (inits_after_first != 1) {
    fail("packer init count");
  }
  expect_init(8, "second init keeps the first size");
  if (g_packer_inits != inits_after_first) {
    fail("second init called packer");
  }

  expect_reject(0, 1, "zero width");
  expect_reject(1, 0, "zero height");
  expect_reject(65, 1, "wider than atlas");
  expect_reject(1, 65, "taller than atlas");
  /* Still the 64 atlas: a 32-wide rect would not fit an 8 atlas. */
  expect_at(32, 16, 0, 0, "first shelf rect");
  expect_at(32, 16, 32, 0, "same row");
  expect_at(16, 32, 0, 16, "next row");
  expect_at(16, 8, 16, 16, "beside taller rect");
  expect_at(48, 8, 0, 48, "row after tall rect");
  expect_at(64, 8, 0, 56, "exact bottom edge");
  expect_reject(1, 1, "atlas full");

  surface_atlas_shutdown();
  if (g_packer_shutdowns != 1) {
    fail("packer shutdown count");
  }
  expect_reject(1, 1, "allocate after shutdown");
  surface_atlas_shutdown();
  if (g_packer_shutdowns != 1) {
    fail("second shutdown called packer");
  }

  expect_init(32, "re-init");
  if (g_packer_inits != 2) {
    fail("re-init packer count");
  }
  expect_at(24, 24, 0, 0, "fresh origin");
  expect_reject(16, 16, "reject does not consume space");
  expect_at(8, 8, 24, 0, "space remains after reject");

  surface_atlas_shutdown();
  expect_init(16, "null outputs");
  if (!surface_atlas_allocate(4, 4, NULL, NULL)) {
    fail("null outputs rejected");
  }
  expect_at(4, 4, 4, 0, "null outputs still advance the cursor");
  surface_atlas_shutdown();

  printf("PASS: surface atlas shelf packing\n");
  return 0;
}

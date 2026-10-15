/* GLSL must not be returned as SPIR-V for the Vulkan block pipeline.
 * Compiles vulkan_pipeline.c directly. Does not link Engine.
 */

#include "core/logging/unified_logger.h"
#include "include/core/resource/vfs/vfs.h"

#include <stdio.h>
#include <stdlib.h>
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

u32 *vulkan_read_shader_file(VFS *vfs, const char *path, u32 *size);
bool vulkan_load_block_pipeline_shaders(VFS *vfs, u32 **vert_code, u32 *vert_size,
                                        u32 **frag_code, u32 *frag_size);

struct VFSFile {
  const unsigned char *data;
  u64 size;
};

typedef struct Fixture {
  const char *path;
  const unsigned char *data;
  u64 size;
} Fixture;

static Fixture g_fixtures[8];
static int g_fixture_count;

static void reset_fixtures(void) { g_fixture_count = 0; }

static void add_fixture(const char *path, const void *data, u64 size) {
  if (g_fixture_count >= (int)(sizeof(g_fixtures) / sizeof(g_fixtures[0]))) {
    fprintf(stderr, "FAIL: fixture table full\n");
    exit(1);
  }
  g_fixtures[g_fixture_count].path = path;
  g_fixtures[g_fixture_count].data = (const unsigned char *)data;
  g_fixtures[g_fixture_count].size = size;
  g_fixture_count++;
}

VFSFile *vfs_open(VFS *vfs, const char *path, VFSMode mode) {
  int i;
  (void)vfs;
  (void)mode;
  for (i = 0; i < g_fixture_count; i++) {
    if (strcmp(g_fixtures[i].path, path) == 0) {
      VFSFile *file = (VFSFile *)malloc(sizeof(*file));
      if (!file) {
        return NULL;
      }
      file->data = g_fixtures[i].data;
      file->size = g_fixtures[i].size;
      return file;
    }
  }
  return NULL;
}

void vfs_close(VFSFile *file) { free(file); }

u64 vfs_size(VFSFile *file) { return file ? file->size : 0; }

u64 vfs_read(VFSFile *file, void *buffer, u64 size) {
  if (!file || !buffer || size != file->size) {
    return 0;
  }
  memcpy(buffer, file->data, file->size);
  return file->size;
}

static const unsigned char k_spirv[20] = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* 24 bytes, 4-byte aligned, so rejection depends on the magic, not length. */
static const unsigned char k_glsl[24] = {
    '#', 'v', 'e', 'r', 's', 'i', 'o', 'n', ' ', '4', '5', '0',
    '\n', 'v', 'o', 'i', 'd', ' ', 'm', 'a', 'i', 'n', '(', ')'};

static const unsigned char k_short_glsl[] = "#version 450\n";

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

static void expect_rejected(VFS *vfs, const char *path, const char *label) {
  u32 size = 99;
  u32 *code = vulkan_read_shader_file(vfs, path, &size);
  if (code != NULL || size != 0) {
    free(code);
    fprintf(stderr, "FAIL: %s was accepted as SPIR-V\n", label);
    exit(1);
  }
}

int main(void) {
  VFS vfs;
  u32 *code;
  u32 size;
  u32 *vert = (u32 *)1;
  u32 *frag = (u32 *)1;
  u32 vert_size = 7;
  u32 frag_size = 7;
  const unsigned char *bytes;

  memset(&vfs, 0, sizeof(vfs));

  if (vulkan_read_shader_file(NULL, "x", &size) != NULL) {
    fail("null vfs");
  }
  if (vulkan_load_block_pipeline_shaders(NULL, &vert, &vert_size, &frag,
                                         &frag_size)) {
    fail("null vfs load");
  }

  reset_fixtures();
  add_fixture("assets/shaders/block_vertex_simple.glsl", k_glsl, sizeof(k_glsl));
  add_fixture("assets/shaders/block_fragment_simple.glsl", k_glsl,
              sizeof(k_glsl));
  add_fixture("assets/shaders/short.glsl", k_short_glsl,
              sizeof(k_short_glsl) - 1);
  expect_rejected(&vfs, "assets/shaders/block_vertex_simple.glsl",
                  "vertex GLSL");
  expect_rejected(&vfs, "assets/shaders/short.glsl", "short GLSL");

  reset_fixtures();
  add_fixture("assets/shaders/compiled/block_vertex.spv", k_glsl,
              sizeof(k_glsl));
  expect_rejected(&vfs, "assets/shaders/compiled/block_vertex.spv",
                  "GLSL renamed to .spv");

  vert = (u32 *)1;
  frag = (u32 *)1;
  vert_size = 7;
  frag_size = 7;
  reset_fixtures();
  add_fixture("assets/shaders/block_vertex_simple.glsl", k_glsl, sizeof(k_glsl));
  add_fixture("assets/shaders/block_fragment_simple.glsl", k_glsl,
              sizeof(k_glsl));
  if (vulkan_load_block_pipeline_shaders(&vfs, &vert, &vert_size, &frag,
                                         &frag_size)) {
    fail("GLSL fallback succeeded");
  }
  if (vert != NULL || frag != NULL || vert_size != 0 || frag_size != 0) {
    fail("GLSL fallback leaked shader bytes");
  }

  reset_fixtures();
  add_fixture("assets/shaders/compiled/block_vertex.spv", k_spirv,
              sizeof(k_spirv));
  add_fixture("assets/shaders/compiled/block_fragment.spv", k_spirv,
              sizeof(k_spirv));
  add_fixture("assets/shaders/block_vertex_simple.spv", k_spirv,
              sizeof(k_spirv));
  code = vulkan_read_shader_file(&vfs, "assets/shaders/block_vertex_simple.glsl",
                                 &size);
  if (!code || size != sizeof(k_spirv)) {
    fail("compiled .spv sibling was ignored");
  }
  bytes = (const unsigned char *)code;
  if (bytes[0] != 0x03 || bytes[1] != 0x02 || bytes[2] != 0x23 ||
      bytes[3] != 0x07) {
    fail("sibling payload is not SPIR-V");
  }
  free(code);

  if (!vulkan_load_block_pipeline_shaders(&vfs, &vert, &vert_size, &frag,
                                          &frag_size)) {
    fail("real SPIR-V was rejected");
  }
  if (!vert || !frag || vert_size != sizeof(k_spirv) ||
      frag_size != sizeof(k_spirv)) {
    fail("SPIR-V load size");
  }
  bytes = (const unsigned char *)vert;
  if (memcmp(bytes, k_spirv, sizeof(k_spirv)) != 0) {
    fail("vertex SPIR-V bytes");
  }
  bytes = (const unsigned char *)frag;
  if (memcmp(bytes, k_spirv, sizeof(k_spirv)) != 0) {
    fail("fragment SPIR-V bytes");
  }
  free(vert);
  free(frag);

  printf("ok\n");
  return 0;
}

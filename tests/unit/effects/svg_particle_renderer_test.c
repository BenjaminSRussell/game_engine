/* Unit test for svg_particle_renderer.c. Issue #333.
 *
 * The renderer is included directly so its static rasterization helpers can be
 * checked against buffers this test owns. svg_particle_system.c is compiled into
 * the same executable for emitter creation and particle access, and
 * unified_logger_log is stubbed because the Engine library does not build on
 * origin/main.
 *
 * svg_particle_renderer_rasterize_shape() returns a truncated pointer to a pixel
 * buffer that the module never frees, so its public success path is checked for
 * the return value and a nonzero handle only. Pixel coverage is checked through
 * the helpers it dispatches to.
 *
 * svg_particle_renderer_render() and svg_particle_renderer_render_system() have no
 * observable output yet (they are TODO stubs), so their tests cover the guard
 * clauses and confirm that populated emitters are walked without being changed.
 */

#include "effects/svg_particles/svg_particle_renderer.c"

#include <stdio.h>
#include <stdlib.h>

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

static bool is_opaque_white(const uint8_t *pixels, uint32_t width, uint32_t x,
                            uint32_t y) {
  const uint8_t *p = pixels + (y * width + x) * 4;
  return p[0] == 255 && p[1] == 255 && p[2] == 255 && p[3] == 255;
}

static bool is_clear(const uint8_t *pixels, uint32_t width, uint32_t x,
                     uint32_t y) {
  const uint8_t *p = pixels + (y * width + x) * 4;
  return p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 0;
}

static SVGParticleEmitter *make_emitter(uint32_t max_particles, uint32_t burst) {
  SVGParticleEmitterConfig config = {
      .max_particles = max_particles,
      .spawn_rate = 0,
      .lifetime_min = 1.0f,
      .lifetime_max = 2.0f,
      .scale_min = 1.0f,
      .scale_max = 1.0f,
      .opacity = 1.0f,
      .gravity = 0.0f,
      .drag = 0.0f,
      .svg_shape = NULL,
      .svg_element_count = 0,
  };
  SVGParticleEmitter *emitter = svg_particle_emitter_create(config);
  if (emitter && burst > 0) {
    svg_particle_emitter_burst(emitter, burst);
  }
  return emitter;
}

static void test_init_and_cleanup_are_idempotent(void) {
  svg_particle_renderer_cleanup();
  EXPECT(!_renderer_state.initialized,
         "cleanup before init should leave the renderer uninitialised");
  EXPECT(_renderer_state.rasterization_cache == NULL,
         "no cache should exist before init");

  svg_particle_renderer_init();
  EXPECT(_renderer_state.initialized, "init should mark the renderer initialised");
  EXPECT(_renderer_state.rasterization_cache != NULL,
         "init should allocate the rasterization cache");
  EXPECT(_renderer_state.cache_size == 16,
         "init should size the cache to its 16-entry initial capacity");

  uint32_t *cache = _renderer_state.rasterization_cache;
  svg_particle_renderer_init();
  EXPECT(_renderer_state.rasterization_cache == cache,
         "a second init must not reallocate the cache");

  svg_particle_renderer_cleanup();
  EXPECT(!_renderer_state.initialized, "cleanup should clear the initialised flag");
  EXPECT(_renderer_state.rasterization_cache == NULL,
         "cleanup should release the cache");
  EXPECT(_renderer_state.cache_size == 0, "cleanup should reset the cache size");

  svg_particle_renderer_cleanup();
  EXPECT(!_renderer_state.initialized, "a second cleanup should be a no-op");
}

static void test_rasterize_rejects_null_arguments(void) {
  /* Static: SVGElement holds an 8192-entry path array, too large for the stack. */
  static SVGElement shape;
  uint32_t handle = 0xABCDu;

  EXPECT(!svg_particle_renderer_rasterize_shape(NULL, 8, 8, &handle),
         "a NULL shape should be rejected");
  EXPECT(!svg_particle_renderer_rasterize_shape(&shape, 8, 8, NULL),
         "a NULL out_texture should be rejected");
  EXPECT(handle == 0xABCDu, "a rejected call must not write the handle");
}

static void test_rasterize_shape_returns_handle_for_each_kind(void) {
  /* Each call leaks its pixel buffer: the module offers no free path for the
   * handle, and the handle is a truncated pointer that cannot be reversed. */
  static SVGElement shape;
  const SVGElementType kinds[] = {
      SVG_ELEMENT_CIRCLE,
      SVG_ELEMENT_RECT,
      SVG_ELEMENT_PATH, /* not a basic shape: falls back to a circle */
  };

  for (size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++) {
    shape.type = kinds[i];
    uint32_t handle = 0;
    EXPECT(svg_particle_renderer_rasterize_shape(&shape, 8, 8, &handle),
           "a shape of any supported kind should rasterize");
    EXPECT(handle != 0, "a rasterized shape should return a nonzero handle");
  }
}

static void test_circle_fills_inclusive_radius(void) {
  /* Centre (2,2), radius 1. The four edge neighbours lie exactly on the radius
   * and must be filled (dist <= radius). The diagonals (dist sqrt(2)) must not. */
  uint8_t pixels[5 * 5 * 4];
  _rasterize_circle(pixels, 5, 5, 2.0f, 2.0f, 1.0f);

  EXPECT(is_opaque_white(pixels, 5, 2, 2), "the centre pixel should be filled");
  EXPECT(is_opaque_white(pixels, 5, 2, 1),
         "an edge pixel exactly on the radius should be filled");
  EXPECT(is_opaque_white(pixels, 5, 3, 2),
         "an edge pixel exactly on the radius should be filled");
  EXPECT(is_clear(pixels, 5, 1, 1),
         "a diagonal pixel outside the radius should be clear");
  EXPECT(is_clear(pixels, 5, 0, 0), "a corner pixel should be clear");
  EXPECT(is_clear(pixels, 5, 2, 0), "a pixel two rows from centre should be clear");
}

static void test_rect_fills_inclusive_bounds(void) {
  /* Rect spans x 1..3 and y 1..4 inclusive. */
  uint8_t pixels[6 * 6 * 4];
  _rasterize_rect(pixels, 6, 6, 1.0f, 1.0f, 3.0f, 4.0f);

  EXPECT(is_opaque_white(pixels, 6, 1, 1), "the top-left corner should be filled");
  EXPECT(is_opaque_white(pixels, 6, 3, 4),
         "the bottom-right corner should be filled");
  EXPECT(is_clear(pixels, 6, 0, 1), "a pixel left of the rect should be clear");
  EXPECT(is_clear(pixels, 6, 4, 4), "a pixel right of the rect should be clear");
  EXPECT(is_clear(pixels, 6, 2, 5), "a pixel below the rect should be clear");
}

static void test_render_ignores_null_and_empty_inputs(void) {
  SVGParticleRenderState state = {0};
  SVGParticleEmitter *populated = make_emitter(4, 2);
  SVGParticleEmitter *empty = make_emitter(4, 0);
  EXPECT(populated != NULL, "a populated emitter should be created");
  EXPECT(empty != NULL, "an empty emitter should be created");

  svg_particle_renderer_render(NULL, &state);
  svg_particle_renderer_render(populated, NULL);
  svg_particle_renderer_render(empty, &state);
  svg_particle_renderer_render_system(NULL);

  EXPECT(populated->particle_count == 2,
         "rejected render calls must not change the emitter");

  svg_particle_emitter_destroy(populated);
  svg_particle_emitter_destroy(empty);
}

static void test_render_leaves_emitter_unchanged(void) {
  SVGParticleRenderState state = {0};
  SVGParticleEmitter *emitter = make_emitter(4, 3);
  EXPECT(emitter != NULL, "an emitter should be created");
  EXPECT(svg_particle_emitter_get_active_count(emitter) == 3,
         "the burst should activate three particles");

  svg_particle_renderer_render(emitter, &state);
  EXPECT(emitter->particle_count == 3,
         "render must not change the particle count");
  EXPECT(svg_particle_emitter_get_active_count(emitter) == 3,
         "render must not change the active count");

  svg_particle_emitter_destroy(emitter);
}

static void test_render_system_walks_populated_emitters(void) {
  SVGParticleEmitter *emitter = make_emitter(4, 2);
  EXPECT(emitter != NULL, "an emitter should be created");

  /* Built by hand rather than with svg_particle_system_create/destroy: the
   * destroy path frees interior pointers of the emitter array. */
  SVGParticleSystem system = {
      .emitters = emitter,
      .emitter_count = 1,
      .max_emitters = 1,
      .delta_time = 0.0f,
  };
  svg_particle_renderer_render_system(&system);
  EXPECT(emitter->particle_count == 2,
         "render_system must not change the emitter's particles");

  svg_particle_emitter_destroy(emitter);
}

int main(void) {
  test_init_and_cleanup_are_idempotent();
  test_rasterize_rejects_null_arguments();
  test_rasterize_shape_returns_handle_for_each_kind();
  test_circle_fills_inclusive_radius();
  test_rect_fills_inclusive_bounds();
  test_render_ignores_null_and_empty_inputs();
  test_render_leaves_emitter_unchanged();
  test_render_system_walks_populated_emitters();

  if (g_failed) {
    fprintf(stderr, "%d svg particle renderer test(s) failed\n", g_failed);
    return 1;
  }
  printf("svg particle renderer tests passed\n");
  return 0;
}

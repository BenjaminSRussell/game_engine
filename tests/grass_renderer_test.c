/* Issue #342: grass instance placement and distance culling.
 * Compiles grass_renderer.c directly. Does not link Engine.
 */
#include <stdio.h>
#include <string.h>

#include "environment/landscape/terrain/landscape/vegetation/grass_renderer.h"

static int fail(const char *msg) {
  fprintf(stderr, "grass renderer: %s\n", msg);
  return 1;
}

static Vec3 cam_at(float x, float y, float z) {
  Vec3 v;
  memset(&v, 0, sizeof(v));
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

static landscape_grass_renderer_desc_t desc_grid(float density, uint32_t chunk,
                                                 float cull) {
  landscape_grass_renderer_desc_t desc;
  memset(&desc, 0, sizeof(desc));
  desc.flags = 0xA5u;
  desc.blade_config.width = 0.05f;
  desc.blade_config.min_height = 0.2f;
  desc.blade_config.max_height = 0.6f;
  desc.blade_config.bend_amount = 0.15f;
  desc.blade_config.wind_stiffness = 0.5f;
  desc.density = density;
  desc.cull_distance = cull;
  desc.chunk_size = chunk;
  return desc;
}

int main(void) {
  landscape_grass_renderer_handle_t handle;
  landscape_grass_renderer_handle_t second;
  landscape_grass_renderer_handle_t extra;
  landscape_grass_renderer_desc_t desc;
  landscape_grass_renderer_info_t info;
  landscape_grass_renderer_handle_t filled[8];
  Vec3 origin;
  int i;

  if (landscape_grass_renderer_create(&handle, NULL) != -1) {
    return fail("create before init should fail");
  }
  if (landscape_grass_renderer_process_pending() != -1) {
    return fail("process_pending before init should fail");
  }
  if (landscape_grass_renderer_init() != 0) {
    return fail("init failed");
  }
  if (landscape_grass_renderer_init() != 0) {
    return fail("second init failed");
  }
  if (landscape_grass_renderer_get_count() != 0) {
    return fail("count after init is not 0");
  }

  desc = desc_grid(4.0f, 1u, 1.0f);
  if (landscape_grass_renderer_create(NULL, &desc) != -1) {
    return fail("null handle should fail");
  }
  if (landscape_grass_renderer_create(&handle, NULL) != -1) {
    return fail("null desc should fail");
  }
  desc.density = -1.0f;
  if (landscape_grass_renderer_create(&handle, &desc) != -1) {
    return fail("negative density should fail");
  }
  desc = desc_grid(4.0f, 1u, 1.0f);
  if (landscape_grass_renderer_create(&handle, &desc) != 0) {
    return fail("create failed");
  }
  if (!landscape_grass_renderer_is_valid(handle)) {
    return fail("new handle is not valid");
  }
  if (landscape_grass_renderer_get_info(handle, NULL) != -1) {
    return fail("null info should fail");
  }
  if (landscape_grass_renderer_get_info(handle, &info) != 0) {
    return fail("get_info failed");
  }
  /* density 4 on a 1m chunk places 4 blades. All are visible before culling. */
  if (info.instance_count != 4 || info.visible_instances != 4) {
    return fail("expected 4 instances before culling");
  }
  if (info.flags != 0xA5u || !info.initialized || info.id != handle.id) {
    return fail("info did not echo flags or id");
  }
  if (info.buffer_memory == 0 ||
      landscape_grass_renderer_get_memory_usage() != info.buffer_memory) {
    return fail("memory usage does not match the instance buffer");
  }

  origin = cam_at(0.0f, 0.0f, 0.0f);
  if (landscape_grass_renderer_update(handle, origin, 0.016f) != 0) {
    return fail("update at origin failed");
  }
  if (landscape_grass_renderer_get_info(handle, &info) != 0) {
    return fail("get_info after origin update failed");
  }
  /* Grid cells are (-0.5, -0.5), (0, -0.5), (-0.5, 0), (0, 0). Cull of 1m
   * keeps every blade. A later tight cull keeps only the blade on the origin. */
  if (info.visible_instances != 4 || info.instance_count != 4) {
    return fail("origin cull dropped blades inside 1m");
  }

  desc = desc_grid(4.0f, 1u, 0.1f);
  if (landscape_grass_renderer_create(&second, &desc) != 0) {
    return fail("second create failed");
  }
  if (landscape_grass_renderer_get_count() != 2) {
    return fail("count is not 2");
  }
  if (landscape_grass_renderer_update(second, origin, 0.0f) != 0) {
    return fail("tight cull update failed");
  }
  if (landscape_grass_renderer_get_info(second, &info) != 0) {
    return fail("tight cull get_info failed");
  }
  if (info.instance_count != 4 || info.visible_instances != 1) {
    return fail("tight cull should keep the origin blade only");
  }
  landscape_grass_renderer_render(second, NULL);
  if (landscape_grass_renderer_get_info(second, &info) != 0 ||
      info.visible_instances != 1) {
    return fail("render changed the visible set");
  }

  if (landscape_grass_renderer_update(second, cam_at(100.0f, 0.0f, 0.0f), 0.5f) !=
      0) {
    return fail("far camera update failed");
  }
  if (landscape_grass_renderer_get_info(second, &info) != 0 ||
      info.visible_instances != 0 || info.instance_count != 4) {
    return fail("far camera should cull every blade");
  }
  if (landscape_grass_renderer_update(second, origin, -0.1f) != -1) {
    return fail("negative delta_time should fail");
  }

  landscape_grass_renderer_mark_dirty(second);
  if (landscape_grass_renderer_process_pending() != 1) {
    return fail("process_pending should rebuild one renderer");
  }
  if (landscape_grass_renderer_get_info(second, &info) != 0 ||
      info.visible_instances != 4) {
    return fail("rebuild should restore the unculled instance set");
  }
  if (landscape_grass_renderer_process_pending() != 0) {
    return fail("second process_pending should be a no-op");
  }

  landscape_grass_renderer_destroy(second);
  if (landscape_grass_renderer_is_valid(second) ||
      landscape_grass_renderer_get_count() != 1) {
    return fail("destroy did not drop the second renderer");
  }
  if (landscape_grass_renderer_get_info(second, &info) != -1) {
    return fail("get_info on a destroyed handle should fail");
  }
  if (landscape_grass_renderer_create(&second, &desc) != 0 || second.id == 0) {
    return fail("create should reuse a free slot");
  }

  landscape_grass_renderer_shutdown();
  if (landscape_grass_renderer_is_valid(handle) ||
      landscape_grass_renderer_get_count() != 0 ||
      landscape_grass_renderer_get_memory_usage() != 0) {
    return fail("shutdown left renderers alive");
  }
  if (landscape_grass_renderer_update(handle, origin, 0.0f) != -1) {
    return fail("update after shutdown should fail");
  }

  if (landscape_grass_renderer_init() != 0) {
    return fail("re-init failed");
  }
  desc = desc_grid(1.0f, 0u, 10.0f);
  if (landscape_grass_renderer_create(&handle, &desc) != 0) {
    return fail("zero chunk create failed");
  }
  if (landscape_grass_renderer_get_info(handle, &info) != 0 ||
      info.instance_count != 0 || info.buffer_memory != 0) {
    return fail("zero chunk should place no blades");
  }

  landscape_grass_renderer_shutdown();
  landscape_grass_renderer_init();
  desc = desc_grid(1.0f, 2u, 8.0f);
  for (i = 0; i < 8; i++) {
    if (landscape_grass_renderer_create(&filled[i], &desc) != 0) {
      return fail("slot fill failed");
    }
  }
  if (landscape_grass_renderer_get_count() != 8) {
    return fail("expected 8 live renderers");
  }
  if (landscape_grass_renderer_create(&extra, &desc) != -1 || extra.id != 0) {
    return fail("create past capacity should fail");
  }
  landscape_grass_renderer_debug_print();
  landscape_grass_renderer_shutdown();

  printf("grass renderer instances ok\n");
  return 0;
}

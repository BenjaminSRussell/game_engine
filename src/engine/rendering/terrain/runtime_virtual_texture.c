// src/engine/rendering/terrain/runtime_virtual_texture.c
#include "include/rendering/terrain/runtime_virtual_texture.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>

RVTSystem *rvt_system_create(void) {
  RVTSystem *sys = (RVTSystem *)calloc(1, sizeof(RVTSystem));
  if (!sys)
    return NULL;

  LOG_INFO("Runtime Virtual Texture System initialized");
  return sys;
}

void rvt_system_destroy(RVTSystem *sys) {
  if (sys)
    free(sys);
}

u32 rvt_add_volume(RVTSystem *sys, Vec3 origin, Vec3 size,
                   uint32_t resolution) {
  if (!sys || sys->volume_count >= RVT_MAX_VOLUMES)
    return 0xFFFFFFFF;

  u32 id = sys->volume_count++;
  RVTVolume *vol = &sys->volumes[id];

  vol->origin = origin;
  vol->size = size;
  vol->resolution = resolution;
  vol->active = true;

  LOG_INFO("RVT: Added volume at (%.1f, %.1f, %.1f) res %u", origin.x, origin.y,
           origin.z, resolution);

  return id;
}

void rvt_remove_volume(RVTSystem *sys, u32 volume_id) {
  if (!sys || volume_id >= sys->volume_count)
    return;
  sys->volumes[volume_id].active = false;
}

void rvt_draw_to_volume(RVTSystem *sys, u32 volume_id, void *render_context) {
  if (!sys || volume_id >= sys->volume_count)
    return;
  RVTVolume *vol = &sys->volumes[volume_id];
  if (!vol->active)
    return;

  // In a real implementation:
  // 1. Set render target to vol->gpu_texture_handle
  // 2. Set orthographic projection matching vol->origin/size
  // 3. Render terrain/objects filtered for RVT
}

void *rvt_get_texture(RVTSystem *sys, u32 volume_id) {
  if (!sys || volume_id >= sys->volume_count)
    return NULL;
  return sys->volumes[volume_id].gpu_texture_handle;
}

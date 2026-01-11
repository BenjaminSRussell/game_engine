#ifndef RUNTIME_VIRTUAL_TEXTURE_H
#define RUNTIME_VIRTUAL_TEXTURE_H

#include "include/common.h"
#include "include/math/vec3.h"

#define RVT_MAX_VOLUMES 16

typedef enum {
  RVT_CONTENT_COLOR,
  RVT_CONTENT_HEIGHT,
  RVT_CONTENT_NORMAL,
  RVT_CONTENT_MASK
} RVTContentType;

typedef struct {
  Vec3 origin;
  Vec3 size;
  void *gpu_texture_handle;
  uint32_t resolution;
  bool active;
} RVTVolume;

typedef struct {
  RVTVolume volumes[RVT_MAX_VOLUMES];
  uint32_t volume_count;
} RVTSystem;

#ifdef __cplusplus
extern "C" {
#endif

RVTSystem *rvt_system_create(void);
void rvt_system_destroy(RVTSystem *sys);

u32 rvt_add_volume(RVTSystem *sys, Vec3 origin, Vec3 size, uint32_t resolution);
void rvt_remove_volume(RVTSystem *sys, u32 volume_id);

// Scene Rendering Hooks
void rvt_draw_to_volume(RVTSystem *sys, u32 volume_id, void *render_context);
void *rvt_get_texture(RVTSystem *sys, u32 volume_id);

#ifdef __cplusplus
}
#endif

#endif

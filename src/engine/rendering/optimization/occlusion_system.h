#ifndef OCCLUSION_SYSTEM_H
#define OCCLUSION_SYSTEM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  OCCLUSION_MODE_NONE = 0,
  OCCLUSION_MODE_PORTAL = 1,
  OCCLUSION_MODE_HARDWARE = 2
} OcclusionMode;

void occlusion_sys_init(void);
void occlusion_sys_shutdown(void);
void occlusion_sys_update(float delta_time);

void occlusion_sys_set_mode(OcclusionMode mode);
OcclusionMode occlusion_sys_get_mode(void);

uint32_t occlusion_sys_get_visible_objects(void);
uint32_t occlusion_sys_get_culled_objects(void);

void occlusion_sys_register_occluder(uint64_t entity_id);
void occlusion_sys_unregister_occluder(uint64_t entity_id);

void occlusion_sys_set_debug_visualization(bool enabled);
bool occlusion_sys_get_debug_visualization(void);

#endif // OCCLUSION_SYSTEM_H

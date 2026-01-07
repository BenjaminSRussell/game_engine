#include "platform/occlusion_api_bridge.h"
#include "core/logger.h"
#include "rendering/optimization/occlusion_system.h"

void occlusion_set_mode(PlatformOcclusionMode mode) { occlusion_sys_set_mode((OcclusionMode)mode); }

PlatformOcclusionMode occlusion_get_mode(void) { return (PlatformOcclusionMode)occlusion_sys_get_mode(); }

uint32_t occlusion_get_visible_objects(void) {
  return occlusion_sys_get_visible_objects();
}

uint32_t occlusion_get_culled_objects(void) {
  return occlusion_sys_get_culled_objects();
}

void occlusion_register_occluder(uint64_t entity_id) {
  occlusion_sys_register_occluder(entity_id);
}

void occlusion_unregister_occluder(uint64_t entity_id) {
  occlusion_sys_unregister_occluder(entity_id);
}

void occlusion_set_debug_visualization(bool enabled) {
  occlusion_sys_set_debug_visualization(enabled);
}

bool occlusion_get_debug_visualization(void) {
  return occlusion_sys_get_debug_visualization();
}

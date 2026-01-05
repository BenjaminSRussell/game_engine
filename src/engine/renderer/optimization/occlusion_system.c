#include "renderer/optimization/occlusion_system.h"
#include "core/logger.h"
#include <string.h>

#define MAX_OCCLUDERS 256

static struct {
  OcclusionMode mode;
  uint32_t visible_objects;
  uint32_t culled_objects;
  uint64_t occluders[MAX_OCCLUDERS];
  uint32_t occluder_count;
  bool debug_viz;
} occlusion_state;

void occlusion_sys_init(void) {
  occlusion_state.mode = OCCLUSION_MODE_NONE;
  occlusion_state.visible_objects = 0;
  occlusion_state.culled_objects = 0;
  occlusion_state.occluder_count = 0;
  occlusion_state.debug_viz = false;
  memset(occlusion_state.occluders, 0, sizeof(occlusion_state.occluders));
  LOG_INFO("Occlusion System Initialized");
}

void occlusion_sys_shutdown(void) { LOG_INFO("Occlusion System Shutdown"); }

void occlusion_sys_update(float delta_time) {
  // Simulate culling based on mode
  switch (occlusion_state.mode) {
  case OCCLUSION_MODE_NONE:
    occlusion_state.visible_objects = 1000;
    occlusion_state.culled_objects = 0;
    break;
  case OCCLUSION_MODE_PORTAL:
    occlusion_state.visible_objects = 650;
    occlusion_state.culled_objects = 350;
    break;
  case OCCLUSION_MODE_HARDWARE:
    occlusion_state.visible_objects = 450;
    occlusion_state.culled_objects = 550;
    break;
  }
}

void occlusion_sys_set_mode(OcclusionMode mode) {
  occlusion_state.mode = mode;
  LOG_INFO("Occlusion mode set to %d", mode);
}

OcclusionMode occlusion_sys_get_mode(void) { return occlusion_state.mode; }

uint32_t occlusion_sys_get_visible_objects(void) {
  return occlusion_state.visible_objects;
}

uint32_t occlusion_sys_get_culled_objects(void) {
  return occlusion_state.culled_objects;
}

void occlusion_sys_register_occluder(uint64_t entity_id) {
  if (occlusion_state.occluder_count < MAX_OCCLUDERS) {
    occlusion_state.occluders[occlusion_state.occluder_count++] = entity_id;
    LOG_INFO("Registered occluder: %llu", entity_id);
  }
}

void occlusion_sys_unregister_occluder(uint64_t entity_id) {
  for (uint32_t i = 0; i < occlusion_state.occluder_count; i++) {
    if (occlusion_state.occluders[i] == entity_id) {
      occlusion_state.occluders[i] =
          occlusion_state.occluders[--occlusion_state.occluder_count];
      LOG_INFO("Unregistered occluder: %llu", entity_id);
      return;
    }
  }
}

void occlusion_sys_set_debug_visualization(bool enabled) {
  occlusion_state.debug_viz = enabled;
}

bool occlusion_sys_get_debug_visualization(void) {
  return occlusion_state.debug_viz;
}

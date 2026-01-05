#include "platform/destruction_api_bridge.h"
#include "core/logger.h"
#include "physics/destruction/destruction_impl.h"
#include "physics/fracture/voronoi_shatter.h"
#include <stdbool.h>
#include <stdint.h>

void destruction_set_enabled(bool enabled) {
  destruction_sys_set_enabled(enabled);
}

bool destruction_is_enabled(void) { return destruction_sys_is_enabled(); }

void destruction_set_debris_lifetime(float lifetime_seconds) {
  destruction_sys_set_debris_lifetime(lifetime_seconds);
}

float destruction_get_debris_lifetime(void) {
  return destruction_sys_get_debris_lifetime();
}

uint64_t destruction_create_fractured_mesh(uint64_t source_mesh_id,
                                           uint32_t fragment_count) {
  return voronoi_shatter_mesh(source_mesh_id, fragment_count);
}

void destruction_trigger_break(uint64_t entity_id, float x, float y, float z,
                               float force) {
  destruction_sys_trigger_break(entity_id, x, y, z, force);
}

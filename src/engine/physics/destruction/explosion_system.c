#include "destruction_impl.h"
#include <math.h>
#include <stdio.h>

void destruction_sys_trigger_explosion(float x, float y, float z, float radius,
                                       float force) {
  if (!destruction_sys_is_enabled())
    return;

  printf("[Physics] Explosion at (%.2f, %.2f, %.2f) with radius %.2f and force "
         "%.2f\n",
         x, y, z, radius, force);

  // In a real implementation:
  // 1. Spatial query to find all destructible entities within 'radius'
  // 2. For each entity, calculate effective force based on distance
  // 3. Call destruction_sys_trigger_break(entity_id, x, y, z, effective_force)
}

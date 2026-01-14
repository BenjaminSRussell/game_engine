#include "../Public/unified_ecs.h"
#include "ecs_types.h"
#include "unified_logger.h"
#include "unified_memory.h"
#include <string.h>

void ecs_register_system(World *world, const char *name,
                         SystemUpdateFn update_fn) {
  if (!world || !update_fn)
    return;

  // Check if system exists (optional, simply appending for now)

  System *sys = UNIFIED_ALLOC(sizeof(System));
  memset(sys, 0, sizeof(System));
  if (name) {
    strncpy(sys->name, name, 63);
  } else {
    snprintf(sys->name, 64, "System_%d", world->system_count);
  }
  sys->update_fn = update_fn;
  sys->next = NULL;

  // Append to end of list (to maintain order)
  if (!world->systems_head) {
    world->systems_head = sys;
  } else {
    System *curr = world->systems_head;
    while (curr->next) {
      curr = curr->next;
    }
    curr->next = sys;
  }

  world->system_count++;
  LOG_INFO(LOG_CAT_GENERAL, "Registered System: %s", sys->name);
}

void ecs_run_systems(World *world, float dt) {
  if (!world)
    return;

  System *curr = world->systems_head;
  while (curr) {
    // Run system
    // Note: Simple systems run sequentially.
    if (curr->update_fn) {
      curr->update_fn(world, dt);
    }
    curr = curr->next;
  }
}

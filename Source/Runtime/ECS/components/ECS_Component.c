#include "../../Core/Memory/Public/Memory.h"
#include "../Public/ECS.h"
#include "ECS/Private/ECS_Internal.h"
#include <string.h>

#define MAX_COMPONENTS_REGISTERED 256

typedef struct ComponentInfoInternal {
  char name[64];
  u32 size;
  u32 alignment;
  u32 id;
} ComponentInfoInternal;

typedef struct ComponentManager {
  ComponentInfoInternal registered_components[MAX_COMPONENTS_REGISTERED];
  u32 registered_count;
} ComponentManager;

// Global instance (or part of World)
static ComponentManager g_comp_mgr = {0};

void ComponentManager_Init(void) {
  Memory_Zero(&g_comp_mgr, sizeof(ComponentManager));
}

ECSComponentID ComponentManager_Register(const char *name, u32 size,
                                         u32 alignment) {
  if (g_comp_mgr.registered_count >= MAX_COMPONENTS_REGISTERED)
    return 0;

  // Check if duplicate
  for (u32 i = 0; i < g_comp_mgr.registered_count; i++) {
    if (strcmp(g_comp_mgr.registered_components[i].name, name) == 0) {
      return g_comp_mgr.registered_components[i].id;
    }
  }

  u32 index = g_comp_mgr.registered_count++;
  ComponentInfoInternal *info = &g_comp_mgr.registered_components[index];
  strncpy(info->name, name, 63);
  info->size = size;
  info->alignment = alignment;
  // IDs start at 1, bitmask compatible if < 32 (simplified)
  // For archetype ECS, sequential IDs are fine.
  info->id = index + 1;

  return info->id;
}

u32 ComponentManager_GetSize(ECSComponentID id) {
  if (id == 0 || id > g_comp_mgr.registered_count)
    return 0;
  return g_comp_mgr.registered_components[id - 1].size;
}

const char *ComponentManager_GetName(ECSComponentID id) {
  if (id == 0 || id > g_comp_mgr.registered_count)
    return "Invalid";
  return g_comp_mgr.registered_components[id - 1].name;
}

#include "Public/ECS.h"
#include "../Memory/Public/Memory.h"
#include "Private/ECS_Internal.h"
#include <string.h>

// ECS God Function Implementation
// Delegates details to subsystems (Managers)

static bool g_ecs_initialized = false;
static EntityID g_next_entity_id = 1;

// Entity Record to track which Archetype an entity belongs to
// (Needed for fast lookup: EntityID -> Archetype + Index)
typedef struct EntityRecord {
  Archetype *archetype;
  // For now we don't store chunk index/index within chunk here
  // In optimized version we would.
  // This simple record just tells us where to LOOK (which archetype).
  bool active;
} EntityRecord;

// #define MAX_ENTITIES 10000
static EntityRecord g_entity_index[ECS_MAX_ENTITIES];

bool ECS_Init(void) {
  if (g_ecs_initialized)
    return true;

  Memory_Zero(g_entity_index, sizeof(g_entity_index));
  g_next_entity_id = 1;

  // Initialize Managers
  ComponentManager_Init();
  ArchetypeManager_Init();
  SystemManager_Init();
  QueryManager_Init();

  g_ecs_initialized = true;
  return true;
}

ComponentType ECS_RegisterComponent(const char *name, u32 size, u32 alignment) {
  if (!g_ecs_initialized)
    return 0;
  return ComponentManager_Register(name, size, alignment);
}

void ECS_Shutdown(void) {
  if (!g_ecs_initialized)
    return;

  ArchetypeManager_Shutdown();
  // Other managers don't have shutdown needs currently (static arrays)

  g_ecs_initialized = false;
}

EntityID ECS_CreateEntity(void) {
  if (!g_ecs_initialized)
    return INVALID_ENTITY_ID;
  if (g_next_entity_id >= ECS_MAX_ENTITIES)
    return INVALID_ENTITY_ID;

  EntityID id = g_next_entity_id++;
  g_entity_index[id].active = true;

  // New entities start with NO components, so they effectively belong to "Empty
  // Archetype" or just don't have an archetype yet. Let's say NULL archetype =
  // no components.
  g_entity_index[id].archetype = NULL;

  return id;
}

void ECS_DestroyEntity(EntityID entity) {
  if (!g_ecs_initialized || entity >= ECS_MAX_ENTITIES)
    return;

  // Mark inactive
  // Real impl: Copy-swap remove from archetype chunk
  g_entity_index[entity].active = false;
  g_entity_index[entity].archetype = NULL;
}

bool ECS_IsEntityValid(EntityID entity) {
  if (entity == INVALID_ENTITY_ID || entity >= ECS_MAX_ENTITIES)
    return false;
  return g_entity_index[entity].active;
}

void *ECS_AddComponent(EntityID entity, ComponentType type, void *data) {
  if (!ECS_IsEntityValid(entity))
    return NULL;

  // Archetype Transition: Old Archetype -> New Archetype (+type)
  // 1. Get current component types
  Archetype *old_arch = g_entity_index[entity].archetype;

  // To do this properly we need to know existing types.
  // Simplified Phase 4: Construct new list of types
  // This is expensive O(N) where N is component count.

  // Temp: assumed max components 32
  ComponentType new_types[32];
  u32 count = 0;

  if (old_arch) {
    // Copy existing
    // Check if already has component
    for (u32 i = 0; i < old_arch->component_count; i++) {
      if (old_arch->component_types[i] == type) {
        // Already has it, just update data
        void *ptr = Archetype_GetComponent(old_arch, entity, type);
        if (ptr && data) {
          u32 size = ComponentManager_GetSize(type);
          if (size == 0)
            size = 1;
          memcpy(ptr, data, size);
        }
        return ptr;
      }
      new_types[count++] = old_arch->component_types[i];
    }
  }

  // Add new
  new_types[count++] = type;

  // Sort types to ensure canonical archetypes (simple bubble sort)
  for (u32 i = 0; i < count - 1; i++) {
    for (u32 j = 0; j < count - i - 1; j++) {
      if (new_types[j] > new_types[j + 1]) {
        ComponentType temp = new_types[j];
        new_types[j] = new_types[j + 1];
        new_types[j + 1] = temp;
      }
    }
  }

  // Find/Create new archetype
  Archetype *new_arch = Archetype_FindOrCreate(new_types, count);

  // Move entity: Add to new
  Archetype_AddEntity(new_arch, entity);

  // Copy/Move old components properties
  // For every component in old_arch, copy value to new_arch
  if (old_arch) {
    for (u32 i = 0; i < old_arch->component_count; i++) {
      ComponentType t = old_arch->component_types[i];
      void *src = Archetype_GetComponent(old_arch, entity, t);
      void *dst = Archetype_GetComponent(new_arch, entity, t);
      if (src && dst) {
        u32 size = ComponentManager_GetSize(t);
        if (size == 0)
          size = 1;
        memcpy(dst, src, size);
      }
    }
    // Remove from old archetype (Logic needed in Archetype_RemoveEntity)
    // Stub: Just leave it "dead" in old archetype for now (leak in this
    // simplified version)
  }

  // Initialize new component data
  void *ptr = Archetype_GetComponent(new_arch, entity, type);
  if (ptr && data) {
    u32 size = ComponentManager_GetSize(type);
    if (size == 0)
      size = 1;
    memcpy(ptr, data, size);
  }

  // Update index
  g_entity_index[entity].archetype = new_arch;

  return ptr;
}

void *ECS_GetComponent(EntityID entity, ComponentType type) {
  if (!ECS_IsEntityValid(entity))
    return NULL;

  Archetype *arch = g_entity_index[entity].archetype;
  if (!arch)
    return NULL;

  return Archetype_GetComponent(arch, entity, type);
}

void ECS_RemoveComponent(EntityID entity, ComponentType type) {
  if (!ECS_IsEntityValid(entity))
    return;

  Archetype *old_arch = g_entity_index[entity].archetype;
  if (!old_arch)
    return;

  // Construct new type list (minus removed type)
  ComponentType new_types[32];
  u32 count = 0;
  bool found = false;

  for (u32 i = 0; i < old_arch->component_count; i++) {
    if (old_arch->component_types[i] == type) {
      found = true;
      continue; // Skip
    }
    new_types[count++] = old_arch->component_types[i];
  }

  if (!found)
    return; // Didn't have it

  // Find/Create new archetype
  Archetype *new_arch = Archetype_FindOrCreate(new_types, count);

  // Move entity
  Archetype_AddEntity(new_arch, entity);

  // Copy remaining components
  for (u32 i = 0; i < count; i++) {
    ComponentType t = new_types[i];
    void *src = Archetype_GetComponent(old_arch, entity, t);
    void *dst = Archetype_GetComponent(new_arch, entity, t);
    if (src && dst) {
      u32 size = ComponentManager_GetSize(t);
      if (size == 0)
        size = 1;
      memcpy(dst, src, size);
    }
  }

  // Update index
  g_entity_index[entity].archetype = new_arch;
}

bool ECS_HasComponent(EntityID entity, ComponentType type) {
  return ECS_GetComponent(entity, type) != NULL;
}

void ECS_Update(float delta_time) {
  if (!g_ecs_initialized)
    return;
  SystemManager_Execute(delta_time);
}

void ECS_Render(void) {
  if (!g_ecs_initialized)
    return;
  SystemManager_ExecuteRender();
}

void ECS_RegisterSystem(const char *name, ECSSystemUpdateFunc update,
                        ECSSystemRenderFunc render, int order) {
  if (!g_ecs_initialized)
    return;
  // Cast render func to match SystemManager's internal type (which takes dummy
  // float) Or we could fix SystemManager, but casting is easier for now to
  // avoid ripple. Safe because we call with 0.0f and it ignores it.
  SystemManager_Register(name, (SystemFunction)update, (SystemFunction)render,
                         order);
}

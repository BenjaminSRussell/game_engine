#ifndef ULTIMATE_ENGINE_ECS_INTERNAL_H
#define ULTIMATE_ENGINE_ECS_INTERNAL_H

#include "../Public/ECS.h"

#include "../Public/ECS.h"

// ECS Component ID alias
typedef ComponentType ECSComponentID;

// Component Manager
void ComponentManager_Init(void);
ECSComponentID ComponentManager_Register(const char *name, u32 size,
                                         u32 alignment);
u32 ComponentManager_GetSize(ECSComponentID id);
const char *ComponentManager_GetName(ECSComponentID id);

// System Manager
void SystemManager_Init(void);
typedef void (*SystemFunction)(float dt);
void SystemManager_Register(const char *name, SystemFunction update,
                            SystemFunction render, int order);
void SystemManager_Execute(float dt);
void SystemManager_ExecuteRender(void);

// Archetype Manager

// Internal Structures (Exposed for ECS.c)
#define ENTITIES_PER_CHUNK 256
#define MAX_ARCHETYPES 1024

typedef struct Chunk {
  EntityID *entities;      // Array of Entity IDs in this chunk
  void **component_arrays; // Array of pointers (one per component type) to data
  u32 entity_count;
  u32 capacity;
} Chunk;

typedef struct Archetype {
  u32 id;
  ComponentType *component_types;
  u32 *component_sizes; // Needed for pointer arithmetic
  u32 component_count;
  Chunk **chunks;
  u32 chunk_count;
  u32 chunk_capacity;
} Archetype;
void ArchetypeManager_Init(void);
void ArchetypeManager_Shutdown(void);
Archetype *Archetype_FindOrCreate(ComponentType *types, u32 count);
// Note: ECS.h uses EntityID, ComponentType (u32)
void *Archetype_AddEntity(Archetype *arch, EntityID entity);
void *Archetype_GetComponent(Archetype *arch, EntityID entity,
                             ComponentType type);

// Query Manager
void QueryManager_Init(void);

#endif // ULTIMATE_ENGINE_ECS_INTERNAL_H

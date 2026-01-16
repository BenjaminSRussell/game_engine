#include "../../Core/Memory/Public/Memory.h"
#include "../Public/ECS.h"
#include "ECS/Private/ECS_Internal.h"
#include <string.h>

// ----------------------------------------------------------------------------
// Defines
// ----------------------------------------------------------------------------
// Structs moved to ECS_Internal.h

// Global Archetype Registry
static struct {
  Archetype *archetypes[MAX_ARCHETYPES];
  u32 count;
} g_archetype_mgr = {0};

// Forward declaration from ComponentManager
extern u32 ComponentManager_GetSize(ECSComponentID id);

// ----------------------------------------------------------------------------
// Helper Functions
// ----------------------------------------------------------------------------

static Chunk *Chunk_Create(ComponentType *component_types,
                           u32 component_count) {
  Chunk *chunk = (Chunk *)Memory_Allocate(sizeof(Chunk), MEMORY_TAG_ENTITY);
  if (!chunk)
    return NULL;

  chunk->capacity = ENTITIES_PER_CHUNK;
  chunk->entity_count = 0;

  // Allocate Entity ID array
  chunk->entities = (EntityID *)Memory_Allocate(
      sizeof(EntityID) * ENTITIES_PER_CHUNK, MEMORY_TAG_ENTITY);

  // Allocate array of component pointers
  chunk->component_arrays = (void **)Memory_Allocate(
      sizeof(void *) * component_count, MEMORY_TAG_ENTITY);

  // Allocate actual data pages for each component
  for (u32 i = 0; i < component_count; i++) {
    u32 size = ComponentManager_GetSize(component_types[i]);
    if (size == 0)
      size = 1; // Zero-size components (tags) handling?

    chunk->component_arrays[i] =
        Memory_Allocate(size * ENTITIES_PER_CHUNK, MEMORY_TAG_ENTITY);
    Memory_Zero(chunk->component_arrays[i], size * ENTITIES_PER_CHUNK);
  }

  return chunk;
}

static void Chunk_Destroy(Chunk *chunk, u32 component_count,
                          ComponentType *types) {
  if (!chunk)
    return;

  for (u32 i = 0; i < component_count; i++) {
    if (chunk->component_arrays[i]) {
      u32 size = ComponentManager_GetSize(types[i]);
      if (size == 0)
        size = 1;
      Memory_Free(chunk->component_arrays[i], size * ENTITIES_PER_CHUNK,
                  MEMORY_TAG_ENTITY);
    }
  }
  Memory_Free(chunk->component_arrays, sizeof(void *) * component_count,
              MEMORY_TAG_ENTITY);
  Memory_Free(chunk->entities, sizeof(EntityID) * ENTITIES_PER_CHUNK,
              MEMORY_TAG_ENTITY);
  Memory_Free(chunk, sizeof(Chunk), MEMORY_TAG_ENTITY);
}

// ----------------------------------------------------------------------------
// Archetype API
// ----------------------------------------------------------------------------

void ArchetypeManager_Init(void) {
  Memory_Zero(&g_archetype_mgr, sizeof(g_archetype_mgr));
}

void ArchetypeManager_Shutdown(void) {
  for (u32 i = 0; i < g_archetype_mgr.count; i++) {
    Archetype *arch = g_archetype_mgr.archetypes[i];
    if (arch) {
      for (u32 c = 0; c < arch->chunk_count; c++) {
        Chunk_Destroy(arch->chunks[c], arch->component_count,
                      arch->component_types);
      }
      Memory_Free(arch->chunks, sizeof(Chunk *) * arch->chunk_capacity,
                  MEMORY_TAG_ENTITY);
      Memory_Free(arch->component_types,
                  sizeof(ComponentType) * arch->component_count,
                  MEMORY_TAG_ENTITY);
      Memory_Free(arch->component_sizes, sizeof(u32) * arch->component_count,
                  MEMORY_TAG_ENTITY);
      Memory_Free(arch, sizeof(Archetype), MEMORY_TAG_ENTITY);
    }
  }
  g_archetype_mgr.count = 0;
}

Archetype *Archetype_FindOrCreate(ComponentType *types, u32 count) {
  // 1. Search existing
  for (u32 i = 0; i < g_archetype_mgr.count; i++) {
    Archetype *arch = g_archetype_mgr.archetypes[i];
    if (arch->component_count == count) {
      // Check types match (assuming sorted or strict order? For now strict)
      // Real impl should sort types before search
      if (memcmp(arch->component_types, types, sizeof(ComponentType) * count) ==
          0) {
        return arch;
      }
    }
  }

  // 2. Create New
  Archetype *arch =
      (Archetype *)Memory_Allocate(sizeof(Archetype), MEMORY_TAG_ENTITY);
  arch->id = g_archetype_mgr.count;
  arch->component_count = count;

  // Copy types
  if (count > 0) {
    arch->component_types = (ComponentType *)Memory_Allocate(
        sizeof(ComponentType) * count, MEMORY_TAG_ENTITY);
    Memory_Copy(arch->component_types, types, sizeof(ComponentType) * count);

    arch->component_sizes =
        (u32 *)Memory_Allocate(sizeof(u32) * count, MEMORY_TAG_ENTITY);
    for (u32 i = 0; i < count; ++i) {
      arch->component_sizes[i] = ComponentManager_GetSize(types[i]);
    }
  }

  arch->chunk_capacity = 4;
  arch->chunk_count = 0;
  arch->chunks =
      (Chunk **)Memory_Allocate(sizeof(Chunk *) * 4, MEMORY_TAG_ENTITY);

  g_archetype_mgr.archetypes[g_archetype_mgr.count++] = arch;
  return arch;
}

void *Archetype_AddEntity(Archetype *arch, EntityID entity) {
  Chunk *target = NULL;

  // Find chunk with space
  for (u32 i = 0; i < arch->chunk_count; i++) {
    if (arch->chunks[i]->entity_count < ENTITIES_PER_CHUNK) {
      target = arch->chunks[i];
      break;
    }
  }

  // Create new chunk if needed
  if (!target) {
    target = Chunk_Create(arch->component_types, arch->component_count);
    if (arch->chunk_count >= arch->chunk_capacity) {
      // Resize chunk list
      // Simplification: just cap or realloc (Realloc logic needed)
    }
    arch->chunks[arch->chunk_count++] = target;
  }

  target->entities[target->entity_count] = entity;
  target->entity_count++;

  // Return base pointer to components?
  // Usually we set components immediately.
  return NULL;
}

void *Archetype_GetComponent(Archetype *arch, EntityID entity,
                             ComponentType type) {
  // Search chunks for entity (Slow! O(N) where N is entities in archetype)
  // Needs Entity index map.
  // For now, linear scan of chunks
  for (u32 c = 0; c < arch->chunk_count; c++) {
    Chunk *chunk = arch->chunks[c];
    for (u32 e = 0; e < chunk->entity_count; e++) {
      if (chunk->entities[e] == entity) {
        // Found entity
        // Find component index
        for (u32 i = 0; i < arch->component_count; i++) {
          if (arch->component_types[i] == type) {
            u32 size = arch->component_sizes[i];
            if (size == 0)
              size = 1;
            char *array = (char *)chunk->component_arrays[i];
            return array + (e * size);
          }
        }
      }
    }
  }
  return NULL;
}

#ifndef ULTIMATE_ENGINE_MEMORY_H
#define ULTIMATE_ENGINE_MEMORY_H

#include "../../Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Memory Tags for tracking allocation types
typedef enum MemoryTag {
  MEMORY_TAG_UNKNOWN,
  MEMORY_TAG_ARRAY,
  MEMORY_TAG_DYNAMIC_ARRAY,
  MEMORY_TAG_DICT,
  MEMORY_TAG_RING_QUEUE,
  MEMORY_TAG_BST,
  MEMORY_TAG_STRING,
  MEMORY_TAG_APPLICATION,
  MEMORY_TAG_JOB,
  MEMORY_TAG_TEXTURE,
  MEMORY_TAG_MATERIAL_INSTANCE,
  MEMORY_TAG_RENDERER,
  MEMORY_TAG_GAME,
  MEMORY_TAG_TRANSFORM,
  MEMORY_TAG_ENTITY,
  MEMORY_TAG_ENTITY_NODE,
  MEMORY_TAG_SCENE,
  MEMORY_TAG_ARENA,
  MEMORY_TAG_MAX_TAGS
} MemoryTag;

// Lifecycle
VF_API bool Memory_Init(void);
VF_API void Memory_Shutdown(void);

// Core Allocation
VF_API void *Memory_Allocate(u64 size, MemoryTag tag);
VF_API void Memory_Free(void *block, u64 size, MemoryTag tag);
VF_API void *Memory_Zero(void *block, u64 size);
VF_API void *Memory_Copy(void *dest, const void *source, u64 size);
VF_API void *Memory_Set(void *dest, i32 value, u64 size);

// Memory Reporting
VF_API char *Memory_GetTagString(MemoryTag tag);

// Arena Allocator Forward Declaration
typedef struct Arena_Allocator Arena_Allocator;
VF_API Arena_Allocator *Arena_Create(u64 size);
VF_API void *Arena_Allocate(Arena_Allocator *arena, u64 size);
VF_API void Arena_Reset(Arena_Allocator *arena);
VF_API void Arena_Destroy(Arena_Allocator *arena);

#ifdef __cplusplus
}
#endif

#endif // ULTIMATE_ENGINE_MEMORY_H

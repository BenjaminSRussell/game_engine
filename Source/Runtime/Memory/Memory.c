#include "../../Core/Memory/Public/Memory.h"
#include "../Core/Public/core_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Implement advanced tracking and stats
typedef struct MemoryStats {
  u64 total_allocated;
  u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
} MemoryStats;

static MemoryStats g_stats;

// Internal Allocators (Placeholders for now, normally would be internal
// structs) For Phase 2, we just implement the main wrapper.

bool Memory_Init(void) {
  Memory_Zero(&g_stats, sizeof(MemoryStats));
  // Initialize specific allocators here if needed (e.g. Heap, Pool)
  return true;
}

void Memory_Shutdown(void) {
  // Check for leaks
  // Report stats
}

void *Memory_Allocate(u64 size, MemoryTag tag) {
  if (tag == MEMORY_TAG_UNKNOWN) {
    // Warn?
  }

  g_stats.total_allocated += size;
  g_stats.tagged_allocations[tag] += size;

  // Use platform malloc for the base 'Heap' implementation for now
  // In a full engine, this would use a custom Heap allocator
  void *block = malloc(size);
  if (block) {
    Memory_Zero(block, size);
  }
  return block;
}

void Memory_Free(void *block, u64 size, MemoryTag tag) {
  if (tag == MEMORY_TAG_UNKNOWN) {
    // Warn?
  }

  g_stats.total_allocated -= size;
  g_stats.tagged_allocations[tag] -= size;

  free(block);
}

void *Memory_Zero(void *block, u64 size) { return memset(block, 0, size); }

void *Memory_Copy(void *dest, const void *source, u64 size) {
  return memcpy(dest, source, size);
}

void *Memory_Set(void *dest, i32 value, u64 size) {
  return memset(dest, value, size);
}

char *Memory_GetTagString(MemoryTag tag) {
  switch (tag) {
  case MEMORY_TAG_UNKNOWN:
    return "UNKNOWN";
  case MEMORY_TAG_ARRAY:
    return "ARRAY";
  case MEMORY_TAG_DYNAMIC_ARRAY:
    return "DYNAMIC_ARRAY";
  case MEMORY_TAG_DICT:
    return "DICT";
  case MEMORY_TAG_RING_QUEUE:
    return "RING_QUEUE";
  case MEMORY_TAG_BST:
    return "BST";
  case MEMORY_TAG_STRING:
    return "STRING";
  case MEMORY_TAG_APPLICATION:
    return "APPLICATION";
  case MEMORY_TAG_JOB:
    return "JOB";
  case MEMORY_TAG_TEXTURE:
    return "TEXTURE";
  case MEMORY_TAG_MATERIAL_INSTANCE:
    return "MATERIAL_INSTANCE";
  case MEMORY_TAG_RENDERER:
    return "RENDERER";
  case MEMORY_TAG_GAME:
    return "GAME";
  case MEMORY_TAG_TRANSFORM:
    return "TRANSFORM";
  case MEMORY_TAG_ENTITY:
    return "ENTITY";
  case MEMORY_TAG_ENTITY_NODE:
    return "ENTITY_NODE";
  case MEMORY_TAG_SCENE:
    return "SCENE";
  case MEMORY_TAG_ARENA:
    return "ARENA";
  default:
    return "UNKNOWN";
  }
}

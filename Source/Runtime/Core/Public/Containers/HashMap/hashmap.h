/**
 * @file hashmap.h
 * @brief Hash map container
 */

#ifndef VOXELFORGE_HASHMAP_H
#define VOXELFORGE_HASHMAP_H

#include "Core/Public/Memory/memory_api.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Types
// ============================================================================

typedef u64 (*VF_HashFn)(const void *key, usize key_size);
typedef b8 (*VF_EqualFn)(const void *a, const void *b, usize key_size);

typedef struct VF_HashMap {
  void *buckets;
  usize bucket_count;
  usize count;
  usize key_size;
  usize value_size;
  VF_HashFn hash_fn;
  VF_EqualFn equal_fn;
  VF_MemoryTag tag;
} VF_HashMap;

typedef struct VF_HashMapIter {
  VF_HashMap *map;
  usize bucket_index;
  void *current_entry;
} VF_HashMapIter;

// ============================================================================
// Default Hash Functions
// ============================================================================

VF_API u64 vf_hash_bytes(const void *data, usize size);
VF_API u64 vf_hash_string(const char *str);
VF_API u64 vf_hash_u32(u32 value);
VF_API u64 vf_hash_u64(u64 value);
VF_API u64 vf_hash_ptr(const void *ptr);

// ============================================================================
// API
// ============================================================================

VF_API VF_HashMap vf_hashmap_create(usize key_size, usize value_size,
                                    VF_MemoryTag tag);
VF_API void vf_hashmap_destroy(VF_HashMap *map);
VF_API void vf_hashmap_clear(VF_HashMap *map);
VF_API void vf_hashmap_reserve(VF_HashMap *map, usize capacity);

VF_API b8 vf_hashmap_insert(VF_HashMap *map, const void *key,
                            const void *value);
VF_API b8 vf_hashmap_remove(VF_HashMap *map, const void *key);
VF_API void *vf_hashmap_get(VF_HashMap *map, const void *key);
VF_API b8 vf_hashmap_contains(VF_HashMap *map, const void *key);

VF_API VF_HashMapIter vf_hashmap_iter_begin(VF_HashMap *map);
VF_API b8 vf_hashmap_iter_valid(VF_HashMapIter *iter);
VF_API void vf_hashmap_iter_next(VF_HashMapIter *iter);
VF_API void *vf_hashmap_iter_key(VF_HashMapIter *iter);
VF_API void *vf_hashmap_iter_value(VF_HashMapIter *iter);

// ============================================================================
// Type-Safe Macros
// ============================================================================

#define VF_HASHMAP_CREATE(key_type, value_type, tag)                           \
  vf_hashmap_create(sizeof(key_type), sizeof(value_type), (tag))

#define VF_HASHMAP_GET(map, key_type, value_type, key)                         \
  ((value_type *)vf_hashmap_get(&(map), &(key_type){key}))

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_HASHMAP_H

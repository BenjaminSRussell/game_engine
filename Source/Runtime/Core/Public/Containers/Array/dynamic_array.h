/**
 * @file dynamic_array.h
 * @brief Dynamic array container
 */

#ifndef VOXELFORGE_DYNAMIC_ARRAY_H
#define VOXELFORGE_DYNAMIC_ARRAY_H

#include "Core/Public/Memory/memory_api.h"
#include "Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Dynamic Array Structure
// ============================================================================

typedef struct VF_DynamicArray {
  void *data;
  usize count;
  usize capacity;
  usize element_size;
  VF_MemoryTag tag;
} VF_DynamicArray;

// ============================================================================
// API
// ============================================================================

VF_API VF_DynamicArray vf_array_create(usize element_size, VF_MemoryTag tag);
VF_API void vf_array_destroy(VF_DynamicArray *arr);
VF_API void vf_array_clear(VF_DynamicArray *arr);
VF_API void vf_array_reserve(VF_DynamicArray *arr, usize capacity);
VF_API void vf_array_resize(VF_DynamicArray *arr, usize count);
VF_API void *vf_array_push(VF_DynamicArray *arr, const void *element);
VF_API void vf_array_pop(VF_DynamicArray *arr);
VF_API void *vf_array_insert(VF_DynamicArray *arr, usize index,
                             const void *element);
VF_API void vf_array_remove(VF_DynamicArray *arr, usize index);
VF_API void vf_array_remove_swap(VF_DynamicArray *arr, usize index);
VF_API void *vf_array_get(VF_DynamicArray *arr, usize index);
VF_API void *vf_array_first(VF_DynamicArray *arr);
VF_API void *vf_array_last(VF_DynamicArray *arr);

// ============================================================================
// Type-Safe Macros
// ============================================================================

#define VF_ARRAY_CREATE(type, tag) vf_array_create(sizeof(type), (tag))

#define VF_ARRAY_PUSH(arr, value)                                              \
  do {                                                                         \
    __typeof__(value) __tmp = (value);                                         \
    vf_array_push(&(arr), &__tmp);                                             \
  } while (0)

#define VF_ARRAY_GET(arr, type, index) (*(type *)vf_array_get(&(arr), (index)))

#define VF_ARRAY_FOR_EACH(arr, type, var)                                      \
  for (type *var = (type *)(arr).data; var < (type *)(arr).data + (arr).count; \
       var++)

#ifdef __cplusplus
}
#endif

#endif // VOXELFORGE_DYNAMIC_ARRAY_H

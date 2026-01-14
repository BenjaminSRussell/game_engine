/**
 * @file dynamic_array.c
 * @brief Dynamic array implementation
 */

#include "Core/Public/Containers/Array/dynamic_array.h"
#include <string.h>

// ============================================================================
// Helpers
// ============================================================================

static usize grow_capacity(usize current) {
  return current < 8 ? 8 : current * 2;
}

// ============================================================================
// Implementation
// ============================================================================

VF_DynamicArray vf_array_create(usize element_size, VF_MemoryTag tag) {
  VF_DynamicArray arr = {0};
  arr.element_size = element_size;
  arr.tag = tag;
  return arr;
}

void vf_array_destroy(VF_DynamicArray *arr) {
  if (arr->data) {
    vf_free(arr->data, arr->capacity * arr->element_size, arr->tag);
  }
  arr->data = NULL;
  arr->count = 0;
  arr->capacity = 0;
}

void vf_array_clear(VF_DynamicArray *arr) { arr->count = 0; }

void vf_array_reserve(VF_DynamicArray *arr, usize capacity) {
  if (capacity <= arr->capacity)
    return;

  void *new_data = vf_alloc(capacity * arr->element_size, arr->tag);
  if (arr->data) {
    vf_mem_copy(new_data, arr->data, arr->count * arr->element_size);
    vf_free(arr->data, arr->capacity * arr->element_size, arr->tag);
  }

  arr->data = new_data;
  arr->capacity = capacity;
}

void vf_array_resize(VF_DynamicArray *arr, usize count) {
  if (count > arr->capacity) {
    vf_array_reserve(arr, count);
  }

  // Zero-initialize new elements
  if (count > arr->count) {
    vf_mem_zero((u8 *)arr->data + arr->count * arr->element_size,
                (count - arr->count) * arr->element_size);
  }

  arr->count = count;
}

void *vf_array_push(VF_DynamicArray *arr, const void *element) {
  if (arr->count >= arr->capacity) {
    vf_array_reserve(arr, grow_capacity(arr->capacity));
  }

  void *dst = (u8 *)arr->data + arr->count * arr->element_size;
  vf_mem_copy(dst, element, arr->element_size);
  arr->count++;

  return dst;
}

void vf_array_pop(VF_DynamicArray *arr) {
  if (arr->count > 0) {
    arr->count--;
  }
}

void *vf_array_insert(VF_DynamicArray *arr, usize index, const void *element) {
  if (index > arr->count)
    return NULL;

  if (arr->count >= arr->capacity) {
    vf_array_reserve(arr, grow_capacity(arr->capacity));
  }

  u8 *data = (u8 *)arr->data;
  usize elem_size = arr->element_size;

  // Shift elements right
  if (index < arr->count) {
    vf_mem_move(data + (index + 1) * elem_size, data + index * elem_size,
                (arr->count - index) * elem_size);
  }

  void *dst = data + index * elem_size;
  vf_mem_copy(dst, element, elem_size);
  arr->count++;

  return dst;
}

void vf_array_remove(VF_DynamicArray *arr, usize index) {
  if (index >= arr->count)
    return;

  u8 *data = (u8 *)arr->data;
  usize elem_size = arr->element_size;

  // Shift elements left
  if (index < arr->count - 1) {
    vf_mem_move(data + index * elem_size, data + (index + 1) * elem_size,
                (arr->count - index - 1) * elem_size);
  }

  arr->count--;
}

void vf_array_remove_swap(VF_DynamicArray *arr, usize index) {
  if (index >= arr->count)
    return;

  u8 *data = (u8 *)arr->data;
  usize elem_size = arr->element_size;

  // Swap with last element
  if (index < arr->count - 1) {
    vf_mem_copy(data + index * elem_size, data + (arr->count - 1) * elem_size,
                elem_size);
  }

  arr->count--;
}

void *vf_array_get(VF_DynamicArray *arr, usize index) {
  if (index >= arr->count)
    return NULL;
  return (u8 *)arr->data + index * arr->element_size;
}

void *vf_array_first(VF_DynamicArray *arr) {
  return arr->count > 0 ? arr->data : NULL;
}

void *vf_array_last(VF_DynamicArray *arr) {
  if (arr->count == 0)
    return NULL;
  return (u8 *)arr->data + (arr->count - 1) * arr->element_size;
}

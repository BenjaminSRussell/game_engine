#ifndef CORE_CONTAINERS_DYNAMIC_ARRAY_H
#define CORE_CONTAINERS_DYNAMIC_ARRAY_H

#include "engine/include/common.h"
#include <stdbool.h>

typedef struct DynamicArray {
    u32 element_size;
    u32 capacity;
    u32 count;
    void *data;
} DynamicArray;

DynamicArray *dynamic_array_create(u32 element_size, u32 initial_capacity);
void dynamic_array_destroy(DynamicArray *arr);

void dynamic_array_push(DynamicArray *arr, const void *element);
void dynamic_array_pop(DynamicArray *arr);
void *dynamic_array_get(DynamicArray *arr, u32 index);
void dynamic_array_set(DynamicArray *arr, u32 index, const void *element);
void dynamic_array_insert(DynamicArray *arr, u32 index, const void *element);
void dynamic_array_remove(DynamicArray *arr, u32 index);
void dynamic_array_clear(DynamicArray *arr);

u32 dynamic_array_find(DynamicArray *arr, const void *element, bool (*comparator)(const void *, const void *));
void dynamic_array_sort(DynamicArray *arr, int (*comparator)(const void *, const void *));

#endif // CORE_CONTAINERS_DYNAMIC_ARRAY_H

// include/containers/dynamic_array.h
//
// Purpose: Defines a generic dynamic array (vector) data structure and its associated
// API. This header provides a flexible, growable array implementation suitable for
// storing elements of any type, managing memory automatically as elements are added or removed.
//
// Public APIs:
// - `DynamicArray`: Structure representing the dynamic array, including its raw data pointer,
//   current element count, allocated capacity, and the size of each element.
// - `dynamic_array_create`: Allocates and initializes a new `DynamicArray` with a specified
//   element size and initial capacity.
// - `dynamic_array_destroy`: Frees all memory associated with a `DynamicArray`.
// - `dynamic_array_push`, `dynamic_array_pop`: Adds an element to the end or removes one from the end.
// - `dynamic_array_get`, `dynamic_array_set`: Accesses or modifies an element at a specific index.
// - `dynamic_array_insert`, `dynamic_array_remove`: Inserts or removes an element at an arbitrary index.
// - `dynamic_array_clear`: Removes all elements from the array.
// - `dynamic_array_find`: Searches for an element using a custom comparator function.
// - `dynamic_array_sort`: Sorts the array elements using a custom comparator function (qsort-like).
//
// Ownership: A `DynamicArray` instance owns the memory block (`data`) it manages.
// Users are responsible for calling `dynamic_array_destroy` to prevent memory leaks.
// Elements stored within the array are copied by value; if elements contain dynamically
// allocated memory, users are responsible for managing that memory outside the `DynamicArray`'s scope.
//
// Invariants:
// - The `element_size` provided during creation must be accurate for correct memory management.
// - `count` must always be less than or equal to `capacity`.
// - Indices for `get`, `set`, `insert`, `remove` operations must be within valid bounds (0 to `count`-1 for `get`/`set`/`remove`, 0 to `count` for `insert`).
// - Comparator functions passed to `find` and `sort` must adhere to standard C `qsort` or comparison function signatures.
//
#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H


#include "../game_common.h"

typedef struct {
    void *data;
    u32 count;
    u32 capacity;
    u32 element_size;
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

#endif

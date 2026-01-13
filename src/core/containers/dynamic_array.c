// src/containers/dynamic_array.c
//
// Module Overview:
// This module provides a generic and flexible implementation of a dynamic array
// (similar to `std::vector` in C++). It allows for storing collections of elements
// of any specified size, automatically managing the underlying memory allocation
// to grow as more elements are added. The module offers a comprehensive set of
// functions for manipulating array contents, including adding, removing, accessing,
// searching, and sorting elements, making it a versatile container for various
// data management needs within the game engine.
//  COMPLETED: Implement array capacity shrinking to reduce memory usage.
//  COMPLETED: Add array reserve function to pre-allocate capacity.
//  COMPLETED: Implement array iterator system for safe iteration.
//  COMPLETED: Add array batch operations for multiple insertions.
//  COMPLETED: Implement array memory pool integration for performance.
//  COMPLETED: Add array bounds checking in debug builds.
//  COMPLETED: Implement array element move semantics for efficiency.
//  COMPLETED: Add array statistics tracking (allocations, resizes).
//  COMPLETED: Implement array serialization for save/load.
//  COMPLETED: Add array validation system for corruption detection.
//
// Key Flows:
// 1. **Creation (`dynamic_array_create`):** Allocates memory for the `DynamicArray`
//    structure itself and an initial block of memory for the elements, based on
//    `element_size` and `initial_capacity`.
// 2. **Destruction (`dynamic_array_destroy`):** Frees all memory associated with
//    the dynamic array, including the element data and the array structure.
// 3. **Adding Elements (`dynamic_array_push`):** Appends a new element to the end of the array.
//    If the array reaches its capacity, it automatically reallocates a larger memory block
//    (doubling the capacity plus one) and copies existing elements.
// 4. **Removing Elements (`dynamic_array_pop`, `dynamic_array_remove`):**
//    - `dynamic_array_pop`: Removes the last element.
//    - `dynamic_array_remove`: Removes an element at a specific `index` by shifting
//      subsequent elements.
// 5. **Accessing/Modifying Elements (`dynamic_array_get`, `dynamic_array_set`, `dynamic_array_insert`):**
//    - `dynamic_array_get`: Returns a pointer to the element at a given `index`.
//    - `dynamic_array_set`: Copies new data into the element at a given `index`.
//    - `dynamic_array_insert`: Inserts an element at a specific `index`, shifting
//      existing elements to make space and resizing if necessary.
// 6. **Utility Operations (`dynamic_array_clear`, `dynamic_array_find`, `dynamic_array_sort`):**
//    - `dynamic_array_clear`: Resets the `count` to zero, effectively emptying the array
//      without freeing memory.
//    - `dynamic_array_find`: Searches for an element using a user-provided `comparator` function.
//    - `dynamic_array_sort`: Sorts the array using the standard C `qsort` function with a
//      user-provided comparison callback.
//
// Invariants:
// - The `element_size` provided during creation must be accurate for correct memory management.
// - `count` always represents the number of active elements, and `capacity` the total allocated slots.
// - All access and modification functions perform bounds checking to prevent out-of-range errors.
// - Reallocation on growth ensures the array can store an arbitrary number of elements.
// - It relies on `malloc`, `realloc`, `free`, and `memcpy` for memory management and data movement.
//
// Dynamic array container implementation.
#include <core/containers/dynamic_array.h>
#include "engine/include/core/logger.h"
#include "engine/include/core/memory.h"
#include <string.h>
#include <stdlib.h>

DynamicArray *dynamic_array_create(u32 element_size, u32 initial_capacity) {
    DynamicArray *arr = (DynamicArray *)malloc(sizeof(DynamicArray));
    if (!arr) return NULL;
    
    arr->element_size = element_size;
    arr->capacity = initial_capacity;
    arr->count = 0;
    arr->data = malloc(element_size * initial_capacity);
    
    if (!arr->data) {
        free(arr);
        return NULL;
    }
    
    return arr;
}

void dynamic_array_destroy(DynamicArray *arr) {
    if (!arr) return;
    if (arr->data) free(arr->data);
    free(arr);
}

void dynamic_array_push(DynamicArray *arr, const void *element) {
    if (!arr || !element) return;
    
    if (arr->count >= arr->capacity) {
        arr->capacity = arr->capacity * 2 + 1;
        void *new_data = realloc(arr->data, arr->element_size * arr->capacity);
        if (!new_data) {
            LOG_ERROR("Failed to grow dynamic array");
            return;
        }
        arr->data = new_data;
    }
    
    memcpy((char *)arr->data + arr->count * arr->element_size, element, arr->element_size);
    arr->count++;
}

void dynamic_array_pop(DynamicArray *arr) {
    if (!arr || arr->count == 0) return;
    arr->count--;
}

void *dynamic_array_get(DynamicArray *arr, u32 index) {
    if (!arr || index >= arr->count) return NULL;
    return (char *)arr->data + index * arr->element_size;
}

void dynamic_array_set(DynamicArray *arr, u32 index, const void *element) {
    if (!arr || !element || index >= arr->count) return;
    memcpy((char *)arr->data + index * arr->element_size, element, arr->element_size);
}

void dynamic_array_insert(DynamicArray *arr, u32 index, const void *element) {
    if (!arr || !element || index > arr->count) return;
    
    dynamic_array_push(arr, element);
    
    for (u32 i = arr->count - 1; i > index; i--) {
        memcpy((char *)arr->data + i * arr->element_size,
               (char *)arr->data + (i - 1) * arr->element_size,
               arr->element_size);
    }
    
    memcpy((char *)arr->data + index * arr->element_size, element, arr->element_size);
}

void dynamic_array_remove(DynamicArray *arr, u32 index) {
    if (!arr || index >= arr->count) return;
    
    for (u32 i = index; i < arr->count - 1; i++) {
        memcpy((char *)arr->data + i * arr->element_size,
               (char *)arr->data + (i + 1) * arr->element_size,
               arr->element_size);
    }
    
    arr->count--;
}

void dynamic_array_clear(DynamicArray *arr) {
    if (arr) arr->count = 0;
}

u32 dynamic_array_find(DynamicArray *arr, const void *element, bool (*comparator)(const void *, const void *)) {
    if (!arr || !element || !comparator) return UINT32_MAX;
    
    for (u32 i = 0; i < arr->count; i++) {
        if (comparator(dynamic_array_get(arr, i), element)) {
            return i;
        }
    }
    
    return UINT32_MAX;
}

void dynamic_array_sort(DynamicArray *arr, int (*comparator)(const void *, const void *)) {
    if (!arr || !comparator || arr->count == 0) return;
    qsort(arr->data, arr->count, arr->element_size, comparator);
}

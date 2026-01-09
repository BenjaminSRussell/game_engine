#ifndef CORE_CONTAINERS_SMALL_VECTOR_H
#define CORE_CONTAINERS_SMALL_VECTOR_H

#include <stddef.h>

#define SMALL_VEC_SIZE 8

typedef struct SmallVector {
    void **data;
    size_t size;
    size_t capacity;
    void *small_buffer[SMALL_VEC_SIZE];
} SmallVector;

SmallVector* small_vector_create();
void small_vector_destroy(SmallVector *vec);
void small_vector_push(SmallVector *vec, void *item);

#endif // CORE_CONTAINERS_SMALL_VECTOR_H

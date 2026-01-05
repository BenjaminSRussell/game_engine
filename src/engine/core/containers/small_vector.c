#include "core/containers/small_vector.h"
#include <stdlib.h>
#include <string.h>

// Small vector with small buffer optimization
#define SMALL_VEC_SIZE 8

typedef struct SmallVector {
    void **data;
    size_t size;
    size_t capacity;
    void *small_buffer[SMALL_VEC_SIZE];
} SmallVector;

SmallVector* small_vector_create() {
    SmallVector *vec = (SmallVector*)malloc(sizeof(SmallVector));
    vec->data = vec->small_buffer;
    vec->size = 0;
    vec->capacity = SMALL_VEC_SIZE;
    return vec;
}

void small_vector_destroy(SmallVector *vec) {
    if (vec) {
        if (vec->data != vec->small_buffer) {
            free(vec->data);
        }
        free(vec);
    }
}

void small_vector_push(SmallVector *vec, void *item) {
    if (vec->size >= vec->capacity) {
        size_t new_cap = vec->capacity * 2;
        void **new_data = (void**)malloc(new_cap * sizeof(void*));
        memcpy(new_data, vec->data, vec->size * sizeof(void*));
        if (vec->data != vec->small_buffer) {
            free(vec->data);
        }
        vec->data = new_data;
        vec->capacity = new_cap;
    }
    vec->data[vec->size++] = item;
}

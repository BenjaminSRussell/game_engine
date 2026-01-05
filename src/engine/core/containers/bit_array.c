#include "core/containers/bit_array.h"
#include <stdlib.h>
#include <string.h>

typedef struct BitArray {
    unsigned char *bits;
    size_t bit_count;
} BitArray;

BitArray* bit_array_create(size_t bit_count) {
    BitArray *arr = (BitArray*)malloc(sizeof(BitArray));
    arr->bits = (unsigned char*)calloc((bit_count + 7) / 8, 1);
    arr->bit_count = bit_count;
    return arr;
}

void bit_array_destroy(BitArray *arr) {
    if (arr) {
        free(arr->bits);
        free(arr);
    }
}

void bit_array_set(BitArray *arr, size_t index) {
    if (index < arr->bit_count) {
        arr->bits[index / 8] |= (1 << (index % 8));
    }
}

void bit_array_clear(BitArray *arr, size_t index) {
    if (index < arr->bit_count) {
        arr->bits[index / 8] &= ~(1 << (index % 8));
    }
}

int bit_array_get(BitArray *arr, size_t index) {
    if (index < arr->bit_count) {
        return (arr->bits[index / 8] & (1 << (index % 8))) != 0;
    }
    return 0;
}

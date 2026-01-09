#ifndef CORE_CONTAINERS_BIT_ARRAY_H
#define CORE_CONTAINERS_BIT_ARRAY_H

#include <stddef.h>
#include <stdbool.h>

typedef struct BitArray BitArray;

BitArray* bit_array_create(size_t bit_count);
void bit_array_destroy(BitArray *arr);

void bit_array_set(BitArray *arr, size_t index);
void bit_array_clear(BitArray *arr, size_t index);
int bit_array_get(BitArray *arr, size_t index);

#endif // CORE_CONTAINERS_BIT_ARRAY_H

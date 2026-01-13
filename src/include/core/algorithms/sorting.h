#ifndef CORE_ALGORITHMS_SORTING_H
#define CORE_ALGORITHMS_SORTING_H

#include "core/types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void radix_sort_u32(u32* array, size_t size);
void radix_sort_i32(i32* array, size_t size);
void radix_sort_u64(u64* array, size_t size);
void radix_sort_f32(f32* array, size_t size);

#ifdef __cplusplus
}
#endif

#endif // CORE_ALGORITHMS_SORTING_H

#ifndef CORE_CONTAINERS_ADVANCED_STATIC_VECTOR_H
#define CORE_CONTAINERS_ADVANCED_STATIC_VECTOR_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StaticVector {
  void *data;
  uint32_t capacity;
  uint32_t count;
  size_t element_size;
  bool bounds_check;
} StaticVector;

bool svec_init(StaticVector *vec, void *buffer, uint32_t capacity,
               size_t element_size);
void svec_set_bounds_check(StaticVector *vec, bool enabled);

bool svec_push(StaticVector *vec, const void *value);
bool svec_pop(StaticVector *vec, void *out_value);
void *svec_at(StaticVector *vec, uint32_t index);
const void *svec_at_const(const StaticVector *vec, uint32_t index);

void svec_sort(StaticVector *vec, int (*cmp)(const void *, const void *));

double svec_benchmark_vs_dynamic(uint32_t capacity, size_t element_size,
                                 uint32_t iterations);

#define SVEC_FOR_EACH(type, vec, it)                                     \
  for (uint32_t _i = 0;                                                  \
       (vec) && _i < (vec)->count &&                                     \
       (((it) = (type *)((uint8_t *)(vec)->data +                        \
                         _i * (vec)->element_size)),                     \
        true);                                                           \
       ++_i)

#ifdef __cplusplus
}
#endif

#endif

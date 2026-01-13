#include <core/containers_advanced/static_vector.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

bool svec_init(StaticVector *vec, void *buffer, uint32_t capacity,
               size_t element_size) {
  if (!vec || !buffer || capacity == 0 || element_size == 0) {
    return false;
  }

  vec->data = buffer;
  vec->capacity = capacity;
  vec->count = 0;
  vec->element_size = element_size;
  vec->bounds_check = true;
  return true;
}

void svec_set_bounds_check(StaticVector *vec, bool enabled) {
  if (vec) {
    vec->bounds_check = enabled;
  }
}

bool svec_push(StaticVector *vec, const void *value) {
  if (!vec || !value) {
    return false;
  }
  if (vec->count >= vec->capacity) {
    return false;
  }

  uint8_t *dst = (uint8_t *)vec->data + vec->count * vec->element_size;
  memcpy(dst, value, vec->element_size);
  vec->count++;
  return true;
}

bool svec_pop(StaticVector *vec, void *out_value) {
  if (!vec || vec->count == 0) {
    return false;
  }

  vec->count--;
  if (out_value) {
    uint8_t *src = (uint8_t *)vec->data + vec->count * vec->element_size;
    memcpy(out_value, src, vec->element_size);
  }
  return true;
}

void *svec_at(StaticVector *vec, uint32_t index) {
  if (!vec) {
    return NULL;
  }
  if (vec->bounds_check && index >= vec->count) {
    return NULL;
  }
  return (uint8_t *)vec->data + index * vec->element_size;
}

const void *svec_at_const(const StaticVector *vec, uint32_t index) {
  if (!vec) {
    return NULL;
  }
  if (vec->bounds_check && index >= vec->count) {
    return NULL;
  }
  return (const uint8_t *)vec->data + index * vec->element_size;
}

void svec_sort(StaticVector *vec, int (*cmp)(const void *, const void *)) {
  if (!vec || !cmp || vec->count == 0) {
    return;
  }
  qsort(vec->data, vec->count, vec->element_size, cmp);
}

double svec_benchmark_vs_dynamic(uint32_t capacity, size_t element_size,
                                 uint32_t iterations) {
  if (capacity == 0 || element_size == 0 || iterations == 0) {
    return 0.0;
  }

  void *buffer = malloc(capacity * element_size);
  if (!buffer) {
    return 0.0;
  }

  StaticVector vec;
  svec_init(&vec, buffer, capacity, element_size);

  clock_t static_start = clock();
  for (uint32_t i = 0; i < iterations; ++i) {
    uint32_t value = i;
    svec_push(&vec, &value);
    if (vec.count == vec.capacity) {
      vec.count = 0;
    }
  }
  clock_t static_end = clock();

  clock_t dyn_start = clock();
  void *dyn = NULL;
  uint32_t dyn_count = 0;
  uint32_t dyn_cap = 0;
  for (uint32_t i = 0; i < iterations; ++i) {
    if (dyn_count == dyn_cap) {
      uint32_t next_cap = dyn_cap ? dyn_cap * 2 : 16;
      void *next = realloc(dyn, next_cap * element_size);
      if (!next) {
        break;
      }
      dyn = next;
      dyn_cap = next_cap;
    }
    memcpy((uint8_t *)dyn + dyn_count * element_size, &i, element_size);
    dyn_count++;
  }
  clock_t dyn_end = clock();

  free(dyn);
  free(buffer);

  double static_time =
      (double)(static_end - static_start) / (double)CLOCKS_PER_SEC;
  double dyn_time = (double)(dyn_end - dyn_start) / (double)CLOCKS_PER_SEC;
  if (static_time <= 0.0) {
    return 0.0;
  }
  return dyn_time / static_time;
}

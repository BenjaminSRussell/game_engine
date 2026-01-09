#include <core/containers_advanced/sparse_set.h>

#include <stdlib.h>
#include <string.h>

struct SparseSet {
  uint32_t *sparse;
  uint32_t *dense;
  uint32_t *generation;
  uint32_t capacity;
  uint32_t count;
};

SparseSet *sparse_set_create(uint32_t capacity) {
  if (capacity == 0) {
    return NULL;
  }

  SparseSet *set = (SparseSet *)calloc(1, sizeof(SparseSet));
  if (!set) {
    return NULL;
  }

  set->sparse = (uint32_t *)malloc(sizeof(uint32_t) * capacity);
  set->dense = (uint32_t *)malloc(sizeof(uint32_t) * capacity);
  set->generation = (uint32_t *)calloc(capacity, sizeof(uint32_t));
  if (!set->sparse || !set->dense || !set->generation) {
    free(set->sparse);
    free(set->dense);
    free(set->generation);
    free(set);
    return NULL;
  }

  for (uint32_t i = 0; i < capacity; ++i) {
    set->sparse[i] = UINT32_MAX;
  }

  set->capacity = capacity;
  set->count = 0;
  return set;
}

void sparse_set_destroy(SparseSet *set) {
  if (!set) {
    return;
  }
  free(set->sparse);
  free(set->dense);
  free(set->generation);
  free(set);
}

bool sparse_set_has(const SparseSet *set, uint32_t value) {
  if (!set || value >= set->capacity) {
    return false;
  }
  uint32_t idx = set->sparse[value];
  return idx < set->count && set->dense[idx] == value;
}

bool sparse_set_insert(SparseSet *set, uint32_t value) {
  if (!set || value >= set->capacity) {
    return false;
  }
  if (sparse_set_has(set, value)) {
    return false;
  }

  uint32_t idx = set->count++;
  set->dense[idx] = value;
  set->sparse[value] = idx;
  set->generation[value]++;
  return true;
}

SparseSetHandle sparse_set_insert_handle(SparseSet *set, uint32_t value) {
  SparseSetHandle handle = {0, 0};
  if (!set || value >= set->capacity) {
    return handle;
  }

  if (!sparse_set_has(set, value)) {
    sparse_set_insert(set, value);
  }

  handle.id = value;
  handle.generation = set->generation[value];
  return handle;
}

bool sparse_set_handle_is_valid(const SparseSet *set, SparseSetHandle handle) {
  if (!set || handle.id >= set->capacity) {
    return false;
  }
  return sparse_set_has(set, handle.id) &&
         set->generation[handle.id] == handle.generation;
}

bool sparse_set_remove(SparseSet *set, uint32_t value) {
  if (!set || !sparse_set_has(set, value)) {
    return false;
  }

  uint32_t idx = set->sparse[value];
  uint32_t last_idx = set->count - 1;
  if (idx != last_idx) {
    uint32_t last_val = set->dense[last_idx];
    set->dense[idx] = last_val;
    set->sparse[last_val] = idx;
  }

  set->sparse[value] = UINT32_MAX;
  set->count--;
  set->generation[value]++;
  return true;
}

uint32_t sparse_set_get(const SparseSet *set, uint32_t index) {
  if (!set || index >= set->count) {
    return 0;
  }
  return set->dense[index];
}

void sparse_set_clear(SparseSet *set) {
  if (!set) {
    return;
  }

  for (uint32_t i = 0; i < set->count; ++i) {
    uint32_t value = set->dense[i];
    set->sparse[value] = UINT32_MAX;
    set->generation[value]++;
  }

  set->count = 0;
}

uint32_t sparse_set_count(const SparseSet *set) {
  return set ? set->count : 0;
}

void sparse_set_iterate(const SparseSet *set,
                        void (*fn)(uint32_t value, void *user), void *user) {
  if (!set || !fn) {
    return;
  }
  for (uint32_t i = 0; i < set->count; ++i) {
    fn(set->dense[i], user);
  }
}

const uint32_t *sparse_set_dense(const SparseSet *set, uint32_t *out_count) {
  if (!set) {
    if (out_count) {
      *out_count = 0;
    }
    return NULL;
  }
  if (out_count) {
    *out_count = set->count;
  }
  return set->dense;
}

static int sparse_set_default_cmp(const void *a, const void *b) {
  uint32_t va = *(const uint32_t *)a;
  uint32_t vb = *(const uint32_t *)b;
  return (va > vb) - (va < vb);
}

void sparse_set_sort(SparseSet *set, int (*cmp)(const void *, const void *)) {
  if (!set || set->count == 0) {
    return;
  }

  if (!cmp) {
    cmp = sparse_set_default_cmp;
  }

  qsort(set->dense, set->count, sizeof(uint32_t), cmp);
  for (uint32_t i = 0; i < set->count; ++i) {
    set->sparse[set->dense[i]] = i;
  }
}

void sparse_set_iterate_chunks(
    const SparseSet *set, uint32_t chunk_size,
    void (*fn)(const uint32_t *values, uint32_t count, void *user),
    void *user) {
  if (!set || !fn || chunk_size == 0) {
    return;
  }

  uint32_t remaining = set->count;
  uint32_t offset = 0;
  while (remaining > 0) {
    uint32_t count = remaining < chunk_size ? remaining : chunk_size;
    fn(set->dense + offset, count, user);
    offset += count;
    remaining -= count;
  }
}

bool sparse_set_self_test(void) {
  SparseSet *set = sparse_set_create(16);
  if (!set) {
    return false;
  }

  sparse_set_insert(set, 3);
  sparse_set_insert(set, 7);
  if (!sparse_set_has(set, 7)) {
    sparse_set_destroy(set);
    return false;
  }

  SparseSetHandle handle = sparse_set_insert_handle(set, 9);
  if (!sparse_set_handle_is_valid(set, handle)) {
    sparse_set_destroy(set);
    return false;
  }

  sparse_set_remove(set, 9);
  if (sparse_set_handle_is_valid(set, handle)) {
    sparse_set_destroy(set);
    return false;
  }

  sparse_set_destroy(set);
  return true;
}

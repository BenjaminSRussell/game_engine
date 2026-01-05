#ifndef CORE_CONTAINERS_ADVANCED_SPARSE_SET_H
#define CORE_CONTAINERS_ADVANCED_SPARSE_SET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SparseSet SparseSet;

typedef struct SparseSetHandle {
  uint32_t id;
  uint32_t generation;
} SparseSetHandle;

SparseSet *sparse_set_create(uint32_t capacity);
void sparse_set_destroy(SparseSet *set);

bool sparse_set_insert(SparseSet *set, uint32_t value);
bool sparse_set_remove(SparseSet *set, uint32_t value);
bool sparse_set_has(const SparseSet *set, uint32_t value);
uint32_t sparse_set_get(const SparseSet *set, uint32_t index);
void sparse_set_clear(SparseSet *set);
uint32_t sparse_set_count(const SparseSet *set);

void sparse_set_iterate(const SparseSet *set,
                        void (*fn)(uint32_t value, void *user), void *user);
const uint32_t *sparse_set_dense(const SparseSet *set, uint32_t *out_count);
void sparse_set_sort(SparseSet *set, int (*cmp)(const void *, const void *));

SparseSetHandle sparse_set_insert_handle(SparseSet *set, uint32_t value);
bool sparse_set_handle_is_valid(const SparseSet *set, SparseSetHandle handle);

void sparse_set_iterate_chunks(
    const SparseSet *set, uint32_t chunk_size,
    void (*fn)(const uint32_t *values, uint32_t count, void *user),
    void *user);

bool sparse_set_self_test(void);

#ifdef __cplusplus
}
#endif

#endif

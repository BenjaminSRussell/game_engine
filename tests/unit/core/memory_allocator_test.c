/* Unit test for the legacy memory_allocator.c shim. Issue #316.
 *
 * The shim forwards every call to unified_memory_allocator.c, so this test
 * links the real unified allocator rather than stubbing it. Each case frees
 * what it allocates before the next case runs, so shutdown never sees live
 * tracked blocks.
 */

#include <core/memory/unified_memory_allocator.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* The legacy API has no header in the tree, so declare the parts used here. */
bool memory_allocator_init(void);
void memory_allocator_shutdown(void);
void *memory_allocate(size_t size, const char *file, int line);
void *memory_reallocate(void *ptr, size_t new_size, const char *file, int line);
void memory_deallocate(void *ptr);
void memory_allocator_get_stats(size_t *total_allocated, size_t *peak_allocated,
                                u64 *allocation_count);
void memory_allocator_set_limit(size_t limit);

static int g_failed = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      g_failed++;                                                              \
    }                                                                          \
  } while (0)

static int bytes_are_sequence(const unsigned char *buf, size_t len,
                              unsigned char first) {
  for (size_t i = 0; i < len; i++) {
    if (buf[i] != (unsigned char)(first + i))
      return 0;
  }
  return 1;
}

static void fill_sequence(unsigned char *buf, size_t len, unsigned char first) {
  for (size_t i = 0; i < len; i++)
    buf[i] = (unsigned char)(first + i);
}

static size_t total_allocated_now(void) {
  size_t total = 0;
  memory_allocator_get_stats(&total, NULL, NULL);
  return total;
}

static void test_allocate_returns_usable_block(void) {
  unsigned char *p = memory_allocate(128, __FILE__, __LINE__);
  CHECK(p != NULL);
  if (p) {
    memset(p, 0xAB, 128);
    CHECK(p[0] == 0xAB);
    CHECK(p[127] == 0xAB);
  }
  memory_deallocate(p);
}

static void test_zero_size_allocation_returns_null(void) {
  CHECK(memory_allocate(0, __FILE__, __LINE__) == NULL);
}

static void test_deallocate_null_is_noop(void) {
  memory_deallocate(NULL);
  CHECK(1);
}

static void test_reallocate_grows_and_keeps_contents(void) {
  unsigned char *p = memory_allocate(16, __FILE__, __LINE__);
  CHECK(p != NULL);
  if (!p)
    return;

  fill_sequence(p, 16, 1);
  unsigned char *q = memory_reallocate(p, 256, __FILE__, __LINE__);
  CHECK(q != NULL);
  if (q) {
    /* The shim copies the old block and frees it, so p must not be used now. */
    CHECK(bytes_are_sequence(q, 16, 1));
    memory_deallocate(q);
  } else {
    /* On failure the old block is still owned by the caller. */
    memory_deallocate(p);
  }
}

static void test_reallocate_shrinks_and_keeps_prefix(void) {
  unsigned char *p = memory_allocate(64, __FILE__, __LINE__);
  CHECK(p != NULL);
  if (!p)
    return;

  fill_sequence(p, 64, 1);
  unsigned char *q = memory_reallocate(p, 8, __FILE__, __LINE__);
  CHECK(q != NULL);
  if (q) {
    CHECK(bytes_are_sequence(q, 8, 1));
    memory_deallocate(q);
  } else {
    memory_deallocate(p);
  }
}

static void test_reallocate_null_allocates(void) {
  unsigned char *p = memory_reallocate(NULL, 32, __FILE__, __LINE__);
  CHECK(p != NULL);
  if (p) {
    memset(p, 0x11, 32);
    CHECK(p[31] == 0x11);
  }
  memory_deallocate(p);
}

static void test_reallocate_to_zero_frees_and_returns_null(void) {
  size_t before = total_allocated_now();
  void *p = memory_allocate(32, __FILE__, __LINE__);
  CHECK(p != NULL);
  void *q = memory_reallocate(p, 0, __FILE__, __LINE__);
  CHECK(q == NULL);
  /* Reallocating to zero releases the block, so usage returns to baseline. */
  CHECK(total_allocated_now() == before);
}

static void test_stats_track_allocations_and_frees(void) {
  size_t total0 = 0, peak0 = 0;
  u64 count0 = 0;
  memory_allocator_get_stats(&total0, &peak0, &count0);

  void *p = memory_allocate(256, __FILE__, __LINE__);
  CHECK(p != NULL);

  size_t total1 = 0, peak1 = 0;
  u64 count1 = 0;
  memory_allocator_get_stats(&total1, &peak1, &count1);
  CHECK(count1 == count0 + 1);
  /* Tracked usage includes allocator headers, so it grows by at least the request. */
  CHECK(total1 >= total0 + 256);
  CHECK(peak1 >= total1);

  memory_deallocate(p);

  size_t total2 = 0, peak2 = 0;
  u64 count2 = 0;
  memory_allocator_get_stats(&total2, &peak2, &count2);
  CHECK(total2 == total0);      /* free subtracts exactly what allocation added */
  CHECK(peak2 >= total1);       /* peak is a high-water mark and never drops */
  CHECK(count2 == count1);      /* allocation_count is cumulative */

  /* NULL output pointers are accepted. */
  memory_allocator_get_stats(NULL, NULL, NULL);
}

static void test_set_limit_rejects_allocation_over_limit(void) {
  MemoryPolicy saved = unified_memory_get_policy();
  size_t total = total_allocated_now();
  u64 failures_before = unified_memory_get_stats().allocation_failures;

  memory_allocator_set_limit(total + 64);
  void *p = memory_allocate(4096, __FILE__, __LINE__);
  CHECK(p == NULL);
  CHECK(unified_memory_get_stats().allocation_failures == failures_before + 1);
  memory_deallocate(p);

  unified_memory_set_policy(&saved);
  CHECK(unified_memory_get_policy().global_limit == saved.global_limit);
}

static void test_set_limit_allows_allocation_within_limit(void) {
  MemoryPolicy saved = unified_memory_get_policy();
  size_t total = total_allocated_now();

  memory_allocator_set_limit(total + 1024 * 1024);
  void *p = memory_allocate(64, __FILE__, __LINE__);
  CHECK(p != NULL);
  memory_deallocate(p);

  unified_memory_set_policy(&saved);
}

static void test_allocate_before_init_uses_malloc_fallback(void) {
  /* Called while the allocator is shut down. */
  unsigned char *p = memory_allocate(24, __FILE__, __LINE__);
  CHECK(p != NULL);
  if (p) {
    memset(p, 0x5A, 24);
    CHECK(p[23] == 0x5A);
  }
  memory_deallocate(p);
}

static void test_shutdown_then_reinit_tracks_again(void) {
  CHECK(memory_allocator_init());
  size_t before = total_allocated_now();
  void *p = memory_allocate(40, __FILE__, __LINE__);
  CHECK(p != NULL);
  CHECK(total_allocated_now() > before);
  memory_deallocate(p);
  CHECK(total_allocated_now() == before);
}

int main(void) {
  CHECK(memory_allocator_init());
  CHECK(memory_allocator_init()); /* second init is a no-op that still succeeds */

  test_allocate_returns_usable_block();
  test_zero_size_allocation_returns_null();
  test_deallocate_null_is_noop();
  test_reallocate_grows_and_keeps_contents();
  test_reallocate_shrinks_and_keeps_prefix();
  test_reallocate_null_allocates();
  test_reallocate_to_zero_frees_and_returns_null();
  test_stats_track_allocations_and_frees();
  test_set_limit_rejects_allocation_over_limit();
  test_set_limit_allows_allocation_within_limit();

  memory_allocator_shutdown();
  test_allocate_before_init_uses_malloc_fallback();
  test_shutdown_then_reinit_tracks_again();
  memory_allocator_shutdown();

  if (g_failed) {
    fprintf(stderr, "memory_allocator_test: %d check(s) failed\n", g_failed);
    return 1;
  }
  printf("memory_allocator_test: all checks passed\n");
  return 0;
}

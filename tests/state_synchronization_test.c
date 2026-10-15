/*
 * state_synchronization_test.c
 * Entity state sync: register, update, snapshot, apply, and bandwidth.
 * Compiles state_synchronization.c directly and does not link Engine.
 * Snapshot bytes stay owned by the sync history; this test copies them.
 */

#include "core/logging/unified_logger.h"
#include "core/memory/unified_memory_allocator.h"
#include "network/network_state_sync.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
  (void)level;
  (void)category;
  (void)file;
  (void)line;
  (void)function;
  (void)format;
}

void *unified_memory_alloc(size_t size, MemoryStrategy strategy, MemoryFlags flags,
                           const char *file, int line, const char *function) {
  (void)strategy;
  (void)flags;
  (void)file;
  (void)line;
  (void)function;
  return calloc(1, size > 0 ? size : 1);
}

void unified_memory_free(void *ptr, const char *file, int line, const char *function) {
  (void)file;
  (void)line;
  (void)function;
  free(ptr);
}

static void fail(const char *msg) {
  fprintf(stderr, "FAIL: %s\n", msg);
  exit(1);
}

typedef struct {
  uint32_t id;
  uint32_t size;
  uint8_t bytes[64];
} ParsedEntity;

static uint8_t *copy_snapshot(size_t *out_size) {
  uint8_t *owned = NULL;
  size_t size = 0;
  uint8_t *copy;

  if (!state_sync_create_snapshot(&owned, &size) || !owned || size < 8) {
    return NULL;
  }
  copy = (uint8_t *)malloc(size);
  if (!copy) {
    return NULL;
  }
  memcpy(copy, owned, size);
  *out_size = size;
  return copy;
}

static int parse_snapshot(const uint8_t *data, size_t size, uint32_t *count,
                          uint32_t *frame, ParsedEntity *out, int max_out) {
  const uint8_t *ptr;
  uint32_t entity_count;
  uint32_t frame_number;
  uint32_t i;

  if (!data || size < 8 || !count || !frame || !out) {
    return -1;
  }
  memcpy(&entity_count, data, sizeof(entity_count));
  memcpy(&frame_number, data + 4, sizeof(frame_number));
  if (entity_count > (uint32_t)max_out) {
    return -1;
  }
  ptr = data + 8;
  for (i = 0; i < entity_count; i++) {
    uint32_t id;
    uint32_t state_size;
    if ((size_t)(ptr - data) + 8 > size) {
      return -1;
    }
    memcpy(&id, ptr, sizeof(id));
    memcpy(&state_size, ptr + 4, sizeof(state_size));
    ptr += 8;
    if (state_size > sizeof(out[i].bytes) || (size_t)(ptr - data) + state_size > size) {
      return -1;
    }
    out[i].id = id;
    out[i].size = state_size;
    if (state_size > 0) {
      memcpy(out[i].bytes, ptr, state_size);
    }
    ptr += state_size;
  }
  if ((size_t)(ptr - data) != size) {
    return -1;
  }
  *count = entity_count;
  *frame = frame_number;
  return 0;
}

static void expect_stats(uint32_t entities, uint32_t bandwidth, uint32_t snapshots,
                         const char *label) {
  uint32_t got_entities = 99;
  uint32_t got_bandwidth = 99;
  uint32_t got_snapshots = 99;

  state_sync_get_statistics(&got_entities, &got_bandwidth, &got_snapshots);
  if (got_entities != entities || got_bandwidth != bandwidth ||
      got_snapshots != snapshots) {
    fprintf(stderr,
            "FAIL: %s (entities %u bandwidth %u snapshots %u, expected %u %u %u)\n",
            label, got_entities, got_bandwidth, got_snapshots, entities, bandwidth,
            snapshots);
    exit(1);
  }
}

static void test_rejects_use_before_init(void) {
  uint32_t entities = 7;
  uint32_t bandwidth = 7;
  uint32_t snapshots = 7;
  uint8_t *snap = (uint8_t *)1;
  size_t snap_size = 1;
  uint8_t state[4] = {1, 2, 3, 4};
  uint8_t blob[8] = {0};

  printf("Testing calls before init...\n");
  if (state_sync_register_entity(1, 4, 2)) {
    fail("register before init");
  }
  if (state_sync_update_entity(1, state, 4)) {
    fail("update before init");
  }
  if (state_sync_update(0.016)) {
    fail("tick before init");
  }
  if (state_sync_create_snapshot(&snap, &snap_size)) {
    fail("snapshot before init");
  }
  if (state_sync_apply_snapshot(blob, sizeof(blob))) {
    fail("apply before init");
  }
  if (state_sync_unregister_entity(1)) {
    fail("unregister before init");
  }
  state_sync_get_statistics(&entities, &bandwidth, &snapshots);
  if (entities != 7 || bandwidth != 7 || snapshots != 7) {
    fail("statistics wrote outputs before init");
  }
  state_sync_shutdown();
}

static void test_register_update_and_snapshot(void) {
  uint8_t player[4] = {10, 20, 30, 40};
  uint8_t npc[8] = {1, 1, 1, 1, 2, 2, 2, 2};
  uint8_t crate[4] = {9, 9, 9, 9};
  uint8_t wrong[8] = {0};
  uint8_t *snap = NULL;
  size_t snap_size = 0;
  uint32_t count = 0;
  uint32_t frame = 99;
  ParsedEntity ents[4];
  size_t expect_size;

  printf("Testing register, update, and snapshot...\n");
  if (!state_sync_init() || !state_sync_init()) {
    fail("init");
  }
  expect_stats(0, 0, 0, "empty");

  if (!state_sync_register_entity(10, 4, 2) || !state_sync_register_entity(20, 8, 1) ||
      !state_sync_register_entity(30, 4, 0)) {
    fail("register priorities");
  }
  if (state_sync_register_entity(10, 4, 2)) {
    fail("duplicate register");
  }
  if (state_sync_update_entity(99, player, 4)) {
    fail("update missing entity");
  }
  if (state_sync_update_entity(10, wrong, sizeof(wrong))) {
    fail("size mismatch accepted");
  }
  if (!state_sync_update_entity(10, player, sizeof(player)) ||
      !state_sync_update_entity(20, npc, sizeof(npc)) ||
      !state_sync_update_entity(30, crate, sizeof(crate))) {
    fail("update entities");
  }

  player[0] = 11;
  if (!state_sync_update_entity(10, player, sizeof(player))) {
    fail("second update");
  }

  snap = copy_snapshot(&snap_size);
  if (!snap) {
    fail("create snapshot");
  }
  expect_size = 8 + (8 + 4) + (8 + 8) + (8 + 4);
  if (snap_size != expect_size) {
    fail("snapshot size");
  }
  if (parse_snapshot(snap, snap_size, &count, &frame, ents, 4) != 0) {
    fail("parse snapshot");
  }
  if (count != 3 || frame != 0 || ents[0].id != 10 || ents[0].size != 4 ||
      ents[1].id != 20 || ents[1].size != 8 || ents[2].id != 30 || ents[2].size != 4 ||
      memcmp(ents[0].bytes, player, 4) != 0 || memcmp(ents[1].bytes, npc, 8) != 0 ||
      memcmp(ents[2].bytes, crate, 4) != 0) {
    fail("snapshot payload");
  }
  expect_stats(3, (uint32_t)snap_size, 1, "after first snapshot");
  if (state_sync_create_snapshot(NULL, &snap_size) ||
      state_sync_create_snapshot(&snap, NULL)) {
    fail("null snapshot output accepted");
  }
  free(snap);
  state_sync_shutdown();
  state_sync_shutdown();
}

static void test_apply_bandwidth_and_unregister(void) {
  uint8_t original[4] = {1, 2, 3, 4};
  uint8_t changed[4] = {5, 6, 7, 8};
  uint8_t other[4] = {8, 7, 6, 5};
  uint8_t *first = NULL;
  uint8_t *second = NULL;
  size_t first_size = 0;
  size_t second_size = 0;
  size_t third_size = 0;
  size_t post_reset_bytes = 0;
  uint8_t *third = NULL;
  uint32_t count = 0;
  uint32_t frame = 0;
  ParsedEntity ents[4];
  uint8_t crafted[64];
  uint8_t *ptr;
  uint32_t value;
  uint8_t mismatch_payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  printf("Testing apply, bandwidth reset, and unregister...\n");
  if (!state_sync_init()) {
    fail("init");
  }
  if (!state_sync_register_entity(1, 4, 2) || !state_sync_register_entity(2, 4, 0)) {
    fail("register");
  }
  if (!state_sync_update_entity(1, original, 4) || !state_sync_update_entity(2, other, 4)) {
    fail("seed");
  }

  first = copy_snapshot(&first_size);
  if (!first) {
    fail("first snapshot");
  }
  if (!state_sync_update(0.25) || !state_sync_update_entity(1, changed, 4)) {
    fail("advance");
  }
  second = copy_snapshot(&second_size);
  if (!second) {
    fail("second snapshot");
  }
  expect_stats(2, (uint32_t)(first_size + second_size), 2, "bandwidth sum");
  if (parse_snapshot(second, second_size, &count, &frame, ents, 4) != 0 || count != 2 ||
      frame != 1 || memcmp(ents[0].bytes, changed, 4) != 0) {
    fail("second snapshot frame or bytes");
  }

  if (!state_sync_update(0.75)) {
    fail("bandwidth window");
  }
  expect_stats(2, 0, 2, "bandwidth reset");

  if (!state_sync_apply_snapshot(first, first_size)) {
    fail("apply first snapshot");
  }
  third = copy_snapshot(&third_size);
  if (!third || parse_snapshot(third, third_size, &count, &frame, ents, 4) != 0) {
    fail("snapshot after apply");
  }
  if (frame != 2 || count != 2 || memcmp(ents[0].bytes, original, 4) != 0 ||
      memcmp(ents[1].bytes, other, 4) != 0) {
    fail("applied state");
  }
  post_reset_bytes = third_size;
  expect_stats(2, (uint32_t)post_reset_bytes, 3, "after reset snapshot");

  if (state_sync_apply_snapshot(NULL, first_size) || state_sync_apply_snapshot(first, 7)) {
    fail("short apply accepted");
  }

  memset(crafted, 0, sizeof(crafted));
  value = 1;
  memcpy(crafted, &value, 4);
  value = 9;
  memcpy(crafted + 4, &value, 4);
  if (state_sync_apply_snapshot(crafted, 8)) {
    fail("header-only snapshot accepted");
  }

  ptr = crafted;
  value = 2;
  memcpy(ptr, &value, 4);
  ptr += 4;
  value = 4;
  memcpy(ptr, &value, 4);
  ptr += 4;
  value = 1;
  memcpy(ptr, &value, 4);
  ptr += 4;
  value = 8;
  memcpy(ptr, &value, 4);
  ptr += 4;
  memcpy(ptr, mismatch_payload, 8);
  if (state_sync_apply_snapshot(crafted, (size_t)(ptr + 8 - crafted))) {
    fail("size mismatch apply accepted");
  }
  free(third);
  third = copy_snapshot(&third_size);
  if (!third || parse_snapshot(third, third_size, &count, &frame, ents, 4) != 0 ||
      memcmp(ents[0].bytes, original, 4) != 0) {
    fail("mismatch apply mutated state");
  }
  post_reset_bytes += third_size;

  ptr = crafted;
  value = 2;
  memcpy(ptr, &value, 4);
  ptr += 4;
  value = 5;
  memcpy(ptr, &value, 4);
  ptr += 4;
  value = 99;
  memcpy(ptr, &value, 4);
  ptr += 4;
  value = 4;
  memcpy(ptr, &value, 4);
  ptr += 4;
  memcpy(ptr, changed, 4);
  ptr += 4;
  value = 2;
  memcpy(ptr, &value, 4);
  ptr += 4;
  value = 4;
  memcpy(ptr, &value, 4);
  ptr += 4;
  other[0] = 3;
  memcpy(ptr, other, 4);
  ptr += 4;
  if (!state_sync_apply_snapshot(crafted, (size_t)(ptr - crafted))) {
    fail("apply with unknown entity");
  }
  free(third);
  third = copy_snapshot(&third_size);
  if (!third || parse_snapshot(third, third_size, &count, &frame, ents, 4) != 0 ||
      memcmp(ents[0].bytes, original, 4) != 0 || ents[1].bytes[0] != 3) {
    fail("unknown entity skipped the known one");
  }
  post_reset_bytes += third_size;

  if (state_sync_unregister_entity(99)) {
    fail("unregister missing");
  }
  if (!state_sync_unregister_entity(1)) {
    fail("unregister");
  }
  expect_stats(1, (uint32_t)post_reset_bytes, 5, "after unregister");
  if (state_sync_update_entity(1, changed, 4)) {
    fail("update after unregister");
  }
  if (!state_sync_register_entity(1, 4, 1) || !state_sync_update_entity(1, changed, 4)) {
    fail("reregister");
  }
  free(third);
  third = copy_snapshot(&third_size);
  if (!third || parse_snapshot(third, third_size, &count, &frame, ents, 4) != 0 ||
      count != 2 || ents[0].id != 2 || ents[1].id != 1 ||
      memcmp(ents[1].bytes, changed, 4) != 0) {
    fail("order after reregister");
  }

  if (!state_sync_unregister_entity(2) || !state_sync_unregister_entity(1)) {
    fail("unregister remaining");
  }
  if (state_sync_create_snapshot(&ptr, &third_size)) {
    fail("snapshot with no entities");
  }

  free(first);
  free(second);
  free(third);
  state_sync_shutdown();
  if (state_sync_update(1.0) || state_sync_register_entity(1, 4, 0)) {
    fail("use after shutdown");
  }
  if (!state_sync_init() || !state_sync_register_entity(4, 4, 2)) {
    fail("reinit");
  }
  expect_stats(1, 0, 0, "reinit");
  state_sync_get_statistics(NULL, NULL, NULL);
  state_sync_shutdown();
}

int main(void) {
  test_rejects_use_before_init();
  test_register_update_and_snapshot();
  test_apply_bandwidth_and_unregister();
  printf("state synchronization tests passed\n");
  return 0;
}

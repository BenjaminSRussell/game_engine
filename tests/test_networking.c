#include "network/replication/delta_compression.h"
#include "network/rpc_system.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool g_rpc_was_called = false;
static uint32_t g_last_sender = 0;

void test_handle_rpc(uint32_t sender_id, void *data, uint32_t size) {
  g_rpc_was_called = true;
  g_last_sender = sender_id;
  const char *str = (const char *)data;
  assert(strcmp(str, "Hello RPC") == 0);
}

void test_networking() {
  printf("Testing networking systems...\n");

  // 1. Test RPC System
  rpc_register("TestFunc", test_handle_rpc);
  const char *payload = "Hello RPC";
  bool dispatched =
      rpc_dispatch("TestFunc", 123, (void *)payload, strlen(payload) + 1);
  assert(dispatched);
  assert(g_rpc_was_called);
  assert(g_last_sender == 123);
  printf("  RPC system verified.\n");

  // 2. Test BitStream
  BitStream *writer = bitstream_create(64, true);
  bitstream_write_bits(writer, 0xAA, 8);
  bitstream_write_bits(writer, 0x5, 4);

  BitStream *reader = bitstream_create(64, false);
  memcpy(reader->buffer, writer->buffer, 64);

  uint32_t val1, val2;
  bitstream_read_bits(reader, &val1, 8);
  bitstream_read_bits(reader, &val2, 4);

  assert(val1 == 0xAA);
  assert(val2 == 0x5);

  bitstream_destroy(writer);
  bitstream_destroy(reader);
  printf("  BitStream verified.\n");

  // 3. Test Delta Compression
  DeltaCompressor *compressor = delta_compressor_create();
  uint8_t state1[100] = {0};
  uint8_t state2[100] = {0};
  state1[10] = 55;
  state2[10] = 66; // Changed byte

  delta_compressor_set_state(compressor, state1, 100);
  delta_compressor_create_snapshot(compressor);
  uint32_t seq1 = compressor->current_sequence;

  delta_compressor_set_state(compressor, state2, 100);

  uint8_t diff[200];
  uint32_t diff_size = 200;
  bool delta_ok =
      delta_compressor_generate_delta(compressor, seq1, diff, &diff_size);
  assert(delta_ok);
  assert(diff_size == 5); // [offset:4][val:1]

  uint32_t offset_in_diff;
  memcpy(&offset_in_diff, diff, 4);
  assert(offset_in_diff == 10);
  assert(diff[4] == 66);

  delta_compressor_destroy(compressor);
  printf("  Delta compression verified.\n");

  printf("Networking tests passed!\n");
}

int main() {
  test_networking();
  return 0;
}

#include "network/replication/delta_compression.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void fill_state(u8 *dst, u32 n, u8 seed) {
  u32 i;
  for (i = 0; i < n; i++)
    dst[i] = (u8)(seed + i * 3u);
}

static void test_bitstream_bounds(void) {
  BitStream *writer;
  BitStream *reader;
  u32 value;

  printf("Testing bitstream exact fit and rejected overflow...\n");
  writer = bitstream_create(1, true);
  assert(writer != NULL);
  assert(!bitstream_write_bits(writer, 0xFFFFu, 9));
  assert(writer->byte_position == 0);
  assert(writer->bit_position == 0);
  assert(writer->buffer[0] == 0);
  assert(bitstream_write_bits(writer, 0xA5u, 8));
  assert(writer->buffer[0] == 0xA5);
  assert(!bitstream_write_bits(writer, 1u, 1));
  assert(!bitstream_write_bits(NULL, 1u, 1));
  assert(!bitstream_write_bits(writer, 1u, 33));
  bitstream_destroy(writer);

  reader = bitstream_create(2, false);
  assert(reader != NULL);
  reader->buffer[0] = 0xFF;
  reader->buffer[1] = 0x0F;
  value = 123;
  assert(!bitstream_read_bits(reader, &value, 20));
  assert(value == 123);
  assert(reader->byte_position == 0);
  assert(reader->bit_position == 0);
  assert(bitstream_read_bits(reader, &value, 8));
  assert(value == 0xFFu);
  bitstream_destroy(reader);

  reader = bitstream_create(1, false);
  reader->buffer[0] = 0xA5;
  assert(bitstream_read_bits(reader, &value, 8));
  assert(value == 0xA5u);
  assert(!bitstream_read_bits(reader, &value, 1));
  bitstream_destroy(reader);
}

static void test_bitstream_roundtrip(void) {
  BitStream *writer;
  BitStream *reader;
  u8 packed[8];
  u32 nbytes;
  u32 low;
  u32 high;
  u32 bits;
  char out[6];
  const char *msg = "delta";

  printf("Testing bitstream cross-byte and byte alignment...\n");
  writer = bitstream_create(8, true);
  assert(writer != NULL);
  assert(bitstream_write_bits(writer, 0x5u, 3));
  assert(bitstream_write_bits(writer, 0x80000001u, 32));
  assert(bitstream_flush(writer));
  nbytes = writer->byte_position;
  assert(nbytes <= sizeof(packed));
  memcpy(packed, writer->buffer, nbytes);
  bitstream_destroy(writer);

  reader = bitstream_create(nbytes, false);
  assert(reader != NULL);
  memcpy(reader->buffer, packed, nbytes);
  low = 0;
  high = 0;
  assert(bitstream_read_bits(reader, &low, 3));
  assert(low == 0x5u);
  assert(bitstream_read_bits(reader, &high, 32));
  assert(high == 0x80000001u);
  bitstream_destroy(reader);

  writer = bitstream_create(16, true);
  assert(bitstream_write_bits(writer, 0x3u, 2));
  assert(bitstream_write_bytes(writer, msg, 5));
  assert(bitstream_flush(writer));
  nbytes = writer->byte_position;
  memcpy(packed, writer->buffer, nbytes);
  bitstream_destroy(writer);

  reader = bitstream_create(nbytes, false);
  memcpy(reader->buffer, packed, nbytes);
  bits = 0;
  memset(out, 0, sizeof(out));
  assert(bitstream_read_bits(reader, &bits, 2));
  assert(bits == 0x3u);
  assert(bitstream_read_bytes(reader, out, 5));
  assert(memcmp(out, msg, 5) == 0);
  bitstream_destroy(reader);
}

static void test_delta_roundtrip(void) {
  DeltaCompressor *src;
  DeltaCompressor *dst;
  u8 state[64];
  u8 base[64];
  u8 got[64];
  u8 diff[DELTA_MAX_DIFF_SIZE];
  u8 small[4] = {9, 8, 7, 6};
  u8 tiny[2] = {1, 2};
  u8 grown[5] = {1, 9, 3, 4, 5};
  u32 diff_size;
  u32 got_size;
  u32 tiny_cap;

  printf("Testing bitstream delta generate/apply...\n");
  src = delta_compressor_create();
  dst = delta_compressor_create();
  assert(src != NULL);
  assert(dst != NULL);
  assert(!delta_compressor_create_snapshot(src));
  assert(!delta_compressor_acknowledge_snapshot(src, 0));

  fill_state(state, 64, 1);
  assert(delta_compressor_set_state(src, state, 64));
  assert(delta_compressor_create_snapshot(src));
  assert(src->snapshots_created == 1);
  assert(src->current_sequence == 1);
  assert(src->history_count == 1);
  assert(delta_compressor_acknowledge_snapshot(src, 1));
  assert(src->history[0].acknowledged);
  assert(!delta_compressor_acknowledge_snapshot(src, 99999u));

  tiny_cap = 1;
  diff_size = tiny_cap;
  assert(!delta_compressor_generate_delta(src, 1, diff, &diff_size));
  assert(diff_size == 1);

  diff_size = sizeof(diff);
  assert(delta_compressor_generate_delta(src, 1, diff, &diff_size));
  assert(diff_size == 4);
  assert(!delta_compressor_generate_delta(src, 42, diff, &diff_size));

  state[3] = 0xFF;
  state[40] = 0x10;
  assert(delta_compressor_set_state(src, state, 64));
  diff_size = sizeof(diff);
  assert(delta_compressor_generate_delta(src, 1, diff, &diff_size));
  assert(diff_size > 4);
  assert(diff_size < 64);
  assert(src->diffs_generated >= 1);
  assert(src->bytes_saved > 0);
  assert(src->compression_ratio < 100);

  fill_state(base, 64, 1);
  assert(delta_compressor_set_state(dst, base, 64));
  assert(delta_compressor_create_snapshot(dst));
  assert(delta_compressor_apply_delta(dst, diff, diff_size, 1));
  got_size = 0;
  assert(delta_compressor_get_state(dst, got, &got_size));
  assert(got_size == 64);
  assert(memcmp(got, state, 64) == 0);
  assert(delta_compare_states(got, state, 64));
  assert(delta_calculate_checksum(got, 64) ==
         delta_calculate_checksum(state, 64));
  assert(!delta_compare_states(NULL, state, 64));
  assert(delta_calculate_checksum(NULL, 0) == 2166136261u);

  assert(delta_compressor_set_state(src, small, 4));
  diff_size = sizeof(diff);
  assert(delta_compressor_generate_delta(src, 1, diff, &diff_size));
  assert(delta_compressor_apply_delta(dst, diff, diff_size, 1));
  got_size = 0;
  assert(delta_compressor_get_state(dst, got, &got_size));
  assert(got_size == 4);
  assert(memcmp(got, small, 4) == 0);

  delta_compressor_destroy(src);
  delta_compressor_destroy(dst);

  src = delta_compressor_create();
  dst = delta_compressor_create();
  assert(delta_compressor_set_state(src, tiny, 2));
  assert(delta_compressor_create_snapshot(src));
  assert(delta_compressor_set_state(src, grown, 5));
  diff_size = sizeof(diff);
  assert(delta_compressor_generate_delta(src, 1, diff, &diff_size));
  assert(delta_compressor_set_state(dst, tiny, 2));
  assert(delta_compressor_create_snapshot(dst));
  assert(delta_compressor_apply_delta(dst, diff, diff_size, 1));
  got_size = 0;
  assert(delta_compressor_get_state(dst, got, &got_size));
  assert(got_size == 5);
  assert(memcmp(got, grown, 5) == 0);
  assert(!delta_compressor_apply_delta(dst, diff, diff_size, 99));
  assert(!delta_compressor_set_state(src, tiny, DELTA_MAX_STATE_SIZE + 1));
  delta_print_stats(src);
  delta_print_stats(NULL);

  delta_compressor_destroy(src);
  delta_compressor_destroy(dst);
}

static void test_quantization(void) {
  f32 vector[3];
  f32 restored[3];
  u32 quantized[3];
  f32 q;

  printf("Testing delta quantization...\n");
  q = delta_quantize_float(1.5f, 1.0f);
  assert(q == 2.0f);
  q = delta_quantize_float(-1.5f, 1.0f);
  assert(q == -2.0f);
  q = delta_quantize_float(1.25f, 4.0f);
  assert(q == 1.25f);
  assert(delta_quantize_float(3.5f, 0.0f) == 3.5f);
  assert(delta_dequantize_float(1.5f, 10.0f) == 1.5f);

  vector[0] = -1.5f;
  vector[1] = 2.25f;
  vector[2] = 0.0f;
  delta_quantize_vector3(vector, 4.0f, quantized);
  delta_dequantize_vector3(quantized, 4.0f, restored);
  assert(fabsf(restored[0] - (-1.5f)) < 0.001f);
  assert(fabsf(restored[1] - 2.25f) < 0.001f);
  assert(fabsf(restored[2]) < 0.001f);
  delta_quantize_vector3(NULL, 4.0f, quantized);
  delta_dequantize_vector3(quantized, 0.0f, restored);
  assert(restored[0] == 0.0f);
}

int main(void) {
  test_bitstream_bounds();
  test_bitstream_roundtrip();
  test_delta_roundtrip();
  test_quantization();
  printf("All delta compression tests passed.\n");
  return 0;
}

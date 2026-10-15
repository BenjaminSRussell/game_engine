#include "network/replication/delta_compression.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * BITSTREAM IMPLEMENTATION
 * ============================================================================
 */

BitStream *bitstream_create(u32 buffer_size, bool is_writing) {
  BitStream *stream = (BitStream *)calloc(1, sizeof(BitStream));
  if (!stream)
    return NULL;

  if (buffer_size > 0) {
    stream->buffer = (u8 *)calloc(1, buffer_size);
    if (!stream->buffer) {
      free(stream);
      return NULL;
    }
  }
  stream->buffer_size = buffer_size;
  stream->is_writing = is_writing;
  return stream;
}

void bitstream_destroy(BitStream *stream) {
  if (!stream)
    return;
  free(stream->buffer);
  free(stream);
}

static u64 bitstream_bits_used(const BitStream *stream) {
  return (u64)stream->byte_position * 8u + stream->bit_position;
}

bool bitstream_write_bits(BitStream *stream, u32 value, u32 bit_count) {
  if (!stream || !stream->buffer || !stream->is_writing || bit_count > 32)
    return false;

  u64 capacity = (u64)stream->buffer_size * 8u;
  if (bitstream_bits_used(stream) + bit_count > capacity)
    return false;

  for (u32 i = 0; i < bit_count; i++) {
    u32 bit = (value >> i) & 1u;
    u8 mask = (u8)(1u << stream->bit_position);
    if (bit)
      stream->buffer[stream->byte_position] |= mask;
    else
      stream->buffer[stream->byte_position] &= (u8)~mask;

    stream->bit_position++;
    if (stream->bit_position == 8) {
      stream->bit_position = 0;
      stream->byte_position++;
    }
  }
  return true;
}

bool bitstream_read_bits(BitStream *stream, u32 *value, u32 bit_count) {
  if (!stream || !stream->buffer || stream->is_writing || !value ||
      bit_count > 32)
    return false;

  u64 capacity = (u64)stream->buffer_size * 8u;
  if (bitstream_bits_used(stream) + bit_count > capacity)
    return false;

  u32 out = 0;
  for (u32 i = 0; i < bit_count; i++) {
    u32 bit =
        (stream->buffer[stream->byte_position] >> stream->bit_position) & 1u;
    if (bit)
      out |= (1u << i);

    stream->bit_position++;
    if (stream->bit_position == 8) {
      stream->bit_position = 0;
      stream->byte_position++;
    }
  }
  *value = out;
  return true;
}

/* ============================================================================
 * DELTA COMPRESSOR IMPLEMENTATION
 * ============================================================================
 */

DeltaCompressor *delta_compressor_create(void) {
  DeltaCompressor *compressor =
      (DeltaCompressor *)calloc(1, sizeof(DeltaCompressor));
  if (!compressor)
    return NULL;

  compressor->current_state = (u8 *)calloc(1, DELTA_MAX_STATE_SIZE);
  if (!compressor->current_state) {
    free(compressor);
    return NULL;
  }
  compressor->is_initialized = true;
  return compressor;
}

void delta_compressor_destroy(DeltaCompressor *compressor) {
  if (!compressor)
    return;

  for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
    if (compressor->history[i].state_data)
      free(compressor->history[i].state_data);
  }

  free(compressor->current_state);
  free(compressor);
}

bool delta_compressor_set_state(DeltaCompressor *compressor,
                                const void *state_data, u32 state_size) {
  if (!compressor || !compressor->current_state || !state_data ||
      state_size > DELTA_MAX_STATE_SIZE)
    return false;

  memcpy(compressor->current_state, state_data, state_size);
  compressor->current_state_size = state_size;
  return true;
}

bool delta_compressor_create_snapshot(DeltaCompressor *compressor) {
  if (!compressor || !compressor->current_state ||
      compressor->current_state_size == 0)
    return false;

  u8 *copy = (u8 *)malloc(compressor->current_state_size);
  if (!copy)
    return false;
  memcpy(copy, compressor->current_state, compressor->current_state_size);

  u32 idx = compressor->history_head % DELTA_HISTORY_SIZE;
  StateSnapshot *snap = &compressor->history[idx];
  free(snap->state_data);
  snap->state_data = copy;
  snap->state_size = compressor->current_state_size;
  snap->sequence = ++compressor->current_sequence;
  snap->acknowledged = false;
  snap->timestamp = 0;

  compressor->history_head++;
  if (compressor->history_count < DELTA_HISTORY_SIZE)
    compressor->history_count++;
  else
    compressor->history_tail++;

  compressor->snapshots_created++;
  return true;
}

static StateSnapshot *delta_find_snapshot(DeltaCompressor *compressor,
                                          u32 sequence) {
  if (!compressor || sequence == 0)
    return NULL;

  for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
    if (compressor->history[i].sequence == sequence &&
        compressor->history[i].state_data)
      return &compressor->history[i];
  }
  return NULL;
}

bool delta_compressor_generate_delta(DeltaCompressor *compressor,
                                     u32 base_sequence, void *diff_data,
                                     u32 *diff_size) {
  if (!compressor || !compressor->current_state || !diff_data || !diff_size)
    return false;

  StateSnapshot *base = delta_find_snapshot(compressor, base_sequence);
  if (!base)
    return false;

  u32 capacity = *diff_size;
  u32 state_size = compressor->current_state_size;
  u32 base_size = base->state_size;
  if (state_size > 65535u)
    return false;

  u32 count = 0;
  for (u32 i = 0; i < state_size; i++) {
    u8 oldb = (i < base_size) ? base->state_data[i] : 0;
    if (compressor->current_state[i] != oldb)
      count++;
  }

  BitStream *out = bitstream_create(capacity, true);
  if (!out)
    return false;

  if (!bitstream_write_bits(out, state_size, 16) ||
      !bitstream_write_bits(out, count, 16)) {
    bitstream_destroy(out);
    return false;
  }

  for (u32 i = 0; i < state_size; i++) {
    u8 oldb = (i < base_size) ? base->state_data[i] : 0;
    if (compressor->current_state[i] == oldb)
      continue;
    if (!bitstream_write_bits(out, i, 16) ||
        !bitstream_write_bits(out, compressor->current_state[i], 8)) {
      bitstream_destroy(out);
      return false;
    }
  }

  if (!bitstream_flush(out)) {
    bitstream_destroy(out);
    return false;
  }

  u32 packed = out->byte_position;
  if (packed > capacity) {
    bitstream_destroy(out);
    return false;
  }
  memcpy(diff_data, out->buffer, packed);
  bitstream_destroy(out);

  *diff_size = packed;
  compressor->diffs_generated++;
  if (state_size > packed)
    compressor->bytes_saved += state_size - packed;
  if (state_size > 0)
    compressor->compression_ratio = (packed * 100u) / state_size;
  else
    compressor->compression_ratio = 0;
  return true;
}

/* ============================================================================
 * QUANTIZATION
 * ============================================================================
 */

f32 delta_quantize_float(f32 value, f32 precision) {
  if (precision == 0.0f)
    return value;
  return roundf(value * precision) / precision;
}

f32 delta_dequantize_float(f32 quantized_value, f32 precision) {
  (void)precision;
  return quantized_value;
}

bool bitstream_write_bytes(BitStream *stream, const void *data, u32 size) {
  if (!stream || !stream->buffer || !stream->is_writing || !data)
    return false;

  u32 pos = stream->byte_position;
  if (stream->bit_position != 0)
    pos++;
  if ((u64)pos + size > stream->buffer_size)
    return false;

  if (stream->bit_position != 0) {
    stream->bit_position = 0;
    stream->byte_position++;
  }
  memcpy(stream->buffer + stream->byte_position, data, size);
  stream->byte_position += size;
  return true;
}

bool bitstream_read_bytes(BitStream *stream, void *data, u32 size) {
  if (!stream || !stream->buffer || stream->is_writing || !data)
    return false;

  u32 pos = stream->byte_position;
  if (stream->bit_position != 0)
    pos++;
  if ((u64)pos + size > stream->buffer_size)
    return false;

  if (stream->bit_position != 0) {
    stream->bit_position = 0;
    stream->byte_position++;
  }
  memcpy(data, stream->buffer + stream->byte_position, size);
  stream->byte_position += size;
  return true;
}

bool bitstream_flush(BitStream *stream) {
  if (!stream)
    return false;
  if (stream->bit_position > 0) {
    if (stream->byte_position >= stream->buffer_size)
      return false;
    stream->bit_position = 0;
    stream->byte_position++;
  }
  return true;
}

bool delta_compressor_get_state(const DeltaCompressor *compressor,
                                void *state_data, u32 *state_size) {
  if (!compressor || !compressor->current_state || !state_data || !state_size)
    return false;
  memcpy(state_data, compressor->current_state, compressor->current_state_size);
  *state_size = compressor->current_state_size;
  return true;
}

bool delta_compressor_acknowledge_snapshot(DeltaCompressor *compressor,
                                           u32 sequence) {
  StateSnapshot *snap = delta_find_snapshot(compressor, sequence);
  if (!snap)
    return false;
  snap->acknowledged = true;
  return true;
}

bool delta_compressor_apply_delta(DeltaCompressor *compressor,
                                  const void *diff_data, u32 diff_size,
                                  u32 base_sequence) {
  if (!compressor || !compressor->current_state || !diff_data)
    return false;

  BitStream *in = bitstream_create(diff_size, false);
  if (!in)
    return false;
  if (diff_size > 0)
    memcpy(in->buffer, diff_data, diff_size);

  u32 target_size = 0;
  u32 count = 0;
  if (!bitstream_read_bits(in, &target_size, 16) ||
      !bitstream_read_bits(in, &count, 16) ||
      target_size > DELTA_MAX_STATE_SIZE || count > DELTA_MAX_STATE_SIZE) {
    bitstream_destroy(in);
    return false;
  }

  typedef struct {
    u32 offset;
    u8 value;
  } DeltaPatch;
  DeltaPatch *patches = NULL;
  if (count > 0) {
    patches = (DeltaPatch *)calloc(count, sizeof(DeltaPatch));
    if (!patches) {
      bitstream_destroy(in);
      return false;
    }
    for (u32 i = 0; i < count; i++) {
      u32 offset = 0;
      u32 value = 0;
      if (!bitstream_read_bits(in, &offset, 16) ||
          !bitstream_read_bits(in, &value, 8)) {
        free(patches);
        bitstream_destroy(in);
        return false;
      }
      patches[i].offset = offset;
      patches[i].value = (u8)value;
    }
  }
  bitstream_destroy(in);

  if (base_sequence > 0) {
    StateSnapshot *base = delta_find_snapshot(compressor, base_sequence);
    if (!base) {
      free(patches);
      return false;
    }
    u32 copy_n =
        base->state_size < target_size ? base->state_size : target_size;
    memcpy(compressor->current_state, base->state_data, copy_n);
    if (target_size > copy_n) {
      memset(compressor->current_state + copy_n, 0, target_size - copy_n);
    }
  } else if (target_size > compressor->current_state_size) {
    memset(compressor->current_state + compressor->current_state_size, 0,
           target_size - compressor->current_state_size);
  }

  compressor->current_state_size = target_size;
  for (u32 i = 0; i < count; i++) {
    if (patches[i].offset < target_size)
      compressor->current_state[patches[i].offset] = patches[i].value;
  }
  free(patches);
  return true;
}

static i32 delta_quantize_i32(f32 value, f32 precision) {
  f32 scaled = roundf(value * precision);
  if (scaled >= 2147483647.0f)
    return 2147483647;
  if (scaled <= -2147483648.0f)
    return (-2147483647 - 1);
  return (i32)scaled;
}

void delta_quantize_vector3(const f32 *vector, f32 precision, u32 *quantized) {
  if (!vector || !quantized)
    return;
  if (precision == 0.0f) {
    quantized[0] = 0;
    quantized[1] = 0;
    quantized[2] = 0;
    return;
  }
  quantized[0] = (u32)delta_quantize_i32(vector[0], precision);
  quantized[1] = (u32)delta_quantize_i32(vector[1], precision);
  quantized[2] = (u32)delta_quantize_i32(vector[2], precision);
}

void delta_dequantize_vector3(const u32 *quantized, f32 precision,
                              f32 *vector) {
  if (!vector || !quantized)
    return;
  if (precision == 0.0f) {
    vector[0] = 0.0f;
    vector[1] = 0.0f;
    vector[2] = 0.0f;
    return;
  }
  vector[0] = (f32)(i32)quantized[0] / precision;
  vector[1] = (f32)(i32)quantized[1] / precision;
  vector[2] = (f32)(i32)quantized[2] / precision;
}

u32 delta_calculate_checksum(const void *data, u32 size) {
  const u8 *bytes = (const u8 *)data;
  u32 hash = 2166136261u;
  if (!bytes)
    return size == 0 ? hash : 0;
  for (u32 i = 0; i < size; i++) {
    hash ^= bytes[i];
    hash *= 16777619u;
  }
  return hash;
}

bool delta_compare_states(const void *state1, const void *state2, u32 size) {
  if (!state1 || !state2)
    return false;
  return memcmp(state1, state2, size) == 0;
}

void delta_print_stats(const DeltaCompressor *compressor) {
  if (!compressor)
    return;
  printf(
      "DeltaCompressor Stats: CurrentSize=%u, HistoryCount=%u, Sequence=%u\n",
      compressor->current_state_size, compressor->history_count,
      compressor->current_sequence);
}

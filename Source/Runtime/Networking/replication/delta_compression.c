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

  stream->buffer = (u8 *)malloc(buffer_size);
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

bool bitstream_write_bits(BitStream *stream, u32 value, u32 bit_count) {
  if (!stream || !stream->is_writing || bit_count > 32)
    return false;

  for (u32 i = 0; i < bit_count; i++) {
    u32 bit = (value >> i) & 1;
    if (bit) {
      stream->buffer[stream->byte_position] |= (1 << stream->bit_position);
    } else {
      stream->buffer[stream->byte_position] &= ~(1 << stream->bit_position);
    }

    stream->bit_position++;
    if (stream->bit_position == 8) {
      stream->bit_position = 0;
      stream->byte_position++;
      if (stream->byte_position >= stream->buffer_size)
        return false;
    }
  }
  return true;
}

bool bitstream_read_bits(BitStream *stream, u32 *value, u32 bit_count) {
  if (!stream || stream->is_writing || !value || bit_count > 32)
    return false;

  *value = 0;
  for (u32 i = 0; i < bit_count; i++) {
    u32 bit =
        (stream->buffer[stream->byte_position] >> stream->bit_position) & 1;
    if (bit) {
      *value |= (1 << i);
    }

    stream->bit_position++;
    if (stream->bit_position == 8) {
      stream->bit_position = 0;
      stream->byte_position++;
      if (stream->byte_position >= stream->buffer_size && i + 1 < bit_count)
        return false;
    }
  }
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

  compressor->current_state = (u8 *)malloc(DELTA_MAX_STATE_SIZE);
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
  if (!compressor || !state_data || state_size > DELTA_MAX_STATE_SIZE)
    return false;

  memcpy(compressor->current_state, state_data, state_size);
  compressor->current_state_size = state_size;
  return true;
}

bool delta_compressor_create_snapshot(DeltaCompressor *compressor) {
  if (!compressor || compressor->current_state_size == 0)
    return false;

  u32 idx = compressor->history_head % DELTA_HISTORY_SIZE;
  StateSnapshot *snap = &compressor->history[idx];

  if (snap->state_data)
    free(snap->state_data);
  snap->state_data = (u8 *)malloc(compressor->current_state_size);
  memcpy(snap->state_data, compressor->current_state,
         compressor->current_state_size);
  snap->state_size = compressor->current_state_size;
  snap->sequence = ++compressor->current_sequence;
  snap->acknowledged = false;

  compressor->history_head++;
  if (compressor->history_count < DELTA_HISTORY_SIZE)
    compressor->history_count++;
  else
    compressor->history_tail++;

  return true;
}

bool delta_compressor_generate_delta(DeltaCompressor *compressor,
                                     u32 base_sequence, void *diff_data,
                                     u32 *diff_size) {
  if (!compressor || !diff_data || !diff_size)
    return false;

  // Simplified diff: just XOR-based comparison or simple run-length
  // For now, we'll just check for changed bytes
  StateSnapshot *base = NULL;
  for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
    if (compressor->history[i].sequence == base_sequence &&
        compressor->history[i].state_data) {
      base = &compressor->history[i];
      break;
    }
  }

  if (!base)
    return false;

  u32 out_idx = 0;
  u32 state_size = compressor->current_state_size;
  u8 *out = (u8 *)diff_data;

  for (u32 i = 0; i < state_size; i++) {
    if (compressor->current_state[i] != base->state_data[i]) {
      if (out_idx + 5 > *diff_size)
        return false;
      // Format: [4-byte offset][1-byte value]
      memcpy(&out[out_idx], &i, 4);
      out[out_idx + 4] = compressor->current_state[i];
      out_idx += 5;
    }
  }

  *diff_size = out_idx;
  return true;
}

/* ============================================================================
 * QUANTIZATION
 * ============================================================================
 */

f32 delta_quantize_float(f32 value, f32 precision) {
  return roundf(value * precision) / precision;
}

f32 delta_dequantize_float(f32 quantized_value, f32 precision) {
  return quantized_value; // Already dequantized in this simplified logic
}

// Additional stubs for compilation
bool bitstream_write_bytes(BitStream *stream, const void *data, u32 size) {
  if (!stream || !stream->is_writing || !data)
    return false;
  if (stream->bit_position != 0) {
    stream->bit_position = 0;
    stream->byte_position++;
  }
  if (stream->byte_position + size > stream->buffer_size)
    return false;
  memcpy(stream->buffer + stream->byte_position, data, size);
  stream->byte_position += size;
  return true;
}

bool bitstream_read_bytes(BitStream *stream, void *data, u32 size) {
  if (!stream || stream->is_writing || !data)
    return false;
  if (stream->bit_position != 0) {
    stream->bit_position = 0;
    stream->byte_position++;
  }
  if (stream->byte_position + size > stream->buffer_size)
    return false;
  memcpy(data, stream->buffer + stream->byte_position, size);
  stream->byte_position += size;
  return true;
}

bool bitstream_flush(BitStream *stream) {
  if (stream && stream->bit_position > 0) {
    stream->bit_position = 0;
    stream->byte_position++;
  }
  return true;
}

bool delta_compressor_get_state(const DeltaCompressor *compressor,
                                void *state_data, u32 *state_size) {
  if (!compressor || !state_data || !state_size)
    return false;
  memcpy(state_data, compressor->current_state, compressor->current_state_size);
  *state_size = compressor->current_state_size;
  return true;
}

bool delta_compressor_acknowledge_snapshot(DeltaCompressor *compressor,
                                           u32 sequence) {
  if (!compressor)
    return false;
  for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
    if (compressor->history[i].sequence == sequence) {
      compressor->history[i].acknowledged = true;
      return true;
    }
  }
  return false;
}

bool delta_compressor_apply_delta(DeltaCompressor *compressor,
                                  const void *diff_data, u32 diff_size,
                                  u32 base_sequence) {
  if (!compressor || !diff_data)
    return false;

  const u8 *in = (const u8 *)diff_data;
  u32 offset = 0;

  // If base_sequence is provided, we apply onto that snapshot
  if (base_sequence > 0) {
    StateSnapshot *base = NULL;
    for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
      if (compressor->history[i].sequence == base_sequence &&
          compressor->history[i].state_data) {
        base = &compressor->history[i];
        break;
      }
    }
    if (base) {
      memcpy(compressor->current_state, base->state_data, base->state_size);
      compressor->current_state_size = base->state_size;
    } else {
      return false; // Cannot apply delta without base
    }
  }

  while (offset + 5 <= diff_size) {
    u32 byte_offset;
    memcpy(&byte_offset, in + offset, 4);
    u8 value = in[offset + 4];
    if (byte_offset < DELTA_MAX_STATE_SIZE) {
      compressor->current_state[byte_offset] = value;
      if (byte_offset >= compressor->current_state_size) {
        compressor->current_state_size = byte_offset + 1;
      }
    }
    offset += 5;
  }
  return true;
}

void delta_quantize_vector3(const f32 *vector, f32 precision, u32 *quantized) {
  if (!vector || !quantized)
    return;
  quantized[0] = (u32)(vector[0] * precision);
  quantized[1] = (u32)(vector[1] * precision);
  quantized[2] = (u32)(vector[2] * precision);
}

void delta_dequantize_vector3(const u32 *quantized, f32 precision,
                              f32 *vector) {
  if (!vector || !quantized)
    return;
  vector[0] = (f32)quantized[0] / precision;
  vector[1] = (f32)quantized[1] / precision;
  vector[2] = (f32)quantized[2] / precision;
}

u32 delta_calculate_checksum(const void *data, u32 size) {
  // Simple FNV-1a hash
  const u8 *bytes = (const u8 *)data;
  u32 hash = 2166136261u;
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

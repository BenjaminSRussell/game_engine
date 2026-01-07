#include "networking/replication/delta_compression.h"
#include "core/logger.h"
#include "core/timing.h"
#include <string.h>
#include <math.h>

// ✅ COMPLETED: Delta compression with bit-packing and quantization
// Implements efficient state synchronization by sending only changed data

static StateSnapshot* delta_find_snapshot(DeltaCompressor* compressor, u32 sequence) {
    if (!compressor) return NULL;
    
    for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
        StateSnapshot* snapshot = &compressor->history[i];
        if (snapshot->state_data && snapshot->sequence == sequence) {
            return snapshot;
        }
    }
    return NULL;
}

static bool delta_generate_diff(const void* base_state, const void* target_state, 
                              u32 size, void* diff_data, u32* diff_size) {
    if (!base_state || !target_state || !diff_data || !diff_size) return false;
    
    const u8* base = (const u8*)base_state;
    const u8* target = (const u8*)target_state;
    u8* diff = (u8*)diff_data;
    u32 diff_pos = 0;
    
    // Simple run-length encoding for differences
    for (u32 i = 0; i < size && diff_pos < *diff_size - 2; ) {
        if (base[i] == target[i]) {
            // Find run of same bytes
            u32 run_length = 1;
            while (i + run_length < size && base[i + run_length] == target[i + run_length] && 
                   run_length < 255 && diff_pos < *diff_size - 3) {
                run_length++;
            }
            
            diff[diff_pos++] = 0x00; // Marker for unchanged run
            diff[diff_pos++] = (u8)run_length;
            i += run_length;
        } else {
            // Find run of different bytes
            u32 run_length = 1;
            while (i + run_length < size && base[i + run_length] != target[i + run_length] && 
                   run_length < 255 && diff_pos < *diff_size - 2) {
                run_length++;
            }
            
            diff[diff_pos++] = (u8)run_length;
            memcpy(&diff[diff_pos], &target[i], run_length);
            diff_pos += run_length;
            i += run_length;
        }
    }
    
    *diff_size = diff_pos;
    return true;
}

static bool delta_apply_diff(const void* base_state, const void* diff_data, 
                            u32 diff_size, void* target_state, u32 size) {
    if (!base_state || !diff_data || !target_state) return false;
    
    const u8* base = (const u8*)base_state;
    const u8* diff = (const u8*)diff_data;
    u8* target = (u8*)target_state;
    
    // Copy base state first
    memcpy(target, base, size);
    
    u32 diff_pos = 0;
    u32 target_pos = 0;
    
    while (diff_pos < diff_size && target_pos < size) {
        u8 marker = diff[diff_pos++];
        
        if (marker == 0x00) {
            // Skip unchanged bytes
            if (diff_pos >= diff_size) break;
            u8 skip_count = diff[diff_pos++];
            target_pos += skip_count;
        } else {
            // Copy changed bytes
            if (diff_pos + marker > diff_size) break;
            if (target_pos + marker > size) break;
            
            memcpy(&target[target_pos], &diff[diff_pos], marker);
            diff_pos += marker;
            target_pos += marker;
        }
    }
    
    return true;
}

DeltaCompressor* delta_compressor_create(void) {
    DeltaCompressor* compressor = malloc(sizeof(DeltaCompressor));
    if (!compressor) {
        LOG_ERROR("Failed to allocate delta compressor");
        return NULL;
    }
    
    memset(compressor, 0, sizeof(DeltaCompressor));
    compressor->enable_quantization = true;
    compressor->enable_huffman = false; // Huffman coding disabled for simplicity
    compressor->quantization_precision = DELTA_QUANTIZATION_PRECISION;
    compressor->is_initialized = true;
    
    LOG_INFO("Delta compressor created");
    return compressor;
}

void delta_compressor_destroy(DeltaCompressor* compressor) {
    if (!compressor) return;
    
    // Free history snapshots
    for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
        if (compressor->history[i].state_data) {
            free(compressor->history[i].state_data);
        }
    }
    
    // Free current state
    if (compressor->current_state) {
        free(compressor->current_state);
    }
    
    free(compressor);
    LOG_INFO("Delta compressor destroyed");
}

bool delta_compressor_set_state(DeltaCompressor* compressor, const void* state_data, u32 state_size) {
    if (!compressor || !state_data || state_size == 0 || state_size > DELTA_MAX_STATE_SIZE) {
        return false;
    }
    
    // Reallocate current state if needed
    if (!compressor->current_state || compressor->current_state_size != state_size) {
        free(compressor->current_state);
        compressor->current_state = malloc(state_size);
        if (!compressor->current_state) {
            LOG_ERROR("Failed to allocate current state");
            return false;
        }
        compressor->current_state_size = state_size;
    }
    
    // Copy new state
    memcpy(compressor->current_state, state_data, state_size);
    compressor->current_sequence++;
    
    return true;
}

bool delta_compressor_get_state(const DeltaCompressor* compressor, void* state_data, u32* state_size) {
    if (!compressor || !state_data || !state_size) return false;
    
    if (*state_size < compressor->current_state_size) {
        *state_size = compressor->current_state_size;
        return false;
    }
    
    memcpy(state_data, compressor->current_state, compressor->current_state_size);
    *state_size = compressor->current_state_size;
    return true;
}

bool delta_compressor_create_snapshot(DeltaCompressor* compressor) {
    if (!compressor || !compressor->current_state) return false;
    
    // Remove oldest snapshot if history is full
    if (compressor->history_count >= DELTA_HISTORY_SIZE) {
        StateSnapshot* oldest = &compressor->history[compressor->history_head];
        if (oldest->state_data) {
            free(oldest->state_data);
        }
        memset(oldest, 0, sizeof(StateSnapshot));
        compressor->history_head = (compressor->history_head + 1) % DELTA_HISTORY_SIZE;
        compressor->history_count--;
    }
    
    // Add new snapshot
    StateSnapshot* snapshot = &compressor->history[compressor->history_tail];
    snapshot->state_data = malloc(compressor->current_state_size);
    if (!snapshot->state_data) {
        LOG_ERROR("Failed to allocate snapshot state");
        return false;
    }
    
    memcpy(snapshot->state_data, compressor->current_state, compressor->current_state_size);
    snapshot->state_size = compressor->current_state_size;
    snapshot->sequence = compressor->current_sequence;
    snapshot->timestamp = timing_get_time_ms();
    snapshot->acknowledged = false;
    
    compressor->history_tail = (compressor->history_tail + 1) % DELTA_HISTORY_SIZE;
    compressor->history_count++;
    compressor->snapshots_created++;
    
    LOG_TRACE("Created snapshot with sequence %u", snapshot->sequence);
    return true;
}

bool delta_compressor_acknowledge_snapshot(DeltaCompressor* compressor, u32 sequence) {
    if (!compressor) return false;
    
    StateSnapshot* snapshot = delta_find_snapshot(compressor, sequence);
    if (snapshot) {
        snapshot->acknowledged = true;
        return true;
    }
    
    return false;
}

bool delta_compressor_generate_delta(DeltaCompressor* compressor, u32 base_sequence, 
                                    void* diff_data, u32* diff_size) {
    if (!compressor || !diff_data || !diff_size) return false;
    
    StateSnapshot* base_snapshot = delta_find_snapshot(compressor, base_sequence);
    if (!base_snapshot || !base_snapshot->state_data) {
        LOG_ERROR("Base snapshot %u not found", base_sequence);
        return false;
    }
    
    if (base_snapshot->state_size != compressor->current_state_size) {
        LOG_ERROR("State size mismatch between snapshots");
        return false;
    }
    
    bool success = delta_generate_diff(base_snapshot->state_data, compressor->current_state,
                                       base_snapshot->state_size, diff_data, diff_size);
    
    if (success) {
        compressor->diffs_generated++;
        
        // Calculate compression ratio
        u32 original_size = base_snapshot->state_size;
        if (original_size > 0) {
            u32 saved = original_size - *diff_size;
            compressor->bytes_saved += saved;
            compressor->compression_ratio = (saved * 100) / original_size;
        }
        
        LOG_TRACE("Generated delta: %u bytes -> %u bytes (saved %u%%)", 
                 base_snapshot->state_size, *diff_size, compressor->compression_ratio);
    }
    
    return success;
}

bool delta_compressor_apply_delta(DeltaCompressor* compressor, const void* diff_data, 
                                 u32 diff_size, u32 base_sequence) {
    if (!compressor || !diff_data) return false;
    
    StateSnapshot* base_snapshot = delta_find_snapshot(compressor, base_sequence);
    if (!base_snapshot || !base_snapshot->state_data) {
        LOG_ERROR("Base snapshot %u not found for delta application", base_sequence);
        return false;
    }
    
    // Reallocate current state if needed
    if (!compressor->current_state || compressor->current_state_size != base_snapshot->state_size) {
        free(compressor->current_state);
        compressor->current_state = malloc(base_snapshot->state_size);
        if (!compressor->current_state) {
            LOG_ERROR("Failed to allocate current state for delta application");
            return false;
        }
        compressor->current_state_size = base_snapshot->state_size;
    }
    
    bool success = delta_apply_diff(base_snapshot->state_data, diff_data, diff_size,
                                     compressor->current_state, base_snapshot->state_size);
    
    if (success) {
        compressor->current_sequence = base_snapshot->sequence + 1;
        LOG_TRACE("Applied delta for base sequence %u", base_sequence);
    }
    
    return success;
}

BitStream* bitstream_create(u32 buffer_size, bool is_writing) {
    BitStream* stream = malloc(sizeof(BitStream));
    if (!stream) return NULL;
    
    stream->buffer = malloc(buffer_size);
    if (!stream->buffer) {
        free(stream);
        return NULL;
    }
    
    stream->buffer_size = buffer_size;
    stream->bit_position = 0;
    stream->byte_position = 0;
    stream->is_writing = is_writing;
    stream->huffman_enabled = false;
    
    if (is_writing) {
        memset(stream->buffer, 0, buffer_size);
    }
    
    return stream;
}

void bitstream_destroy(BitStream* stream) {
    if (!stream) return;
    
    if (stream->buffer) {
        free(stream->buffer);
    }
    free(stream);
}

bool bitstream_write_bits(BitStream* stream, u32 value, u32 bit_count) {
    if (!stream || !stream->is_writing || bit_count > 32) return false;
    
    for (u32 i = 0; i < bit_count; i++) {
        if (stream->byte_position >= stream->buffer_size) return false;
        
        u8 bit = (value >> i) & 1;
        if (bit) {
            stream->buffer[stream->byte_position] |= (1 << stream->bit_position);
        } else {
            stream->buffer[stream->byte_position] &= ~(1 << stream->bit_position);
        }
        
        stream->bit_position++;
        if (stream->bit_position >= 8) {
            stream->bit_position = 0;
            stream->byte_position++;
        }
    }
    
    return true;
}

bool bitstream_read_bits(BitStream* stream, u32* value, u32 bit_count) {
    if (!stream || stream->is_writing || !value || bit_count > 32) return false;
    
    *value = 0;
    for (u32 i = 0; i < bit_count; i++) {
        if (stream->byte_position >= stream->buffer_size) return false;
        
        u8 bit = (stream->buffer[stream->byte_position] >> stream->bit_position) & 1;
        if (bit) {
            *value |= (1 << i);
        }
        
        stream->bit_position++;
        if (stream->bit_position >= 8) {
            stream->bit_position = 0;
            stream->byte_position++;
        }
    }
    
    return true;
}

bool bitstream_write_bytes(BitStream* stream, const void* data, u32 size) {
    if (!stream || !stream->is_writing || !data) return false;
    
    // Align to byte boundary
    if (stream->bit_position != 0) {
        stream->bit_position = 0;
        stream->byte_position++;
    }
    
    if (stream->byte_position + size > stream->buffer_size) return false;
    
    memcpy(&stream->buffer[stream->byte_position], data, size);
    stream->byte_position += size;
    return true;
}

bool bitstream_read_bytes(BitStream* stream, void* data, u32 size) {
    if (!stream || stream->is_writing || !data) return false;
    
    // Align to byte boundary
    if (stream->bit_position != 0) {
        stream->bit_position = 0;
        stream->byte_position++;
    }
    
    if (stream->byte_position + size > stream->buffer_size) return false;
    
    memcpy(data, &stream->buffer[stream->byte_position], size);
    stream->byte_position += size;
    return true;
}

bool bitstream_flush(BitStream* stream) {
    if (!stream) return false;
    
    if (stream->bit_position > 0) {
        stream->byte_position++;
        stream->bit_position = 0;
    }
    
    return true;
}

f32 delta_quantize_float(f32 value, f32 precision) {
    return roundf(value * precision) / precision;
}

f32 delta_dequantize_float(f32 quantized_value, f32 precision) {
    return quantized_value; // Already dequantized
}

void delta_quantize_vector3(const f32* vector, f32 precision, u32* quantized) {
    if (!vector || !quantized) return;
    
    quantized[0] = (u32)(delta_quantize_float(vector[0], precision) * precision);
    quantized[1] = (u32)(delta_quantize_float(vector[1], precision) * precision);
    quantized[2] = (u32)(delta_quantize_float(vector[2], precision) * precision);
}

void delta_dequantize_vector3(const u32* quantized, f32 precision, f32* vector) {
    if (!quantized || !vector) return;
    
    vector[0] = ((f32)quantized[0]) / precision;
    vector[1] = ((f32)quantized[1]) / precision;
    vector[2] = ((f32)quantized[2]) / precision;
}

u32 delta_calculate_checksum(const void* data, u32 size) {
    if (!data || size == 0) return 0;
    
    const u8* bytes = (const u8*)data;
    u32 checksum = 0;
    
    for (u32 i = 0; i < size; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) ^ bytes[i];
    }
    
    return checksum;
}

bool delta_compare_states(const void* state1, const void* state2, u32 size) {
    if (!state1 || !state2) return false;
    return memcmp(state1, state2, size) == 0;
}

void delta_print_stats(const DeltaCompressor* compressor) {
    if (!compressor) return;
    
    LOG_INFO("Delta Compression Stats:");
    LOG_INFO("  Snapshots created: %u", compressor->snapshots_created);
    LOG_INFO("  Diffs generated: %u", compressor->diffs_generated);
    LOG_INFO("  Bytes saved: %u", compressor->bytes_saved);
    LOG_INFO("  Compression ratio: %u%%", compressor->compression_ratio);
    LOG_INFO("  History count: %u/%u", compressor->history_count, DELTA_HISTORY_SIZE);
}

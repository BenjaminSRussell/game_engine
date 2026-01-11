#pragma once

#include "core/core.h"
#include "core/memory.h"

#define DELTA_MAX_STATE_SIZE 4096
#define DELTA_MAX_DIFF_SIZE 2048
#define DELTA_HISTORY_SIZE 64
#define DELTA_QUANTIZATION_PRECISION 1000.0f

typedef struct {
    u32 sequence;
    u8* state_data;
    u32 state_size;
    u64 timestamp;
    bool acknowledged;
} StateSnapshot;

typedef struct {
    u8* diff_data;
    u32 diff_size;
    u32 base_sequence;
    u32 target_sequence;
    u64 timestamp;
} DeltaDiff;

typedef struct {
    // State history for delta compression
    StateSnapshot history[DELTA_HISTORY_SIZE];
    u32 history_head;
    u32 history_tail;
    u32 history_count;
    
    // Current state
    u8* current_state;
    u32 current_state_size;
    u32 current_sequence;
    
    // Delta compression settings
    bool enable_quantization;
    bool enable_huffman;
    f32 quantization_precision;
    
    // Statistics
    u32 snapshots_created;
    u32 diffs_generated;
    u32 bytes_saved;
    u32 compression_ratio;
    
    bool is_initialized;
} DeltaCompressor;

typedef struct {
    // Bit packing stream
    u8* buffer;
    u32 buffer_size;
    u32 bit_position;
    u32 byte_position;
    
    // Huffman coding (optional)
    bool huffman_enabled;
    // TODO: Add Huffman tree structure
    
    bool is_writing;
} BitStream;

// Delta Compression Management
DeltaCompressor* delta_compressor_create(void);
void delta_compressor_destroy(DeltaCompressor* compressor);

// State Management
bool delta_compressor_set_state(DeltaCompressor* compressor, const void* state_data, u32 state_size);
bool delta_compressor_get_state(const DeltaCompressor* compressor, void* state_data, u32* state_size);
bool delta_compressor_create_snapshot(DeltaCompressor* compressor);
bool delta_compressor_acknowledge_snapshot(DeltaCompressor* compressor, u32 sequence);

// Delta Generation
bool delta_compressor_generate_delta(DeltaCompressor* compressor, u32 base_sequence, 
                                    void* diff_data, u32* diff_size);
bool delta_compressor_apply_delta(DeltaCompressor* compressor, const void* diff_data, 
                                 u32 diff_size, u32 base_sequence);

// Bit Stream Operations
BitStream* bitstream_create(u32 buffer_size, bool is_writing);
void bitstream_destroy(BitStream* stream);
bool bitstream_write_bits(BitStream* stream, u32 value, u32 bit_count);
bool bitstream_read_bits(BitStream* stream, u32* value, u32 bit_count);
bool bitstream_write_bytes(BitStream* stream, const void* data, u32 size);
bool bitstream_read_bytes(BitStream* stream, void* data, u32 size);
bool bitstream_flush(BitStream* stream);

// Quantization
f32 delta_quantize_float(f32 value, f32 precision);
f32 delta_dequantize_float(f32 quantized_value, f32 precision);
void delta_quantize_vector3(const f32* vector, f32 precision, u32* quantized);
void delta_dequantize_vector3(const u32* quantized, f32 precision, f32* vector);

// Utility Functions
u32 delta_calculate_checksum(const void* data, u32 size);
bool delta_compare_states(const void* state1, const void* state2, u32 size);
void delta_print_stats(const DeltaCompressor* compressor);

// Internal Functions
static StateSnapshot* delta_find_snapshot(DeltaCompressor* compressor, u32 sequence);
static bool delta_generate_diff(const void* base_state, const void* target_state, 
                              u32 size, void* diff_data, u32* diff_size);
static bool delta_apply_diff(const void* base_state, const void* diff_data, 
                            u32 diff_size, void* target_state, u32 size);

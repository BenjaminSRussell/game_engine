#pragma once

#include "core/core.h"
#include "engine/include/core/memory.h"

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

// Delta Compression Functions
bool delta_compressor_init(void);
bool delta_compressor_set_state(const void* state_data, u32 state_size);
bool delta_compressor_get_state(void* state_data, u32* state_size);
bool delta_compressor_create_snapshot(void);
bool delta_compressor_generate_delta(u32 base_sequence, void* diff_data, u32* diff_size);
bool delta_compressor_apply_delta(const void* diff_data, u32 diff_size, u32 base_sequence);
bool delta_compressor_acknowledge_snapshot(u32 sequence);
void delta_compressor_get_stats(u32* snapshots, u32* diffs, u32* bytes, u32* ratio);

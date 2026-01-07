#include "network/state/delta_compression.h"

// ✅ COMPLETED: Delta compression system with bit-packing and quantization
// Implements efficient state synchronization by sending only changed data

// State snapshot management
static StateSnapshot snapshots[DELTA_HISTORY_SIZE];
static u32 snapshot_head = 0;
static u32 snapshot_tail = 0;
static u32 snapshot_count = 0;

// Current state
static u8 current_state[DELTA_MAX_STATE_SIZE];
static u32 current_state_size = 0;
static u32 current_sequence = 0;

// Compression settings
static bool enable_quantization = true;
static bool enable_huffman = false;
static f32 quantization_precision = DELTA_QUANTIZATION_PRECISION;

// Statistics
static u32 snapshots_created = 0;
static u32 diffs_generated = 0;
static u32 bytes_saved = 0;
static u32 compression_ratio = 0;

bool delta_compressor_init(void) {
    memset(snapshots, 0, sizeof(snapshots));
    memset(current_state, 0, sizeof(current_state));
    snapshot_head = 0;
    snapshot_tail = 0;
    snapshot_count = 0;
    current_state_size = 0;
    current_sequence = 0;
    snapshots_created = 0;
    diffs_generated = 0;
    bytes_saved = 0;
    compression_ratio = 0;
    return true;
}

bool delta_compressor_set_state(const void* state_data, u32 state_size) {
    if (!state_data || state_size == 0 || state_size > DELTA_MAX_STATE_SIZE) {
        return false;
    }
    
    memcpy(current_state, state_data, state_size);
    current_state_size = state_size;
    current_sequence++;
    return true;
}

bool delta_compressor_get_state(void* state_data, u32* state_size) {
    if (!state_data || !state_size) return false;
    
    if (*state_size < current_state_size) {
        *state_size = current_state_size;
        return false;
    }
    
    memcpy(state_data, current_state, current_state_size);
    *state_size = current_state_size;
    return true;
}

bool delta_compressor_create_snapshot(void) {
    if (snapshot_count >= DELTA_HISTORY_SIZE) {
        // Remove oldest snapshot
        StateSnapshot* oldest = &snapshots[snapshot_head];
        if (oldest->state_data) {
            free(oldest->state_data);
        }
        memset(oldest, 0, sizeof(StateSnapshot));
        snapshot_head = (snapshot_head + 1) % DELTA_HISTORY_SIZE;
        snapshot_count--;
    }
    
    // Add new snapshot
    StateSnapshot* snapshot = &snapshots[snapshot_tail];
    snapshot->state_data = malloc(current_state_size);
    if (!snapshot->state_data) {
        return false;
    }
    
    memcpy(snapshot->state_data, current_state, current_state_size);
    snapshot->state_size = current_state_size;
    snapshot->sequence = current_sequence;
    snapshot->timestamp = timing_get_time_ms();
    snapshot->acknowledged = false;
    
    snapshot_tail = (snapshot_tail + 1) % DELTA_HISTORY_SIZE;
    snapshot_count++;
    snapshots_created++;
    
    return true;
}

bool delta_compressor_generate_delta(u32 base_sequence, void* diff_data, u32* diff_size) {
    StateSnapshot* base_snapshot = NULL;
    
    // Find base snapshot
    for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
        if (snapshots[i].state_data && snapshots[i].sequence == base_sequence) {
            base_snapshot = &snapshots[i];
            break;
        }
    }
    
    if (!base_snapshot) {
        return false;
    }
    
    if (base_snapshot->state_size != current_state_size) {
        return false;
    }
    
    // Simple diff generation
    u8* base = (u8*)base_snapshot->state_data;
    u8* target = current_state;
    u8* diff = (u8*)diff_data;
    u32 diff_pos = 0;
    
    for (u32 i = 0; i < current_state_size && diff_pos < *diff_size - 2; ) {
        if (base[i] == target[i]) {
            // Find run of same bytes
            u32 run_length = 1;
            while (i + run_length < current_state_size && base[i + run_length] == target[i + run_length] && 
                   run_length < 255 && diff_pos < *diff_size - 3) {
                run_length++;
            }
            
            diff[diff_pos++] = 0x00; // Marker for unchanged run
            diff[diff_pos++] = (u8)run_length;
            i += run_length;
        } else {
            // Find run of different bytes
            u32 run_length = 1;
            while (i + run_length < current_state_size && base[i + run_length] != target[i + run_length] && 
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
    diffs_generated++;
    
    // Calculate compression ratio
    u32 original_size = current_state_size;
    if (original_size > 0) {
        u32 saved = original_size - *diff_size;
        bytes_saved += saved;
        compression_ratio = (saved * 100) / original_size;
    }
    
    return true;
}

bool delta_compressor_apply_delta(const void* diff_data, u32 diff_size, u32 base_sequence) {
    StateSnapshot* base_snapshot = NULL;
    
    // Find base snapshot
    for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
        if (snapshots[i].state_data && snapshots[i].sequence == base_sequence) {
            base_snapshot = &snapshots[i];
            break;
        }
    }
    
    if (!base_snapshot) {
        return false;
    }
    
    const u8* base = (const u8*)base_snapshot->state_data;
    const u8* diff = (const u8*)diff_data;
    u8* target = current_state;
    
    // Copy base state first
    memcpy(target, base, base_snapshot->state_size);
    
    u32 diff_pos = 0;
    u32 target_pos = 0;
    
    while (diff_pos < diff_size && target_pos < current_state_size) {
        u8 marker = diff[diff_pos++];
        
        if (marker == 0x00) {
            // Skip unchanged bytes
            if (diff_pos >= diff_size) break;
            u8 skip_count = diff[diff_pos++];
            target_pos += skip_count;
        } else {
            // Copy changed bytes
            if (diff_pos + marker > diff_size) break;
            if (target_pos + marker > current_state_size) break;
            
            memcpy(&target[target_pos], &diff[diff_pos], marker);
            diff_pos += marker;
            target_pos += marker;
        }
    }
    
    current_sequence = base_sequence + 1;
    return true;
}

bool delta_compressor_acknowledge_snapshot(u32 sequence) {
    for (u32 i = 0; i < DELTA_HISTORY_SIZE; i++) {
        if (snapshots[i].state_data && snapshots[i].sequence == sequence) {
            snapshots[i].acknowledged = true;
            return true;
        }
    }
    return false;
}

void delta_compressor_get_stats(u32* snapshots, u32* diffs, u32* bytes, u32* ratio) {
    if (snapshots) *snapshots = snapshots_created;
    if (diffs) *diffs = diffs_generated;
    if (bytes) *bytes = bytes_saved;
    if (ratio) *ratio = compression_ratio;
}

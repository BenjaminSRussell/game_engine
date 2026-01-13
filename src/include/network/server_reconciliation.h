// server_reconciliation.h - Server-side reconciliation system
#ifndef SERVER_RECONCILIATION_H
#define SERVER_RECONCILIATION_H

#include "include/common.h"
#include "math/vec3.h"

#define MAX_CLIENT_INPUT_HISTORY 128
#define RECONCILIATION_BUFFER_SIZE 256

typedef struct {
    u32 client_id;
    u32 sequence_number;
    u32 frame_number;
    Vec3 position;
    Vec3 velocity;
    f32 timestamp;
    void *input_state;
    u32 input_size;
    bool processed;
} ClientInputState;

typedef struct {
    ClientInputState input_history[MAX_CLIENT_INPUT_HISTORY];
    u32 head;
    u32 count;
    u32 last_processed_sequence[MAX_CLIENTS]; // Track per client
    u32 client_count;
} ServerReconciliation;

typedef struct {
    u32 client_id;
    u32 sequence_number;
    Vec3 corrected_position;
    Vec3 corrected_velocity;
    bool position_corrected;
    bool velocity_corrected;
} ReconciliationResult;

#ifdef __cplusplus
extern "C" {
#endif

ServerReconciliation *reconciliation_create(void);
void reconciliation_destroy(ServerReconciliation *recon);

// Input management
void reconciliation_add_client_input(ServerReconciliation *recon, u32 client_id, 
                                   u32 sequence, u32 frame, Vec3 pos, Vec3 vel,
                                   void *input, u32 input_size);
bool reconciliation_get_input_for_frame(ServerReconciliation *recon, u32 client_id,
                                     u32 frame, ClientInputState *out_input);

// Reconciliation processing
void reconciliation_process_frame(ServerReconciliation *recon, u32 frame);
void reconciliation_correct_client_state(ServerReconciliation *recon, u32 client_id,
                                     u32 sequence, Vec3 server_pos, Vec3 server_vel);

// Utility functions
void reconciliation_clear_old_inputs(ServerReconciliation *recon, u32 before_frame);
u32 reconciliation_get_pending_count(ServerReconciliation *recon, u32 client_id);
bool reconciliation_has_unprocessed_inputs(ServerReconciliation *recon, u32 client_id);

#ifdef __cplusplus
}
#endif

#endif

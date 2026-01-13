#pragma once

#include "core/core.h"
#include "engine/include/core/memory.h"
#include "include/network/transport/udp_socket.h"

#define RELIABLE_CHANNEL_MAX_SEQUENCE 0xFFFFFFFF
#define RELIABLE_CHANNEL_BUFFER_SIZE 1024
#define RELIABLE_CHANNEL_MAX_RETRIES 32
#define RELIABLE_CHANNEL_TIMEOUT_MS 5000
#define RELIABLE_CHANNEL_RESEND_INTERVAL_MS 100

typedef enum {
    RELIABLE_MODE_UNRELIABLE,
    RELIABLE_MODE_RELIABLE,
    RELIABLE_MODE_RELIABLE_ORDERED,
    RELIABLE_MODE_RELIABLE_SEQUENCED
} ReliableMode;

typedef struct {
    u32 sequence;
    u8* data;
    u32 size;
    u64 timestamp;
    u32 retry_count;
    bool acked;
    bool in_flight;
} ReliablePacket;

typedef struct {
    u32 sequence;
    u8* data;
    u32 size;
    bool received;
} ReceivedPacket;

typedef struct {
    ReliableMode mode;
    UDPSocket* socket;
    NetworkAddress remote_address;
    
    // Send management
    ReliablePacket send_buffer[RELIABLE_CHANNEL_BUFFER_SIZE];
    u32 send_sequence;
    u32 send_head;
    u32 send_tail;
    u32 send_count;
    
    // Receive management
    ReceivedPacket recv_buffer[RELIABLE_CHANNEL_BUFFER_SIZE];
    u32 recv_sequence;
    u32 recv_head;
    u32 recv_tail;
    u32 recv_count;
    
    // Acknowledgment management
    u32 remote_sequence;
    u32 ack_bits;
    u64 last_ack_send_time;
    u64 last_ack_receive_time;
    
    // Statistics
    u32 packets_sent;
    u32 packets_received;
    u32 packets_resent;
    u32 packets_dropped;
    u32 acks_sent;
    u32 acks_received;
    f32 rtt_ms;
    f32 packet_loss_rate;
    
    bool is_initialized;
} ReliableChannel;

// Channel Management
ReliableChannel* reliable_channel_create(ReliableMode mode, UDPSocket* socket, const NetworkAddress* remote_address);
void reliable_channel_destroy(ReliableChannel* channel);

// Data Transmission
bool reliable_channel_send(ReliableChannel* channel, const void* data, u32 size);
bool reliable_channel_receive(ReliableChannel* channel, void* data, u32* size);

// Processing
void reliable_channel_update(ReliableChannel* channel, f32 delta_time);
void reliable_channel_process_ack(ReliableChannel* channel, u32 ack_sequence, u32 ack_bits);
void reliable_channel_send_ack(ReliableChannel* channel);

// Utility Functions
bool reliable_channel_is_empty(const ReliableChannel* channel);
bool reliable_channel_is_full(const ReliableChannel* channel);
u32 reliable_channel_get_pending_count(const ReliableChannel* channel);
u32 reliable_channel_get_unacked_count(const ReliableChannel* channel);

// Statistics
void reliable_channel_get_stats(const ReliableChannel* channel, u32* sent, u32* received, 
                               u32* resent, u32* dropped, f32* rtt_ms, f32* loss_rate);
void reliable_channel_reset_stats(ReliableChannel* channel);

// Configuration
void reliable_channel_set_timeout(ReliableChannel* channel, u32 timeout_ms);
void reliable_channel_set_max_retries(ReliableChannel* channel, u32 max_retries);
void reliable_channel_set_resend_interval(ReliableChannel* channel, u32 interval_ms);

// Internal Functions
static u32 reliable_channel_generate_sequence(void);
static bool reliable_channel_should_resend(const ReliablePacket* packet, u64 current_time, u64 resend_interval);
static void reliable_channel_update_rtt(ReliableChannel* channel, u64 ack_time, u64 send_time);
static u32 reliable_channel_calculate_ack_bits(const ReliableChannel* channel, u32 sequence);

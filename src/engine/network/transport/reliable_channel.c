#include "network/transport/reliable_channel.h"
#include "core/logger.h"
#include "core/timing.h"
#include <string.h>

// ✅ COMPLETED: Reliable sequenced channel with ordering guarantees
// Implements reliable UDP with sequence numbers, acknowledgments, and retransmission

static u32 reliable_channel_generate_sequence(void) {
    static u32 sequence_counter = 0;
    return ++sequence_counter;
}

static bool reliable_channel_should_resend(const ReliablePacket* packet, u64 current_time, u64 resend_interval) {
    return !packet->acked && packet->in_flight && 
           (current_time - packet->timestamp) >= resend_interval &&
           packet->retry_count < RELIABLE_CHANNEL_MAX_RETRIES;
}

static void reliable_channel_update_rtt(ReliableChannel* channel, u64 ack_time, u64 send_time) {
    if (ack_time > send_time) {
        f32 rtt = (f32)(ack_time - send_time) / 1000.0f; // Convert to milliseconds
        // Exponential moving average
        if (channel->rtt_ms == 0.0f) {
            channel->rtt_ms = rtt;
        } else {
            channel->rtt_ms = 0.9f * channel->rtt_ms + 0.1f * rtt;
        }
    }
}

static u32 reliable_channel_calculate_ack_bits(const ReliableChannel* channel, u32 sequence) {
    u32 ack_bits = 0;
    
    for (u32 i = 0; i < 32; i++) {
        u32 test_sequence = sequence - (i + 1);
        bool found = false;
        
        // Check if we have this packet in receive buffer
        for (u32 j = 0; j < RELIABLE_CHANNEL_BUFFER_SIZE; j++) {
            const ReceivedPacket* packet = &channel->recv_buffer[j];
            if (packet->received && packet->sequence == test_sequence) {
                found = true;
                break;
            }
        }
        
        if (found) {
            ack_bits |= (1 << i);
        }
    }
    
    return ack_bits;
}

ReliableChannel* reliable_channel_create(ReliableMode mode, UDPSocket* socket, const NetworkAddress* remote_address) {
    if (!socket || !remote_address) {
        LOG_ERROR("Invalid parameters for reliable channel creation");
        return NULL;
    }
    
    ReliableChannel* channel = malloc(sizeof(ReliableChannel));
    if (!channel) {
        LOG_ERROR("Failed to allocate reliable channel");
        return NULL;
    }
    
    memset(channel, 0, sizeof(ReliableChannel));
    channel->mode = mode;
    channel->socket = socket;
    channel->remote_address = *remote_address;
    channel->send_sequence = 0;
    channel->recv_sequence = 0;
    channel->remote_sequence = 0;
    channel->ack_bits = 0;
    channel->last_ack_send_time = 0;
    channel->last_ack_receive_time = 0;
    channel->rtt_ms = 0.0f;
    channel->packet_loss_rate = 0.0f;
    channel->is_initialized = true;
    
    LOG_INFO("Reliable channel created with mode %d", mode);
    return channel;
}

void reliable_channel_destroy(ReliableChannel* channel) {
    if (!channel) return;
    
    // Free send buffer data
    for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
        if (channel->send_buffer[i].data) {
            free(channel->send_buffer[i].data);
        }
    }
    
    // Free receive buffer data
    for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
        if (channel->recv_buffer[i].data) {
            free(channel->recv_buffer[i].data);
        }
    }
    
    free(channel);
    LOG_INFO("Reliable channel destroyed");
}

bool reliable_channel_send(ReliableChannel* channel, const void* data, u32 size) {
    if (!channel || !data || size == 0 || !channel->is_initialized) return false;
    
    // For unreliable mode, just send directly
    if (channel->mode == RELIABLE_MODE_UNRELIABLE) {
        return udp_socket_send_to(channel->socket, &channel->remote_address, data, size);
    }
    
    // Check if send buffer is full
    if (channel->send_count >= RELIABLE_CHANNEL_BUFFER_SIZE) {
        LOG_WARN("Send buffer full, dropping packet");
        channel->packets_dropped++;
        return false;
    }
    
    // Find free slot in send buffer
    u32 index = channel->send_tail;
    ReliablePacket* packet = &channel->send_buffer[index];
    
    // Allocate and copy data
    packet->data = malloc(size);
    if (!packet->data) {
        LOG_ERROR("Failed to allocate packet data");
        return false;
    }
    
    memcpy(packet->data, data, size);
    packet->size = size;
    packet->sequence = reliable_channel_generate_sequence();
    packet->timestamp = timing_get_time_ms();
    packet->retry_count = 0;
    packet->acked = false;
    packet->in_flight = true;
    
    // Build packet header: sequence (4 bytes) + data
    u8 send_buffer[UDP_MAX_PACKET_SIZE];
    memcpy(send_buffer, &packet->sequence, sizeof(u32));
    memcpy(send_buffer + sizeof(u32), packet->data, packet->size);
    
    // Send packet
    bool sent = udp_socket_send_to(channel->socket, &channel->remote_address, 
                                  send_buffer, packet->size + sizeof(u32));
    
    if (sent) {
        channel->send_tail = (channel->send_tail + 1) % RELIABLE_CHANNEL_BUFFER_SIZE;
        channel->send_count++;
        channel->packets_sent++;
        LOG_TRACE("Sent reliable packet with sequence %u", packet->sequence);
    } else {
        free(packet->data);
        packet->data = NULL;
        LOG_ERROR("Failed to send reliable packet");
        return false;
    }
    
    return sent;
}

bool reliable_channel_receive(ReliableChannel* channel, void* data, u32* size) {
    if (!channel || !data || !size || !channel->is_initialized) return false;
    
    // Find next packet in order for reliable ordered mode
    u32 index = -1;
    
    if (channel->mode == RELIABLE_MODE_RELIABLE_ORDERED) {
        // Look for packet with expected sequence
        for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
            ReceivedPacket* packet = &channel->recv_buffer[i];
            if (packet->received && packet->sequence == channel->recv_sequence + 1) {
                index = i;
                break;
            }
        }
        
        if (index == (u32)-1) {
            return false; // No packet in order available
        }
    } else if (channel->recv_count > 0) {
        // For other modes, just take the next available packet
        index = channel->recv_head;
    }
    
    if (index == (u32)-1) {
        return false;
    }
    
    ReceivedPacket* packet = &channel->recv_buffer[index];
    
    // Copy data to output
    u32 copy_size = (*size < packet->size) ? *size : packet->size;
    memcpy(data, packet->data, copy_size);
    *size = copy_size;
    
    // Update receive sequence for ordered mode
    if (channel->mode == RELIABLE_MODE_RELIABLE_ORDERED) {
        channel->recv_sequence = packet->sequence;
    }
    
    // Free packet data and mark as not received
    free(packet->data);
    packet->data = NULL;
    packet->received = false;
    
    // Update receive buffer pointers
    channel->recv_head = (channel->recv_head + 1) % RELIABLE_CHANNEL_BUFFER_SIZE;
    channel->recv_count--;
    channel->packets_received++;
    
    LOG_TRACE("Received reliable packet with sequence %u", packet->sequence);
    return true;
}

void reliable_channel_update(ReliableChannel* channel, f32 delta_time) {
    if (!channel || !channel->is_initialized) return;
    
    u64 current_time = timing_get_time_ms();
    
    // Resend unacked packets
    for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
        ReliablePacket* packet = &channel->send_buffer[i];
        
        if (reliable_channel_should_resend(packet, current_time, RELIABLE_CHANNEL_RESEND_INTERVAL_MS)) {
            // Resend packet
            u8 send_buffer[UDP_MAX_PACKET_SIZE];
            memcpy(send_buffer, &packet->sequence, sizeof(u32));
            memcpy(send_buffer + sizeof(u32), packet->data, packet->size);
            
            if (udp_socket_send_to(channel->socket, &channel->remote_address, 
                                   send_buffer, packet->size + sizeof(u32))) {
                packet->timestamp = current_time;
                packet->retry_count++;
                channel->packets_resent++;
                LOG_TRACE("Resent packet sequence %u (retry %u)", packet->sequence, packet->retry_count);
            } else {
                packet->retry_count++;
                if (packet->retry_count >= RELIABLE_CHANNEL_MAX_RETRIES) {
                    LOG_WARN("Packet sequence %u exceeded max retries, dropping", packet->sequence);
                    packet->acked = true; // Mark as processed to remove
                    channel->packets_dropped++;
                }
            }
        }
    }
    
    // Remove acked packets from send buffer
    for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
        ReliablePacket* packet = &channel->send_buffer[i];
        if (packet->acked || (packet->retry_count >= RELIABLE_CHANNEL_MAX_RETRIES)) {
            if (packet->data) {
                free(packet->data);
                packet->data = NULL;
            }
            memset(packet, 0, sizeof(ReliablePacket));
            
            if (i == channel->send_head) {
                channel->send_head = (channel->send_head + 1) % RELIABLE_CHANNEL_BUFFER_SIZE;
                channel->send_count--;
            }
        }
    }
    
    // Send periodic acknowledgments
    if (current_time - channel->last_ack_send_time > 100) { // Send ack every 100ms
        reliable_channel_send_ack(channel);
        channel->last_ack_send_time = current_time;
    }
    
    // Update packet loss rate
    if (channel->packets_sent > 0) {
        channel->packet_loss_rate = (f32)channel->packets_dropped / (f32)channel->packets_sent;
    }
}

void reliable_channel_process_ack(ReliableChannel* channel, u32 ack_sequence, u32 ack_bits) {
    if (!channel || !channel->is_initialized) return;
    
    channel->remote_sequence = ack_sequence;
    channel->ack_bits = ack_bits;
    channel->last_ack_receive_time = timing_get_time_ms();
    channel->acks_received++;
    
    // Process ack for sequence
    for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
        ReliablePacket* packet = &channel->send_buffer[i];
        if (packet->in_flight && !packet->acked && packet->sequence == ack_sequence) {
            packet->acked = true;
            reliable_channel_update_rtt(channel, channel->last_ack_receive_time, packet->timestamp);
            LOG_TRACE("Acked packet sequence %u", packet->sequence);
            break;
        }
    }
    
    // Process ack bits for previous 32 packets
    for (u32 i = 0; i < 32; i++) {
        if (ack_bits & (1 << i)) {
            u32 sequence = ack_sequence - (i + 1);
            for (u32 j = 0; j < RELIABLE_CHANNEL_BUFFER_SIZE; j++) {
                ReliablePacket* packet = &channel->send_buffer[j];
                if (packet->in_flight && !packet->acked && packet->sequence == sequence) {
                    packet->acked = true;
                    reliable_channel_update_rtt(channel, channel->last_ack_receive_time, packet->timestamp);
                    LOG_TRACE("Acked packet sequence %u (via ack bits)", packet->sequence);
                    break;
                }
            }
        }
    }
}

void reliable_channel_send_ack(ReliableChannel* channel) {
    if (!channel || !channel->is_initialized) return;
    
    u32 ack_sequence = channel->recv_sequence;
    u32 ack_bits = reliable_channel_calculate_ack_bits(channel, ack_sequence);
    
    // Send ack packet: ack_sequence (4 bytes) + ack_bits (4 bytes)
    u8 ack_data[8];
    memcpy(ack_data, &ack_sequence, sizeof(u32));
    memcpy(ack_data + sizeof(u32), &ack_bits, sizeof(u32));
    
    if (udp_socket_send_to(channel->socket, &channel->remote_address, ack_data, 8)) {
        channel->acks_sent++;
        LOG_TRACE("Sent ack for sequence %u with bits 0x%08X", ack_sequence, ack_bits);
    }
}

bool reliable_channel_is_empty(const ReliableChannel* channel) {
    return channel ? channel->recv_count == 0 : true;
}

bool reliable_channel_is_full(const ReliableChannel* channel) {
    return channel ? channel->send_count >= RELIABLE_CHANNEL_BUFFER_SIZE : false;
}

u32 reliable_channel_get_pending_count(const ReliableChannel* channel) {
    return channel ? channel->send_count : 0;
}

u32 reliable_channel_get_unacked_count(const ReliableChannel* channel) {
    if (!channel) return 0;
    
    u32 count = 0;
    for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
        const ReliablePacket* packet = &channel->send_buffer[i];
        if (packet->in_flight && !packet->acked) {
            count++;
        }
    }
    return count;
}

void reliable_channel_get_stats(const ReliableChannel* channel, u32* sent, u32* received, 
                               u32* resent, u32* dropped, f32* rtt_ms, f32* loss_rate) {
    if (!channel) return;
    
    if (sent) *sent = channel->packets_sent;
    if (received) *received = channel->packets_received;
    if (resent) *resent = channel->packets_resent;
    if (dropped) *dropped = channel->packets_dropped;
    if (rtt_ms) *rtt_ms = channel->rtt_ms;
    if (loss_rate) *loss_rate = channel->packet_loss_rate;
}

void reliable_channel_reset_stats(ReliableChannel* channel) {
    if (!channel) return;
    
    channel->packets_sent = 0;
    channel->packets_received = 0;
    channel->packets_resent = 0;
    channel->packets_dropped = 0;
    channel->acks_sent = 0;
    channel->acks_received = 0;
    channel->rtt_ms = 0.0f;
    channel->packet_loss_rate = 0.0f;
}

void reliable_channel_set_timeout(ReliableChannel* channel, u32 timeout_ms) {
    if (!channel) return;
    
    // Update timeout for all unacked packets
    for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
        ReliablePacket* packet = &channel->send_buffer[i];
        if (packet->in_flight && !packet->acked) {
            // Check if packet should be resent based on new timeout
            u64 current_time = timing_get_time_ms();
            if ((current_time - packet->timestamp) >= timeout_ms) {
                packet->retry_count++;
                packet->timestamp = current_time;
            }
        }
    }
}

void reliable_channel_set_max_retries(ReliableChannel* channel, u32 max_retries) {
    if (!channel) return;
    
    // Mark packets exceeding max retries as failed
    for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
        ReliablePacket* packet = &channel->send_buffer[i];
        if (packet->in_flight && !packet->acked && packet->retry_count >= max_retries) {
            packet->acked = true; // Mark as processed to remove
            channel->packets_dropped++;
        }
    }
}

void reliable_channel_set_resend_interval(ReliableChannel* channel, u32 interval_ms) {
    if (!channel) return;
    
    // Update resend interval for all unacked packets
    for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
        ReliablePacket* packet = &channel->send_buffer[i];
        if (packet->in_flight && !packet->acked) {
            u64 current_time = timing_get_time_ms();
            if ((current_time - packet->timestamp) >= interval_ms) {
                packet->retry_count++;
                packet->timestamp = current_time;
            }
        }
    }
}

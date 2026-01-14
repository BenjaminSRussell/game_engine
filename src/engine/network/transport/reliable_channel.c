#include <include/core/logger.h>
#include <include/core/utils.h>
#include <include/network/transport/reliable_channel.h>
#include <stdlib.h>
#include <string.h>

#define RELIABLE_CHANNEL_HEADER_SIZE 12
#define RELIABLE_CHANNEL_MAX_PACKET_SIZE 1200
#define RELIABLE_CHANNEL_ACK_PACKET_SIZE 8

typedef struct {
  u32 sequence;
  u32 ack;
  u32 ack_bits;
} ReliableHeader;

// Forward declarations
void reliable_channel_send_packet(ReliableChannel *channel,
                                  ReliablePacket *packet);
static u32 reliable_channel_calculate_ack_bits(const ReliableChannel *channel,
                                               u32 sequence);
void reliable_channel_process_data_packet(ReliableChannel *channel,
                                          u32 sequence, const u8 *data,
                                          u32 size);
void reliable_channel_retransmit_lost_packets(ReliableChannel *channel,
                                              u64 current_time);
void reliable_channel_cleanup_old_packets(ReliableChannel *channel,
                                          u64 current_time);
void reliable_channel_cleanup_acked_packets(ReliableChannel *channel);

// Removed local get_time_ms

static u32 sequence_diff(u32 a, u32 b) { return (u32)((i32)b - (i32)a); }

static bool sequence_greater_than(u32 a, u32 b) {
  return sequence_diff(a, b) < 0x80000000;
}

ReliableChannel *reliable_channel_create(ReliableMode mode, UDPSocket *socket,
                                         const NetworkAddress *remote_address) {
  if (!socket || !remote_address) {
    LOG_ERROR("Invalid parameters for reliable channel creation");
    return NULL;
  }

  ReliableChannel *channel =
      (ReliableChannel *)calloc(1, sizeof(ReliableChannel));
  if (!channel) {
    LOG_ERROR("Failed to allocate reliable channel");
    return NULL;
  }

  channel->mode = mode;
  channel->socket = socket;
  channel->remote_address = *remote_address;

  // Initialize send buffer
  channel->send_sequence = 0;
  channel->send_head = 0;
  channel->send_tail = 0;
  channel->send_count = 0;

  // Initialize receive buffer
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    channel->recv_buffer[i].sequence = 0;
    channel->recv_buffer[i].received = false;
  }

  // Initialize receive sequence
  channel->recv_sequence = 0;
  channel->recv_head = 0;
  channel->recv_tail = 0;
  channel->recv_count = 0;

  // Initialize ack tracking
  channel->last_ack = 0;
  channel->ack_bits = 0;
  channel->last_ack_send_time = get_current_time_ms();

  LOG_INFO("Created reliable channel (mode: %d)", mode);
  return channel;
}

void reliable_channel_destroy(ReliableChannel *channel) {
  if (!channel)
    return;

  // Free send buffer
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    if (channel->send_buffer[i].data) {
      free(channel->send_buffer[i].data);
    }
  }

  // Free receive buffer
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    if (channel->recv_buffer[i].data) {
      free(channel->recv_buffer[i].data);
    }
  }

  free(channel);
  LOG_INFO("Destroyed reliable channel");
}

bool reliable_channel_send(ReliableChannel *channel, const void *data,
                           u32 size) {
  if (!channel || !data || size == 0)
    return false;

  if (size > RELIABLE_CHANNEL_MAX_PACKET_SIZE) {
    LOG_ERROR("Packet too large: %u bytes (max: %u)", size,
              RELIABLE_CHANNEL_MAX_PACKET_SIZE);
    return false;
  }

  // Check send buffer capacity
  if (channel->send_count >= RELIABLE_CHANNEL_BUFFER_SIZE) {
    LOG_WARN("Send buffer full, dropping packet");
    return false;
  }

  // Create packet
  ReliablePacket *packet = &channel->send_buffer[channel->send_tail];

  // Free old packet data if exists
  if (packet->data) {
    free(packet->data);
  }

  // Allocate and copy packet data
  packet->data = (u8 *)malloc(size);
  if (!packet->data) {
    LOG_ERROR("Failed to allocate packet data");
    return false;
  }

  memcpy(packet->data, data, size);
  packet->size = size;
  packet->sequence = channel->send_sequence++;
  packet->timestamp = get_current_time_ms();
  packet->retry_count = 0;
  packet->acked = false;
  packet->in_flight = true;

  // Update send buffer pointers
  channel->send_tail = (channel->send_tail + 1) % RELIABLE_CHANNEL_BUFFER_SIZE;
  channel->send_count++;

  // Send packet immediately
  reliable_channel_send_packet(channel, packet);

  LOG_DEBUG("Sent packet %u (%u bytes)", packet->sequence, size);
  return true;
}

bool reliable_channel_receive(ReliableChannel *channel, void *data, u32 *size) {
  if (!channel || !data || !size)
    return false;

  // Check receive buffer for packets
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    ReceivedPacket *packet = &channel->recv_buffer[i];

    if (packet->received && packet->data && packet->size > 0) {
      // Check if this is the next expected sequence
      if (sequence_greater_than(packet->sequence, channel->recv_sequence)) {
        // Skip ahead (packet loss)
        channel->recv_sequence = packet->sequence;
      }

      if (packet->sequence == channel->recv_sequence) {
        // This is the packet we're waiting for
        if (*size >= packet->size) {
          memcpy(data, packet->data, packet->size);
          *size = packet->size;

          // Mark as consumed
          free(packet->data);
          packet->data = NULL;
          packet->received = false;

          // Advance receive sequence
          channel->recv_sequence++;

          LOG_DEBUG("Received packet %u (%u bytes)", packet->sequence, *size);
          return true;
        } else {
          LOG_ERROR("Receive buffer too small: %u bytes needed, %u available",
                    packet->size, *size);
          return false;
        }
      }
    }
  }

  // No packet available
  *size = 0;
  return false;
}

void reliable_channel_update(ReliableChannel *channel, f32 delta_time) {
  if (!channel)
    return;

  u64 current_time = get_current_time_ms();

  // Send ack packets periodically
  channel->last_ack_send_time = current_time;

  // Retransmit lost packets
  reliable_channel_retransmit_lost_packets(channel, current_time);

  // Clean up old packets
  reliable_channel_cleanup_old_packets(channel, current_time);
}

void reliable_channel_send_packet(ReliableChannel *channel,
                                  ReliablePacket *packet) {
  if (!channel || !packet || !packet->data)
    return;

  // Create packet buffer
  u8 packet_buffer[RELIABLE_CHANNEL_HEADER_SIZE + packet->size];

  // Write header
  ReliableHeader header = {.sequence = packet->sequence,
                           .ack = channel->recv_sequence,
                           .ack_bits = reliable_channel_calculate_ack_bits(
                               channel, packet->sequence)};

  memcpy(packet_buffer, &header, sizeof(ReliableHeader));
  memcpy(packet_buffer + sizeof(ReliableHeader), packet->data, packet->size);

  // Send packet
  bool success =
      udp_socket_send_to(channel->socket, &channel->remote_address,
                         packet_buffer, sizeof(ReliableHeader) + packet->size);

  if (success) {
    packet->timestamp = get_current_time_ms();
    packet->in_flight = true;
    LOG_DEBUG("Sent packet %u with ack %u", packet->sequence, header.ack);
  } else {
    LOG_ERROR("Failed to send packet %u", packet->sequence);
  }
}

void reliable_channel_send_ack(ReliableChannel *channel) {
  if (!channel)
    return;

  // Create ack packet
  ReliableHeader header = {.sequence = 0, // Not used for ack packets
                           .ack = channel->recv_sequence,
                           .ack_bits = reliable_channel_calculate_ack_bits(
                               channel, channel->recv_sequence)};

  // Send ack packet
  bool success = udp_socket_send_to(channel->socket, &channel->remote_address,
                                    &header, sizeof(ReliableHeader));

  if (success) {
    LOG_DEBUG("Sent ack %u (bits: 0x%08X)", header.ack, header.ack_bits);
  } else {
    LOG_ERROR("Failed to send ack packet");
  }
}

static u32 reliable_channel_calculate_ack_bits(const ReliableChannel *channel,
                                               u32 sequence) {
  if (!channel)
    return 0;

  u32 ack_bits = 0;

  // Calculate ack bits for last 32 packets
  for (u32 i = 0; i < 32; i++) {
    u32 sequence = channel->recv_sequence - i - 1;

    // Check if we have this packet
    for (u32 j = 0; j < RELIABLE_CHANNEL_BUFFER_SIZE; j++) {
      const ReceivedPacket *packet = &channel->recv_buffer[j];

      if (packet->received && packet->sequence == sequence) {
        ack_bits |= (1 << i);
        break;
      }
    }
  }

  return ack_bits;
}

void reliable_channel_process_packet(ReliableChannel *channel, const u8 *data,
                                     u32 size) {
  if (!channel || !data || size < sizeof(ReliableHeader))
    return;

  // Read header
  ReliableHeader header;
  memcpy(&header, data, sizeof(ReliableHeader));

  // Process ack
  reliable_channel_process_ack(channel, header.ack, header.ack_bits);

  // Check if this is an ack-only packet
  if (size == sizeof(ReliableHeader)) {
    LOG_DEBUG("Received ack-only packet (ack: %u, bits: 0x%08X)", header.ack,
              header.ack_bits);
    return;
  }

  // Process data packet
  const u8 *packet_data = data + sizeof(ReliableHeader);
  u32 packet_size = size - sizeof(ReliableHeader);

  reliable_channel_process_data_packet(channel, header.sequence, packet_data,
                                       packet_size);
}

void reliable_channel_process_ack(ReliableChannel *channel, u32 ack,
                                  u32 ack_bits) {
  if (!channel)
    return;

  // Update last ack
  channel->last_ack = ack;
  channel->last_ack_bits = ack_bits;

  // Mark packets as acked
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    ReliablePacket *packet = &channel->send_buffer[i];

    if (packet->in_flight) {
      // Check if packet is acked
      if (packet->sequence == ack) {
        packet->acked = true;
        packet->in_flight = false;
        LOG_DEBUG("Packet %u acked", packet->sequence);
      } else if (sequence_greater_than(ack, packet->sequence)) {
        u32 diff = sequence_diff(packet->sequence, ack);
        if (diff <= 32 && (ack_bits & (1 << (diff - 1)))) {
          packet->acked = true;
          packet->in_flight = false;
          LOG_DEBUG("Packet %u acked (bits)", packet->sequence);
        }
      }
    }
  }

  // Clean up acked packets
  reliable_channel_cleanup_acked_packets(channel);
}

void reliable_channel_process_data_packet(ReliableChannel *channel,
                                          u32 sequence, const u8 *data,
                                          u32 size) {
  if (!channel || !data || size == 0)
    return;

  // Check if packet is too old
  if (!sequence_greater_than(sequence, channel->recv_sequence)) {
    LOG_DEBUG("Discarding old packet %u (current: %u)", sequence,
              channel->recv_sequence);
    return;
  }

  // Check if packet already received
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    ReceivedPacket *packet = &channel->recv_buffer[i];

    if (packet->received && packet->sequence == sequence) {
      LOG_DEBUG("Duplicate packet %u, discarding", sequence);
      return;
    }
  }

  // Find empty slot in receive buffer
  u32 slot = sequence % RELIABLE_CHANNEL_BUFFER_SIZE;
  ReceivedPacket *packet = &channel->recv_buffer[slot];

  // Free old packet data if exists
  if (packet->data) {
    free(packet->data);
  }

  // Store packet
  packet->data = (u8 *)malloc(size);
  if (!packet->data) {
    LOG_ERROR("Failed to allocate receive packet data");
    return;
  }

  memcpy(packet->data, data, size);
  packet->size = size;
  packet->sequence = sequence;
  packet->received = true;

  LOG_DEBUG("Received packet %u (%u bytes)", sequence, size);
}

void reliable_channel_retransmit_lost_packets(ReliableChannel *channel,
                                              u64 current_time) {
  if (!channel)
    return;

  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    ReliablePacket *packet = &channel->send_buffer[i];

    if (packet->in_flight && !packet->acked) {
      u64 time_since_send = current_time - packet->timestamp;

      // Check if packet should be retransmitted
      if (time_since_send > RELIABLE_CHANNEL_RESEND_INTERVAL_MS) {
        if (packet->retry_count < RELIABLE_CHANNEL_MAX_RETRIES) {
          // Retransmit packet
          reliable_channel_send_packet(channel, packet);
          packet->retry_count++;
          packet->timestamp = current_time;

          LOG_DEBUG("Retransmitting packet %u (attempt %u)", packet->sequence,
                    packet->retry_count);
        } else {
          // Max retries exceeded, drop packet
          LOG_WARN("Packet %u max retries exceeded, dropping",
                   packet->sequence);
          packet->in_flight = false;
        }
      }
    }
  }
}

void reliable_channel_cleanup_old_packets(ReliableChannel *channel,
                                          u64 current_time) {
  if (!channel)
    return;

  // Clean up old send packets
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    ReliablePacket *packet = &channel->send_buffer[i];

    if (packet->acked && packet->data) {
      u64 time_since_ack = current_time - packet->timestamp;

      if (time_since_ack > RELIABLE_CHANNEL_TIMEOUT_MS) {
        free(packet->data);
        packet->data = NULL;
        packet->size = 0;
        packet->acked = false;

        LOG_DEBUG("Cleaned up acked packet %u", packet->sequence);
      }
    }
  }

  // Clean up old receive packets
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    ReceivedPacket *packet = &channel->recv_buffer[i];

    if (packet->received && packet->data) {
      u32 age = sequence_diff(packet->sequence, channel->recv_sequence);

      if (age > RELIABLE_CHANNEL_BUFFER_SIZE) {
        free(packet->data);
        packet->data = NULL;
        packet->size = 0;
        packet->received = false;

        LOG_DEBUG("Cleaned up old receive packet %u", packet->sequence);
      }
    }
  }
}

void reliable_channel_cleanup_acked_packets(ReliableChannel *channel) {
  if (!channel)
    return;

  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    ReliablePacket *packet = &channel->send_buffer[i];

    if (packet->acked && packet->data) {
      free(packet->data);
      packet->data = NULL;
      packet->size = 0;
      packet->acked = false;

      // Update send buffer pointers
      if (i == channel->send_head) {
        channel->send_head =
            (channel->send_head + 1) % RELIABLE_CHANNEL_BUFFER_SIZE;
        channel->send_count--;
      }
    }
  }
}

void reliable_channel_get_statistics(const ReliableChannel *channel,
                                     u32 *out_packets_sent,
                                     u32 *out_packets_received,
                                     u32 *out_packets_lost, f32 *out_rtt_ms) {
  if (!channel)
    return;

  u32 packets_sent = 0;
  u32 packets_received = 0;
  u32 packets_lost = 0;
  f32 rtt_ms = 0.0f;

  // Count sent packets
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    const ReliablePacket *packet = &channel->send_buffer[i];
    if (packet->data) {
      packets_sent++;
      if (packet->acked) {
        packets_received++;
      } else if (packet->retry_count >= RELIABLE_CHANNEL_MAX_RETRIES) {
        packets_lost++;
      }
    }
  }

  // Calculate RTT (simplified)
  if (packets_received > 0) {
    rtt_ms = (f32)RELIABLE_CHANNEL_RESEND_INTERVAL_MS * 2.0f;
  }

  if (out_packets_sent)
    *out_packets_sent = packets_sent;
  if (out_packets_received)
    *out_packets_received = packets_received;
  if (out_packets_lost)
    *out_packets_lost = packets_lost;
  if (out_rtt_ms)
    *out_rtt_ms = rtt_ms;
}

void reliable_channel_reset_statistics(ReliableChannel *channel) {
  if (!channel)
    return;

  // Reset all packet states
  for (u32 i = 0; i < RELIABLE_CHANNEL_BUFFER_SIZE; i++) {
    ReliablePacket *packet = &channel->send_buffer[i];
    if (packet->data) {
      packet->retry_count = 0;
      packet->timestamp = get_current_time_ms();
    }
  }

  channel->last_ack = 0;
  channel->last_ack_bits = 0;
  channel->last_ack_send_time = get_current_time_ms();
}

bool reliable_channel_is_connected(const ReliableChannel *channel) {
  if (!channel)
    return false;

  // Check if we've received any packets recently
  return channel->recv_sequence > 0;
}

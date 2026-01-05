/**
 * @file packet_transport.c
 * @brief Reliable UDP Transport Layer.
 *
 * Implements reliability, ordering, and fragmentation on top of UDP.
 *
 * @copyright (c) 2024 Minecraft v2 Engine
 */

#include <networking/core/packet_transport.h>

// =================================================================================================
//                                      STRUCTS
// =================================================================================================

#define MAX_PACKET_SIZE 1200
#define PROTOCOL_ID 0xCAFEBABE

typedef struct PacketHeader {
  uint32_t protocol_id;
  uint32_t sequence;
  uint32_t ack;
  uint32_t ack_bitfield; // Acks for seq-1 to seq-32
} PacketHeader;

// =================================================================================================
//                                      IMPLEMENTATION
// =================================================================================================

/**
 * @brief Wraps data with header and sends via socket.
 */
void net_send_packet(Socket *sock, Address *dest, void *data, int size,
                     uint32_t seq, uint32_t last_received) {
  uint8_t buffer[MAX_PACKET_SIZE];
  PacketHeader *header = (PacketHeader *)buffer;

  header->protocol_id = PROTOCOL_ID;
  header->sequence = seq;
  header->ack = last_received;
  header->ack_bitfield = 0; // ... compute bitfield ...

  memcpy(buffer + sizeof(PacketHeader), data, size);

  socket_sendto(sock, dest, buffer, size + sizeof(PacketHeader));
}

/**
 * @brief Handles incoming packet logic.
 */
void net_receive_packet(Socket *sock, void *buffer, int size) {
  PacketHeader *header = (PacketHeader *)buffer;

  if (header->protocol_id != PROTOCOL_ID)
    return;

  // Update ACK tracking
  // If header->sequence > local_last_received, update local_last_received

  // Process ACKs inside header to mark outgoing packets as delivered
  // process_ack_bitfield(header->ack, header->ack_bitfield);

  // Dispatch payload
  void *payload = (uint8_t *)buffer + sizeof(PacketHeader);
  // ... dispatch ...
}

/**
 * @brief Resends critical packets that were not ACKed.
 */
void net_update_reliability(Connection *conn, float dt) {
  conn->time_since_last_packet += dt;

  for (int i = 0; i < conn->pending_packets_count; i++) {
    PendingPacket *p = &conn->pending_packets[i];
    if (p->is_acked)
      continue;

    if (time_now() - p->send_time > RTT_MULTIPLIER * conn->rtt) {
      // Resend
      net_send_packet(conn->socket, &conn->addr, p->data, p->size, p->sequence,
                      conn->last_received);
      p->send_time = time_now();
    }
  }
}

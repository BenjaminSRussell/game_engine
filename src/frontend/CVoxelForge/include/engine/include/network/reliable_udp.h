/**
 * =================================================================================================
 *                          RELIABLE UDP HEADER
 *                          Phase 5: Networking
 * =================================================================================================
 */

#ifndef RELIABLE_UDP_H
#define RELIABLE_UDP_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_PACKET_SIZE 1200
#define PROTOCOL_ID 0x12345678

typedef struct {
  uint32_t protocol_id;
  uint32_t sequence;
  uint32_t ack;
  uint32_t ack_bits;
  uint16_t data_len;
} PacketHeader;

typedef struct {
  int socket_handle;
  uint32_t remote_seq;
  uint32_t local_seq;
  uint32_t ack_bits;
} ReliableConnection;

bool network_init(void);
void network_shutdown(void);

ReliableConnection *connection_create(void);
void connection_destroy(ReliableConnection *conn);

bool connection_send_packet(ReliableConnection *conn, const void *data,
                            uint32_t size);
bool connection_receive_packet(ReliableConnection *conn, void *buffer,
                               uint32_t size, uint32_t *bytes_read);

#endif // RELIABLE_UDP_H

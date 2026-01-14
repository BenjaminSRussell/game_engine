/*
 * connection_manager.c
 *
 * Network connection management implementation
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 */

#include "connection_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define MAX_PACKET_QUEUE_SIZE 1024
#define DEFAULT_TIMEOUT_MS 30000
#define PACKET_HEADER_SIZE 16
#define MAX_PACKET_SIZE (64 * 1024)
#define RELIABLE_BUFFER_SIZE 256
#define RTT_SMOOTHING_FACTOR 0.1f
#define RESEND_TIMEOUT_MULTIPLIER 1.5f
#define INITIAL_RTT_MS 100.0f
#define FRAGMENT_SIZE 1024
#define MAX_REASSEMBLY_BUFFERS 16

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct {
    uint32_t group_id;
    uint16_t total_fragments;
    uint16_t fragments_received;
    uint16_t original_type;
    size_t total_data_size;
    uint8_t* buffer;
    bool active;
    struct timespec created_time;
} ReassemblyBuffer;

typedef struct {
    TransportPacket packet;
    uint32_t sequence;
    struct timespec first_sent_time;
    struct timespec last_sent_time;
    uint32_t resend_count;
    bool active;
    bool acked;
} ReliablePacketSlot;

typedef struct {
    TransportPacket packet;
    uint32_t retry_count;
    struct timespec send_time;
} QueuedPacket;

typedef struct {
    ConnectionID id;
    int socket;
    struct sockaddr_in peer_address;
    ConnectionState state;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    float latency_ms;
    struct timespec connect_time;
    struct timespec last_activity_time;
    uint32_t timeout_ms;

    /* Packet queues */
    QueuedPacket sent_queue[MAX_PACKET_QUEUE_SIZE];
    uint32_t sent_queue_head;
    uint32_t sent_queue_tail;

    TransportPacket recv_queue[MAX_PACKET_QUEUE_SIZE];
    uint32_t recv_queue_head;
    uint32_t recv_queue_tail;

    /* Reliability */
    uint32_t next_sequence_number;
    uint32_t remote_sequence_number;
    uint32_t ack_bits;
    ReliablePacketSlot reliable_buffer[RELIABLE_BUFFER_SIZE];

    /* Fragmentation */
    uint32_t next_group_id;
    ReassemblyBuffer reassembly_buffers[MAX_REASSEMBLY_BUFFERS];

    /* Quality */
    float packet_loss;
    float jitter_ms;
    float last_rtt_ms;

    /* Bandwidth */
    uint32_t bandwidth_limit;
    int32_t bandwidth_tokens;
    struct timespec last_bandwidth_update;

    pthread_mutex_t lock;
} Connection;

typedef struct ConnectionManager {
    Connection* connections;
    uint16_t max_connections;
    uint32_t next_connection_id;

    int server_socket;
    struct sockaddr_in server_address;
    bool is_server;

    /* Callbacks */
    ConnectionStateChangeCallback state_change_callback;
    PacketReceivedCallback packet_received_callback;
    DisconnectionCallback disconnection_callback;
    void* callback_user_data;

    /* Statistics */
    ConnectionStats stats;

    pthread_t network_thread;
    volatile bool shutdown_requested;
    pthread_mutex_t lock;
} ConnectionManager;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

static double time_diff_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1e6;
}

static int count_set_bits(uint32_t n) {
    int count = 0;
    while (n > 0) {
        n &= (n - 1);
        count++;
    }
    return count;
}

static void set_connection_state(
    ConnectionManager* manager,
    Connection* conn,
    ConnectionState new_state) {

    if (conn->state == new_state) {
        return;
    }

    ConnectionState old_state = conn->state;
    conn->state = new_state;

    if (manager->state_change_callback) {
        manager->state_change_callback(
            conn->id,
            old_state,
            new_state,
            manager->callback_user_data
        );
    }
}

static Connection* find_connection(
    ConnectionManager* manager,
    ConnectionID connection_id) {

    if (!manager || connection_id == 0) {
        return NULL;
    }

    for (uint16_t i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i].id == connection_id) {
            return &manager->connections[i];
        }
    }

    return NULL;
}

static Connection* find_free_connection(ConnectionManager* manager) {
    if (!manager) {
        return NULL;
    }

    for (uint16_t i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i].id == 0 &&
            manager->connections[i].state == CONNECTION_DISCONNECTED) {
            return &manager->connections[i];
        }
    }

    return NULL;
}

/* ============================================================================
 * PACKET QUEUE OPERATIONS
 * ============================================================================ */

static bool enqueue_received_packet(
    Connection* conn,
    const TransportPacket* packet) {

    if (!conn || !packet) {
        return false;
    }

    uint32_t next_tail = (conn->recv_queue_tail + 1) % MAX_PACKET_QUEUE_SIZE;
    if (next_tail == conn->recv_queue_head) {
        return false;  /* Queue full */
    }

    /* Copy packet data */
    conn->recv_queue[conn->recv_queue_tail].data = malloc(packet->size);
    if (!conn->recv_queue[conn->recv_queue_tail].data) {
        return false;
    }

    memcpy(conn->recv_queue[conn->recv_queue_tail].data, packet->data, packet->size);
    conn->recv_queue[conn->recv_queue_tail].size = packet->size;
    conn->recv_queue[conn->recv_queue_tail].type = packet->type;
    conn->recv_queue[conn->recv_queue_tail].delivery_type = packet->delivery_type;
    conn->recv_queue[conn->recv_queue_tail].sequence_number = packet->sequence_number;
    conn->recv_queue[conn->recv_queue_tail].timestamp = packet->timestamp;

    conn->recv_queue_tail = next_tail;
    conn->packets_received++;

    return true;
}

static bool dequeue_received_packet(
    Connection* conn,
    TransportPacket* packet) {

    if (!conn || !packet) {
        return false;
    }

    if (conn->recv_queue_head == conn->recv_queue_tail) {
        return false;  /* Queue empty */
    }

    memcpy(packet, &conn->recv_queue[conn->recv_queue_head], sizeof(TransportPacket));

    conn->recv_queue_head = (conn->recv_queue_head + 1) % MAX_PACKET_QUEUE_SIZE;
    return true;
}

/* ============================================================================
 * NETWORK OPERATIONS
 * ============================================================================ */

static int send_packet_on_socket(
    Connection* conn,
    const TransportPacket* packet) {

    if (!conn || !packet || !packet->data) {
        return -1;
    }

    /* Store reliable packets for retransmission */
    if (packet->delivery_type == PACKET_DELIVERY_RELIABLE) {
        uint32_t index = packet->sequence_number % RELIABLE_BUFFER_SIZE;
        ReliablePacketSlot* slot = &conn->reliable_buffer[index];

        /* Clean up old slot if it was active */
        if (slot->active && slot->packet.data) {
            free(slot->packet.data);
            slot->packet.data = NULL;
        }

        slot->active = true;
        slot->acked = false;
        slot->sequence = packet->sequence_number;
        slot->resend_count = 0;
        clock_gettime(CLOCK_MONOTONIC, &slot->first_sent_time);
        slot->last_sent_time = slot->first_sent_time;

        /* Deep copy packet */
        slot->packet = *packet;
        slot->packet.data = malloc(packet->size);
        if (slot->packet.data) {
            memcpy(slot->packet.data, packet->data, packet->size);
        } else {
            slot->active = false; /* Allocation failed */
        }
    }

    /* Build packet with header: [type(2)] [delivery(1)] [sequence(4)] [size(4)] [data] */
    uint8_t header[PACKET_HEADER_SIZE];
    uint16_t type = packet->type;
    uint8_t delivery = packet->delivery_type;
    uint32_t seq = packet->sequence_number;
    uint32_t size = packet->size;

    memcpy(&header[0], &type, 2);
    memcpy(&header[2], &delivery, 1);
    memcpy(&header[3], &seq, 4);
    memcpy(&header[7], &size, 4);

    /* Send header + data */
    if (sendto(conn->socket, header, PACKET_HEADER_SIZE, 0,
               (struct sockaddr*)&conn->peer_address,
               sizeof(conn->peer_address)) < 0) {
        return -2;
    }

    if (sendto(conn->socket, packet->data, packet->size, 0,
               (struct sockaddr*)&conn->peer_address,
               sizeof(conn->peer_address)) < 0) {
        return -3;
    }

    conn->bytes_sent += PACKET_HEADER_SIZE + packet->size;
    conn->packets_sent++;
    clock_gettime(CLOCK_MONOTONIC, &conn->last_activity_time);

    return 0;
}

static void send_ack(Connection* conn) {
    if (!conn) return;

    uint8_t header[PACKET_HEADER_SIZE];
    uint16_t type = PACKET_TYPE_ACK;
    uint8_t delivery = PACKET_DELIVERY_UNRELIABLE;
    uint32_t seq = conn->next_sequence_number;
    uint32_t size = 8; /* ACK payload size */

    memcpy(&header[0], &type, 2);
    memcpy(&header[2], &delivery, 1);
    memcpy(&header[3], &seq, 4);
    memcpy(&header[7], &size, 4);

    uint8_t payload[8];
    uint32_t ack_seq = conn->remote_sequence_number;
    uint32_t ack_bits = conn->ack_bits;
    memcpy(&payload[0], &ack_seq, 4);
    memcpy(&payload[4], &ack_bits, 4);

    if (sendto(conn->socket, header, PACKET_HEADER_SIZE, 0,
              (struct sockaddr*)&conn->peer_address, sizeof(conn->peer_address)) < 0) {
        return;
    }

    if (sendto(conn->socket, payload, 8, 0,
              (struct sockaddr*)&conn->peer_address, sizeof(conn->peer_address)) < 0) {
        return;
    }

    conn->bytes_sent += PACKET_HEADER_SIZE + 8;
    conn->packets_sent++;
    clock_gettime(CLOCK_MONOTONIC, &conn->last_activity_time);
}

static void process_ack(Connection* conn, uint32_t ack_seq, uint32_t ack_bits) {
    if (!conn) return;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    for (int i = 0; i < RELIABLE_BUFFER_SIZE; i++) {
        ReliablePacketSlot* slot = &conn->reliable_buffer[i];
        if (slot->active && !slot->acked) {
            bool acked = false;

            if (slot->sequence == ack_seq) {
                acked = true;
            } else if (slot->sequence < ack_seq) {
                uint32_t diff = ack_seq - slot->sequence;
                if (diff <= 32) {
                    if (ack_bits & (1 << (diff - 1))) {
                        acked = true;
                    }
                }
            }

            if (acked) {
                slot->acked = true;
                slot->active = false;

                /* Update RTT & Jitter */
                double rtt = time_diff_ms(slot->first_sent_time, now);
                if (conn->latency_ms == 0.0f) {
                    conn->latency_ms = (float)rtt;
                    conn->jitter_ms = 0.0f;
                    conn->last_rtt_ms = (float)rtt;
                } else {
                    float delta = (float)rtt - conn->last_rtt_ms;
                    if (delta < 0) delta = -delta;

                    conn->jitter_ms = (conn->jitter_ms * 0.9f) + (delta * 0.1f);
                    conn->latency_ms = (conn->latency_ms * (1.0f - RTT_SMOOTHING_FACTOR)) +
                                     ((float)rtt * RTT_SMOOTHING_FACTOR);
                    conn->last_rtt_ms = (float)rtt;
                }

                if (slot->packet.data) {
                    free(slot->packet.data);
                    slot->packet.data = NULL;
                }
            }
        }
    }
}

static void process_fragment(Connection* conn, const TransportPacket* packet) {
    if (packet->size < 10) return;

    uint32_t group_id = *(uint32_t*)&packet->data[0];
    uint16_t frag_index = *(uint16_t*)&packet->data[4];
    uint16_t total_frags = *(uint16_t*)&packet->data[6];
    uint16_t original_type = *(uint16_t*)&packet->data[8];
    uint32_t data_size = packet->size - 10;

    /* Find or allocate reassembly buffer */
    ReassemblyBuffer* buffer = NULL;
    for (int i = 0; i < MAX_REASSEMBLY_BUFFERS; i++) {
        if (conn->reassembly_buffers[i].active && conn->reassembly_buffers[i].group_id == group_id) {
            buffer = &conn->reassembly_buffers[i];
            break;
        }
    }

    if (!buffer) {
        /* New buffer */
        for (int i = 0; i < MAX_REASSEMBLY_BUFFERS; i++) {
            if (!conn->reassembly_buffers[i].active) {
                buffer = &conn->reassembly_buffers[i];
                buffer->active = true;
                buffer->group_id = group_id;
                buffer->total_fragments = total_frags;
                buffer->fragments_received = 0;
                buffer->original_type = original_type;
                buffer->total_data_size = 0;
                buffer->buffer = calloc(total_frags, FRAGMENT_SIZE);
                clock_gettime(CLOCK_MONOTONIC, &buffer->created_time);
                break;
            }
        }
    }

    if (buffer && buffer->buffer) {
        uint32_t offset = frag_index * FRAGMENT_SIZE;
        memcpy(buffer->buffer + offset, packet->data + 10, data_size);
        buffer->fragments_received++;
        buffer->total_data_size += data_size;

        if (buffer->fragments_received == buffer->total_fragments) {
            /* Reassembly complete */
            TransportPacket assembled = {0};
            assembled.type = buffer->original_type;
            assembled.delivery_type = packet->delivery_type;
            assembled.sequence_number = 0;
            assembled.timestamp = packet->timestamp;
            assembled.size = buffer->total_data_size;
            assembled.data = malloc(assembled.size);

            /* Compact buffer logic since we used calloc with potential gaps or padding at end?
               Actually we just copied strictly to offsets.
               The only issue is if the last fragment isn't the last one in the buffer array (which it should be by index).
               Wait, total_data_size calculation assumes no gaps.
               We just copy blindly from buffer->buffer.
               Since we populated offset = index * SIZE, it should be contiguous.
            */
            memcpy(assembled.data, buffer->buffer, assembled.size);

            enqueue_received_packet(conn, &assembled);

            /* Cleanup */
            free(buffer->buffer);
            buffer->active = false;
            free(assembled.data);
        }
    }
}

static int receive_packets_on_socket(Connection* conn) {
    if (!conn || conn->socket < 0) {
        return -1;
    }

    uint8_t buffer[MAX_PACKET_SIZE + PACKET_HEADER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    while (true) {
        ssize_t bytes = recvfrom(
            conn->socket,
            buffer,
            sizeof(buffer),
            MSG_DONTWAIT,
            (struct sockaddr*)&from_addr,
            &from_len
        );

        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  /* No more packets */
            }
            return -2;
        }

        if (bytes < PACKET_HEADER_SIZE) {
            continue;
        }

        /* Parse packet header */
        TransportPacket pkt = {0};
        pkt.type = *(uint16_t*)&buffer[0];
        pkt.delivery_type = buffer[2];
        pkt.sequence_number = *(uint32_t*)&buffer[3];
        uint32_t packet_size = *(uint32_t*)&buffer[7];

        if (packet_size > MAX_PACKET_SIZE) {
            continue;  /* Invalid packet */
        }

        /* Extract packet data (received as separate message) */
        pkt.data = malloc(packet_size);
        if (!pkt.data) {
            continue;
        }

        memcpy(pkt.data, buffer + PACKET_HEADER_SIZE, bytes - PACKET_HEADER_SIZE);
        pkt.size = bytes - PACKET_HEADER_SIZE;
        clock_gettime(CLOCK_MONOTONIC, &pkt.timestamp);

        /* Update reliability state and check for duplicates */
        if (pkt.sequence_number > conn->remote_sequence_number) {
            uint32_t shift = pkt.sequence_number - conn->remote_sequence_number;
            if (shift < 32) {
                conn->ack_bits <<= shift;
                conn->ack_bits |= (1 << (shift - 1));
            } else {
                conn->ack_bits = 0;
            }
            conn->remote_sequence_number = pkt.sequence_number;
        } else if (pkt.sequence_number < conn->remote_sequence_number) {
            /* For Sequenced packets, discard if older than latest */
            if (pkt.delivery_type == PACKET_DELIVERY_SEQUENCED) {
                free(pkt.data);
                continue;
            }

            uint32_t diff = conn->remote_sequence_number - pkt.sequence_number;
            if (diff <= 32) {
                if (conn->ack_bits & (1 << (diff - 1))) {
                    /* Duplicate packet */
                    free(pkt.data);
                    continue;
                }
                conn->ack_bits |= (1 << (diff - 1));
            } else {
                /* Too old, discard */
                free(pkt.data);
                continue;
            }
        } else {
            /* Duplicate of current sequence */
             free(pkt.data);
             continue;
        }

        if (conn->remote_sequence_number > 32) {
             conn->packet_loss = 1.0f - ((float)count_set_bits(conn->ack_bits) / 32.0f);
        }

        /* Handle ACK packets internally */
        if (pkt.type == PACKET_TYPE_ACK) {
            if (pkt.size >= 8) {
                uint32_t ack_seq = *(uint32_t*)&pkt.data[0];
                uint32_t ack_bits = *(uint32_t*)&pkt.data[4];
                process_ack(conn, ack_seq, ack_bits);
            }
            free(pkt.data);
            conn->bytes_received += bytes;
            clock_gettime(CLOCK_MONOTONIC, &conn->last_activity_time);
            continue;
        }

        /* Handle Fragment packets internally */
        if (pkt.type == PACKET_TYPE_FRAGMENT) {
             process_fragment(conn, &pkt);
             free(pkt.data);
             conn->bytes_received += bytes;
             clock_gettime(CLOCK_MONOTONIC, &conn->last_activity_time);

             /* Send immediate ACK for reliable packets */
             if (pkt.delivery_type == PACKET_DELIVERY_RELIABLE) {
                send_ack(conn);
             }
             continue;
        }

        /* Enqueue packet for application */
        if (!enqueue_received_packet(conn, &pkt)) {
            free(pkt.data);
        } else {
            /* Send immediate ACK for reliable packets */
            if (pkt.delivery_type == PACKET_DELIVERY_RELIABLE) {
                send_ack(conn);
            }
        }

        conn->bytes_received += bytes;
        clock_gettime(CLOCK_MONOTONIC, &conn->last_activity_time);
    }

    return 0;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

ConnectionManager* connection_manager_create(uint16_t max_connections) {
    if (max_connections == 0) {
        max_connections = 32;
    }

    ConnectionManager* manager = (ConnectionManager*)malloc(sizeof(ConnectionManager));
    if (!manager) {
        return NULL;
    }

    memset(manager, 0, sizeof(ConnectionManager));

    manager->connections = (Connection*)calloc(max_connections, sizeof(Connection));
    if (!manager->connections) {
        free(manager);
        return NULL;
    }

    manager->max_connections = max_connections;
    manager->server_socket = -1;
    manager->next_connection_id = 1;
    pthread_mutex_init(&manager->lock, NULL);

    /* Initialize connection locks */
    for (uint16_t i = 0; i < max_connections; i++) {
        pthread_mutex_init(&manager->connections[i].lock, NULL);
        manager->connections[i].socket = -1;
    }

    return manager;
}

void connection_manager_destroy(ConnectionManager* manager) {
    if (!manager) {
        return;
    }

    /* Close all connections */
    for (uint16_t i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i].socket >= 0) {
            close(manager->connections[i].socket);
        }
        pthread_mutex_destroy(&manager->connections[i].lock);
    }

    if (manager->server_socket >= 0) {
        close(manager->server_socket);
    }

    pthread_mutex_destroy(&manager->lock);
    free(manager->connections);
    free(manager);
}

ConnectionID connection_manager_connect(
    ConnectionManager* manager,
    const char* address,
    uint16_t port,
    uint32_t timeout_ms) {

    if (!manager || !address) {
        return 0;
    }

    Connection* conn = find_free_connection(manager);
    if (!conn) {
        return 0;  /* No free slots */
    }

    /* Create UDP socket */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return 0;
    }

    /* Set socket to non-blocking */
    int flags = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    /* Setup peer address */
    memset(&conn->peer_address, 0, sizeof(conn->peer_address));
    conn->peer_address.sin_family = AF_INET;
    conn->peer_address.sin_port = htons(port);

    if (inet_pton(AF_INET, address, &conn->peer_address.sin_addr) <= 0) {
        close(sock);
        return 0;
    }

    conn->id = manager->next_connection_id++;
    conn->socket = sock;
    conn->next_sequence_number = 1; /* Start at 1 to avoid confusion with 0 initialization */
    conn->timeout_ms = timeout_ms ? timeout_ms : DEFAULT_TIMEOUT_MS;
    clock_gettime(CLOCK_MONOTONIC, &conn->connect_time);
    clock_gettime(CLOCK_MONOTONIC, &conn->last_activity_time);

    conn->bandwidth_limit = 0;
    conn->bandwidth_tokens = 0;
    conn->last_bandwidth_update = conn->connect_time;

    set_connection_state(manager, conn, CONNECTION_CONNECTING);

    return conn->id;
}

int connection_manager_disconnect(
    ConnectionManager* manager,
    ConnectionID connection_id) {

    if (!manager) {
        return -1;
    }

    Connection* conn = find_connection(manager, connection_id);
    if (!conn) {
        return -2;
    }

    pthread_mutex_lock(&conn->lock);

    if (conn->socket >= 0) {
        close(conn->socket);
        conn->socket = -1;
    }

    set_connection_state(manager, conn, CONNECTION_DISCONNECTED);
    conn->id = 0;

    pthread_mutex_unlock(&conn->lock);

    if (manager->disconnection_callback) {
        manager->disconnection_callback(
            connection_id,
            "Manual disconnect",
            manager->callback_user_data
        );
    }

    return 0;
}

ConnectionState connection_manager_get_state(
    ConnectionManager* manager,
    ConnectionID connection_id) {

    Connection* conn = find_connection(manager, connection_id);
    if (!conn) {
        return CONNECTION_DISCONNECTED;
    }

    return conn->state;
}

int connection_manager_get_info(
    ConnectionManager* manager,
    ConnectionID connection_id,
    ConnectionInfo* info) {

    if (!manager || !info) {
        return -1;
    }

    Connection* conn = find_connection(manager, connection_id);
    if (!conn) {
        return -2;
    }

    pthread_mutex_lock(&conn->lock);

    info->id = conn->id;
    info->remote_port = ntohs(conn->peer_address.sin_port);
    info->state = conn->state;
    info->packets_sent = conn->packets_sent;
    info->packets_received = conn->packets_received;
    info->bytes_sent = conn->bytes_sent;
    info->bytes_received = conn->bytes_received;
    info->latency_ms = conn->latency_ms;
    info->packet_loss = conn->packet_loss;
    info->jitter_ms = conn->jitter_ms;
    info->connect_time = conn->connect_time;

    pthread_mutex_unlock(&conn->lock);

    return 0;
}

static int send_fragmented_packet(Connection* conn, const TransportPacket* packet) {
    uint32_t group_id = conn->next_group_id++;
    uint32_t total_size = packet->size;
    uint16_t total_fragments = (total_size + FRAGMENT_SIZE - 1) / FRAGMENT_SIZE;

    int result = 0;

    for (uint16_t i = 0; i < total_fragments; i++) {
        uint32_t offset = i * FRAGMENT_SIZE;
        uint32_t size = (total_size - offset > FRAGMENT_SIZE) ? FRAGMENT_SIZE : (total_size - offset);

        uint8_t* frag_data = malloc(10 + size);
        if (!frag_data) return -3;

        *(uint32_t*)&frag_data[0] = group_id;
        *(uint16_t*)&frag_data[4] = i;
        *(uint16_t*)&frag_data[6] = total_fragments;
        *(uint16_t*)&frag_data[8] = (uint16_t)packet->type;
        memcpy(&frag_data[10], packet->data + offset, size);

        TransportPacket frag_pkt;
        frag_pkt.type = PACKET_TYPE_FRAGMENT;
        frag_pkt.delivery_type = packet->delivery_type;
        frag_pkt.sequence_number = conn->next_sequence_number++;
        frag_pkt.timestamp = packet->timestamp;
        frag_pkt.data = frag_data;
        frag_pkt.size = 10 + size;

        int res = send_packet_on_socket(conn, &frag_pkt);
        if (res != 0) result = res;

        free(frag_data);
    }

    return result;
}

int connection_manager_send_packet(
    ConnectionManager* manager,
    ConnectionID connection_id,
    const TransportPacket* packet) {

    if (!manager || !packet) {
        return -1;
    }

    Connection* conn = find_connection(manager, connection_id);
    if (!conn || conn->state != CONNECTION_CONNECTED) {
        return -2;
    }

    pthread_mutex_lock(&conn->lock);

    /* Bandwidth Throttling */
    if (conn->bandwidth_limit > 0) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double dt = time_diff_ms(conn->last_bandwidth_update, now) / 1000.0;
        conn->last_bandwidth_update = now;

        conn->bandwidth_tokens += (int32_t)(conn->bandwidth_limit * dt);
        if (conn->bandwidth_tokens > (int32_t)conn->bandwidth_limit) {
            conn->bandwidth_tokens = conn->bandwidth_limit;
        }

        if (conn->bandwidth_tokens < (int32_t)packet->size) {
            pthread_mutex_unlock(&conn->lock);
            return -4; /* Bandwidth exceeded */
        }
        conn->bandwidth_tokens -= packet->size;
    }

    int result;
    if (packet->size > FRAGMENT_SIZE) {
         result = send_fragmented_packet(conn, packet);
    } else {
         TransportPacket pkt = *packet;
         pkt.sequence_number = conn->next_sequence_number++;
         result = send_packet_on_socket(conn, &pkt);
    }

    pthread_mutex_unlock(&conn->lock);

    if (result == 0) {
        manager->stats.total_bytes_sent += packet->size;
        manager->stats.total_packets_sent++;
    }

    return result;
}

int connection_manager_receive_packets(
    ConnectionManager* manager,
    ConnectionID connection_id,
    TransportPacket** packets,
    uint32_t* packet_count) {

    if (!manager || !packets || !packet_count) {
        return -1;
    }

    Connection* conn = find_connection(manager, connection_id);
    if (!conn) {
        return -2;
    }

    pthread_mutex_lock(&conn->lock);

    /* Count packets in queue */
    uint32_t count = 0;
    uint32_t idx = conn->recv_queue_head;
    while (idx != conn->recv_queue_tail) {
        count++;
        idx = (idx + 1) % MAX_PACKET_QUEUE_SIZE;
    }

    if (count == 0) {
        *packets = NULL;
        *packet_count = 0;
        pthread_mutex_unlock(&conn->lock);
        return 0;
    }

    /* Allocate packet array */
    *packets = (TransportPacket*)malloc(count * sizeof(TransportPacket));
    if (!*packets) {
        pthread_mutex_unlock(&conn->lock);
        return -3;
    }

    /* Dequeue all packets */
    for (uint32_t i = 0; i < count; i++) {
        dequeue_received_packet(conn, &(*packets)[i]);
    }

    *packet_count = count;
    pthread_mutex_unlock(&conn->lock);

    return 0;
}

void connection_manager_free_packet(TransportPacket* packet) {
    if (!packet) {
        return;
    }

    if (packet->data) {
        free(packet->data);
        packet->data = NULL;
    }
}

void connection_manager_free_packets(TransportPacket* packets, uint32_t count) {
    if (!packets) {
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        connection_manager_free_packet(&packets[i]);
    }

    free(packets);
}

void connection_manager_set_state_change_callback(
    ConnectionManager* manager,
    ConnectionStateChangeCallback callback,
    void* user_data) {

    if (!manager) {
        return;
    }

    manager->state_change_callback = callback;
    manager->callback_user_data = user_data;
}

void connection_manager_set_packet_received_callback(
    ConnectionManager* manager,
    PacketReceivedCallback callback,
    void* user_data) {

    if (!manager) {
        return;
    }

    manager->packet_received_callback = callback;
    manager->callback_user_data = user_data;
}

void connection_manager_set_disconnection_callback(
    ConnectionManager* manager,
    DisconnectionCallback callback,
    void* user_data) {

    if (!manager) {
        return;
    }

    manager->disconnection_callback = callback;
    manager->callback_user_data = user_data;
}

static void check_timeouts_and_resend(Connection* conn) {
    if (!conn) return;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double resend_delay = (conn->latency_ms > 0) ? (conn->latency_ms * RESEND_TIMEOUT_MULTIPLIER) : INITIAL_RTT_MS;

    for (int i = 0; i < RELIABLE_BUFFER_SIZE; i++) {
        ReliablePacketSlot* slot = &conn->reliable_buffer[i];
        if (slot->active && !slot->acked) {
            double elapsed = time_diff_ms(slot->last_sent_time, now);
            if (elapsed > resend_delay) {
                /* Resend packet */
                uint8_t header[PACKET_HEADER_SIZE];
                uint16_t type = slot->packet.type;
                uint8_t delivery = slot->packet.delivery_type;
                uint32_t seq = slot->packet.sequence_number;
                uint32_t size = slot->packet.size;

                memcpy(&header[0], &type, 2);
                memcpy(&header[2], &delivery, 1);
                memcpy(&header[3], &seq, 4);
                memcpy(&header[7], &size, 4);

                if (sendto(conn->socket, header, PACKET_HEADER_SIZE, 0,
                          (struct sockaddr*)&conn->peer_address, sizeof(conn->peer_address)) >= 0) {
                    if (sendto(conn->socket, slot->packet.data, slot->packet.size, 0,
                              (struct sockaddr*)&conn->peer_address, sizeof(conn->peer_address)) >= 0) {

                        conn->bytes_sent += PACKET_HEADER_SIZE + slot->packet.size;
                        conn->packets_sent++;
                    }
                }

                slot->last_sent_time = now;
                slot->resend_count++;
            }
        }
    }
}

int connection_manager_update(
    ConnectionManager* manager,
    float delta_time) {

    if (!manager) {
        return -1;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    for (uint16_t i = 0; i < manager->max_connections; i++) {
        Connection* conn = &manager->connections[i];

        if (conn->id == 0) {
            continue;
        }

        pthread_mutex_lock(&conn->lock);

        /* Check timeout */
        double inactive_time = time_diff_ms(conn->last_activity_time, now);
        if (conn->state == CONNECTION_CONNECTING ||
            conn->state == CONNECTION_CONNECTED) {
            if (inactive_time > conn->timeout_ms) {
                set_connection_state(manager, conn, CONNECTION_DISCONNECTED);
                if (manager->disconnection_callback) {
                    manager->disconnection_callback(
                        conn->id,
                        "Timeout",
                        manager->callback_user_data
                    );
                }
            }
        }

        /* Receive packets */
        if (conn->state == CONNECTION_CONNECTED ||
            conn->state == CONNECTION_CONNECTING) {
            receive_packets_on_socket(conn);

            /* Process retransmissions */
            check_timeouts_and_resend(conn);

            /* If connecting and got packets, mark as connected */
            if (conn->state == CONNECTION_CONNECTING &&
                conn->recv_queue_head != conn->recv_queue_tail) {
                set_connection_state(manager, conn, CONNECTION_CONNECTED);
            }
        }

        pthread_mutex_unlock(&conn->lock);
    }

    return 0;
}

void connection_manager_set_bandwidth_limit(
    ConnectionManager* manager,
    ConnectionID connection_id,
    uint32_t bytes_per_second) {

    if (!manager) return;
    Connection* conn = find_connection(manager, connection_id);
    if (!conn) return;

    pthread_mutex_lock(&conn->lock);
    conn->bandwidth_limit = bytes_per_second;
    pthread_mutex_unlock(&conn->lock);
}

int connection_manager_get_statistics(
    ConnectionManager* manager,
    ConnectionStats* stats) {

    if (!manager || !stats) {
        return -1;
    }

    memcpy(stats, &manager->stats, sizeof(ConnectionStats));

    /* Count active connections */
    uint32_t active = 0;
    for (uint16_t i = 0; i < manager->max_connections; i++) {
        if (manager->connections[i].state == CONNECTION_CONNECTED) {
            active++;
        }
    }

    stats->active_connections = active;
    stats->total_connections = manager->max_connections;

    return 0;
}

int connection_manager_create_server(
    ConnectionManager* manager,
    const char* bind_address,
    uint16_t port) {

    if (!manager || !bind_address) {
        return -1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -2;
    }

    memset(&manager->server_address, 0, sizeof(manager->server_address));
    manager->server_address.sin_family = AF_INET;
    manager->server_address.sin_port = htons(port);

    if (inet_pton(AF_INET, bind_address, &manager->server_address.sin_addr) <= 0) {
        close(sock);
        return -3;
    }

    if (bind(sock, (struct sockaddr*)&manager->server_address,
             sizeof(manager->server_address)) < 0) {
        close(sock);
        return -4;
    }

    manager->server_socket = sock;
    manager->is_server = true;

    return 0;
}

int connection_manager_shutdown_server(ConnectionManager* manager) {
    if (!manager) {
        return -1;
    }

    if (manager->server_socket >= 0) {
        close(manager->server_socket);
        manager->server_socket = -1;
    }

    manager->is_server = false;
    return 0;
}

bool connection_manager_is_server(ConnectionManager* manager) {
    if (!manager) {
        return false;
    }

    return manager->is_server;
}

/* End of connection_manager.c */

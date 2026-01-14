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

/* ============================================================================
 * INTERNAL STRUCTURES
 * ============================================================================ */

typedef struct {
    Packet packet;
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

    Packet recv_queue[MAX_PACKET_QUEUE_SIZE];
    uint32_t recv_queue_head;
    uint32_t recv_queue_tail;

    uint32_t next_sequence_number;
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
            manager->connections[i].state == CONNECTION_STATE_DISCONNECTED) {
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
    const Packet* packet) {

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
    Packet* packet) {

    if (!conn || !packet) {
        return false;
    }

    if (conn->recv_queue_head == conn->recv_queue_tail) {
        return false;  /* Queue empty */
    }

    memcpy(packet, &conn->recv_queue[conn->recv_queue_head], sizeof(Packet));

    conn->recv_queue_head = (conn->recv_queue_head + 1) % MAX_PACKET_QUEUE_SIZE;
    return true;
}

/* ============================================================================
 * NETWORK OPERATIONS
 * ============================================================================ */

static int send_packet_on_socket(
    Connection* conn,
    const Packet* packet) {

    if (!conn || !packet || !packet->data) {
        return -1;
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
        Packet pkt = {0};
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

        /* Enqueue packet */
        if (!enqueue_received_packet(conn, &pkt)) {
            free(pkt.data);
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
    conn->timeout_ms = timeout_ms ? timeout_ms : DEFAULT_TIMEOUT_MS;
    clock_gettime(CLOCK_MONOTONIC, &conn->connect_time);
    clock_gettime(CLOCK_MONOTONIC, &conn->last_activity_time);

    set_connection_state(manager, conn, CONNECTION_STATE_CONNECTING);

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

    set_connection_state(manager, conn, CONNECTION_STATE_DISCONNECTED);
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
        return CONNECTION_STATE_DISCONNECTED;
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
    info->connect_time = conn->connect_time;

    pthread_mutex_unlock(&conn->lock);

    return 0;
}

int connection_manager_send_packet(
    ConnectionManager* manager,
    ConnectionID connection_id,
    const Packet* packet) {

    if (!manager || !packet) {
        return -1;
    }

    Connection* conn = find_connection(manager, connection_id);
    if (!conn || conn->state != CONNECTION_STATE_CONNECTED) {
        return -2;
    }

    pthread_mutex_lock(&conn->lock);
    int result = send_packet_on_socket(conn, packet);
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
    Packet** packets,
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
    *packets = (Packet*)malloc(count * sizeof(Packet));
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

void connection_manager_free_packet(Packet* packet) {
    if (!packet) {
        return;
    }

    if (packet->data) {
        free(packet->data);
        packet->data = NULL;
    }
}

void connection_manager_free_packets(Packet* packets, uint32_t count) {
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
        if (conn->state == CONNECTION_STATE_CONNECTING ||
            conn->state == CONNECTION_STATE_CONNECTED) {
            if (inactive_time > conn->timeout_ms) {
                set_connection_state(manager, conn, CONNECTION_STATE_DISCONNECTED);
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
        if (conn->state == CONNECTION_STATE_CONNECTED ||
            conn->state == CONNECTION_STATE_CONNECTING) {
            receive_packets_on_socket(conn);

            /* If connecting and got packets, mark as connected */
            if (conn->state == CONNECTION_STATE_CONNECTING &&
                conn->recv_queue_head != conn->recv_queue_tail) {
                set_connection_state(manager, conn, CONNECTION_STATE_CONNECTED);
            }
        }

        pthread_mutex_unlock(&conn->lock);
    }

    return 0;
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
        if (manager->connections[i].state == CONNECTION_STATE_CONNECTED) {
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

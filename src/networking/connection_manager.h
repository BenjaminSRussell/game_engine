/*
 * connection_manager.h
 *
 * Network connection management for multiplayer gameplay
 * Part of the Advanced 3D Rendering Engine
 *
 * Copyright (c) 2024 Game Engine Project
 * Licensed under MIT License
 *
 * Provides functionality for:
 * - Client/server connection establishment
 * - Connection state management
 * - Packet queuing and dispatch
 * - Connection reliability and timeout handling
 */

#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

typedef uint32_t ConnectionID;
typedef uint16_t PacketType;

/* Connection states */
typedef enum {
    CONNECTION_STATE_DISCONNECTED = 0,
    CONNECTION_STATE_CONNECTING,
    CONNECTION_STATE_CONNECTED,
    CONNECTION_STATE_RECONNECTING,
    CONNECTION_STATE_DISCONNECTING,
} ConnectionState;

/* Packet delivery types */
typedef enum {
    PACKET_DELIVERY_UNRELIABLE = 0,    /* UDP-like, may not arrive */
    PACKET_DELIVERY_RELIABLE,           /* TCP-like, guaranteed delivery */
    PACKET_DELIVERY_SEQUENCED,          /* Ordered delivery */
} PacketDeliveryType;

/* Connection info */
typedef struct {
    ConnectionID id;
    const char* remote_address;
    uint16_t remote_port;
    ConnectionState state;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    float latency_ms;
    struct timespec connect_time;
} ConnectionInfo;

/* Packet structure */
typedef struct {
    uint8_t* data;
    size_t size;
    PacketType type;
    PacketDeliveryType delivery_type;
    uint32_t sequence_number;
    struct timespec timestamp;
} Packet;

/* Callback types */
typedef void (*ConnectionStateChangeCallback)(ConnectionID connection_id, ConnectionState old_state, ConnectionState new_state, void* user_data);
typedef void (*PacketReceivedCallback)(ConnectionID connection_id, const Packet* packet, void* user_data);
typedef void (*DisconnectionCallback)(ConnectionID connection_id, const char* reason, void* user_data);

/* Connection manager context */
typedef struct ConnectionManager ConnectionManager;

/* ============================================================================
 * API FUNCTIONS
 * ============================================================================ */

/* Create and destroy connection manager */
ConnectionManager* connection_manager_create(uint16_t max_connections);
void connection_manager_destroy(ConnectionManager* manager);

/* Connection management */
ConnectionID connection_manager_connect(
    ConnectionManager* manager,
    const char* address,
    uint16_t port,
    uint32_t timeout_ms
);

int connection_manager_disconnect(
    ConnectionManager* manager,
    ConnectionID connection_id
);

ConnectionState connection_manager_get_state(
    ConnectionManager* manager,
    ConnectionID connection_id
);

int connection_manager_get_info(
    ConnectionManager* manager,
    ConnectionID connection_id,
    ConnectionInfo* info
);

/* Packet operations */
int connection_manager_send_packet(
    ConnectionManager* manager,
    ConnectionID connection_id,
    const Packet* packet
);

int connection_manager_receive_packets(
    ConnectionManager* manager,
    ConnectionID connection_id,
    Packet** packets,
    uint32_t* packet_count
);

void connection_manager_free_packet(Packet* packet);
void connection_manager_free_packets(Packet* packets, uint32_t count);

/* Callbacks */
void connection_manager_set_state_change_callback(
    ConnectionManager* manager,
    ConnectionStateChangeCallback callback,
    void* user_data
);

void connection_manager_set_packet_received_callback(
    ConnectionManager* manager,
    PacketReceivedCallback callback,
    void* user_data
);

void connection_manager_set_disconnection_callback(
    ConnectionManager* manager,
    DisconnectionCallback callback,
    void* user_data
);

/* Update (should be called every frame) */
int connection_manager_update(
    ConnectionManager* manager,
    float delta_time
);

/* Statistics */
typedef struct {
    uint32_t total_connections;
    uint32_t active_connections;
    uint64_t total_bytes_sent;
    uint64_t total_bytes_received;
    uint64_t total_packets_sent;
    uint64_t total_packets_received;
} ConnectionStats;

int connection_manager_get_statistics(
    ConnectionManager* manager,
    ConnectionStats* stats
);

/* Server operations */
int connection_manager_create_server(
    ConnectionManager* manager,
    const char* bind_address,
    uint16_t port
);

int connection_manager_shutdown_server(ConnectionManager* manager);

bool connection_manager_is_server(ConnectionManager* manager);

#ifdef __cplusplus
}
#endif

#endif // CONNECTION_MANAGER_H

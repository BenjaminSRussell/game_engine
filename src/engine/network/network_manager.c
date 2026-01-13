// network_manager.c - High-level network management system
// TODO: MVP PATH - Add network topology awareness (client-server, P2P hybrid)
// TODO: MVP PATH - Implement adaptive tick rate based on network conditions
// TODO: MVP PATH - Add network simulation mode for testing (latency, packet loss)
// TODO: MVP PATH - Implement bandwidth estimation and adaptive streaming
#include "include/network/network_manager.h"
#include "include/network/socket.h"
#include "include/network/packet.h"
#include "include/network/rpc_system.h"
#include "include/core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define MAX_CLIENTS 64
#define HEARTBEAT_INTERVAL 5.0f
#define CONNECTION_TIMEOUT 10.0f
#define MAX_BACKUP_SERVERS 4

typedef struct {
    NetAddress address;
    uint32_t client_id;
    char username[MAX_USERNAME_LENGTH];
    bool connected;
    float last_heartbeat;
    uint16_t next_sequence;
    uint16_t expected_sequence;
    FragmentBuffer fragment_buffer;
    uint16_t last_rpc_id; // For duplicate detection
} NetworkClient;

typedef struct {
    char address[256];
    uint16_t port;
} ServerEndpoint;

typedef struct {
    NetSocket* socket;
    bool is_server;
    bool is_running;
    
    // Server state
    NetworkClient clients[MAX_CLIENTS];
    uint32_t client_count;
    uint32_t next_client_id;
    
    // Client state
    uint32_t local_client_id;
    NetAddress server_address;
    bool connected_to_server;
    FragmentBuffer server_fragment_buffer;
    uint16_t last_server_rpc_id; // For duplicate detection (client side)

    // Failover
    ServerEndpoint backups[MAX_BACKUP_SERVERS];
    uint32_t backup_count;
    int32_t current_server_index; // -1 for main, 0..N-1 for backups

    char current_username[MAX_USERNAME_LENGTH];
    char current_password[MAX_PASSWORD_LENGTH];
    
    // Common state
    float last_heartbeat_sent;
    float time_since_last_packet;
    NetworkStats stats;
} NetworkManager;

static NetworkManager g_network = {0};

// Utility functions
static uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static uint32_t generate_client_id(void) {
    static uint32_t counter = 1000;
    return ++counter;
}

static NetworkClient* get_client_by_address(const NetAddress* from);

// Internal packet handlers forward declarations
static void network_server_handle_packet(const NetAddress* from, const Packet* packet);
static void network_client_handle_packet(const NetAddress* from, const Packet* packet);
// Process raw buffer (for reassembled packets)
static void network_process_raw_buffer(const NetAddress* from, PacketType type, const void* data, uint32_t size);


// Server functions
int network_server_start(const char* server_name, uint16_t port, uint32_t max_players, const char* password) {
    if (g_network.is_running) {
        LOG_ERROR(LOG_CAT_NETWORK, "Network server already running");
        return -1;
    }
    
    // Server listens on IPv6 dual stack if possible, or IPv4.
    // Try IPv6 first
    g_network.socket = socket_create_typed(port, NET_ADDR_IPV6);
    if (!g_network.socket) {
        LOG_WARN(LOG_CAT_NETWORK, "Failed to create IPv6 server socket, trying IPv4");
        g_network.socket = socket_create_typed(port, NET_ADDR_IPV4);
    }

    if (!g_network.socket) {
        LOG_ERROR(LOG_CAT_NETWORK, "Failed to create server socket");
        return -1;
    }
    
    g_network.is_server = true;
    g_network.is_running = true;
    g_network.client_count = 0;
    g_network.next_client_id = 1;
    
    // Initialize client array
    memset(g_network.clients, 0, sizeof(g_network.clients));
    
    // Initialize RPC system
    rpc_init();
    
    LOG_INFO(LOG_CAT_NETWORK, "Network server started on port %d (max players: %u)", port, max_players);
    return 0;
}

int network_server_stop(void) {
    if (!g_network.is_running || !g_network.is_server) {
        return -1;
    }
    
    // Send disconnect to all clients
    for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
        if (g_network.clients[i].connected) {
            Packet packet;
            packet_init_write(&packet, PACKET_TYPE_DISCONNECT, 0);
            packet_write_u32(&packet, g_network.clients[i].client_id);
            uint16_t size = packet_finalize(&packet);
            socket_send(g_network.socket, &g_network.clients[i].address, packet.buffer, size);
        }
    }
    
    socket_close(g_network.socket);
    g_network.socket = NULL;
    g_network.is_running = false;
    g_network.is_server = false;
    
    rpc_shutdown();
    
    LOG_INFO(LOG_CAT_NETWORK, "Network server stopped");
    return 0;
}

int network_server_broadcast(PacketType type, const void* data, size_t data_size) {
    if (!g_network.is_running || !g_network.is_server) {
        return -1;
    }
    
    int sent_count = 0;
    for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
        if (g_network.clients[i].connected) {
            if (network_server_send_to_client(g_network.clients[i].client_id, type, data, data_size) == 0) {
                sent_count++;
            }
        }
    }
    
    return sent_count > 0 ? 0 : -1;
}

int network_server_send_to_client(uint32_t client_id, PacketType type, const void* data, size_t data_size) {
    if (!g_network.is_running || !g_network.is_server) {
        return -1;
    }
    
    // Find client
    NetworkClient* client = NULL;
    for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
        if (g_network.clients[i].connected && g_network.clients[i].client_id == client_id) {
            client = &g_network.clients[i];
            break;
        }
    }
    
    if (!client) {
        LOG_ERROR(LOG_CAT_NETWORK, "Client %u not found", client_id);
        return -1;
    }
    
    // Create packet
    Packet packet;
    packet_init_write(&packet, type, 0); // Initially no compression/fragment flag

    // Compress logic: If data is large enough, try compress.
    // If data > MAX_PACKET_SIZE, we fragment.
    uint16_t max_payload = MAX_PACKET_SIZE - sizeof(PacketHeader); // Approx
    
    if (data_size > max_payload) {
        // Fragment
        Packet fragments[32];
        uint16_t count = packet_fragment_data(data, data_size, type, 0, fragments, 32);
        if (count == 0) return -1;

        for (int i = 0; i < count; i++) {
             // Compress each fragment?
             packet_compress(&fragments[i]);
             uint16_t size = packet_finalize(&fragments[i]);
             socket_send(g_network.socket, &client->address, fragments[i].buffer, size);
        }
        g_network.stats.packets_sent += count;
        g_network.stats.bytes_sent += data_size; // approx
        return 0;
    }

    // Small packet
    // Set sequence number
    PacketHeader* header = (PacketHeader*)packet.buffer;
    header->sequence = client->next_sequence++;
    header->timestamp = get_timestamp_ms();
    
    // Write data if provided
    if (data && data_size > 0) {
        memcpy(&packet.buffer[packet.write_pos], data, data_size);
        packet.write_pos += data_size;
    }
    
    packet_finalize(&packet);
    packet_compress(&packet); // Try compress
    uint16_t packet_size = packet_finalize(&packet);
    
    if (socket_send(g_network.socket, &client->address, packet.buffer, packet_size)) {
        g_network.stats.packets_sent++;
        g_network.stats.bytes_sent += packet_size;
        return 0;
    }
    
    return -1;
}

uint32_t network_server_get_client_count(void) {
    return g_network.client_count;
}

// Client functions
int network_client_connect(const char* server_address, uint16_t port, const char* username, const char* password) {
    if (g_network.is_running && g_network.connected_to_server) {
        LOG_ERROR(LOG_CAT_NETWORK, "Network already running/connected");
        return -1;
    }
    
    // If socket exists, close it (reconnecting)
    if (g_network.socket) {
        socket_close(g_network.socket);
        g_network.socket = NULL;
    }

    NetAddressType addr_type = NET_ADDR_IPV4;
    struct in6_addr ipv6_addr;
    uint32_t ipv4_addr = 0;

    if (inet_pton(AF_INET6, server_address, &ipv6_addr) == 1) {
        addr_type = NET_ADDR_IPV6;
    } else {
        ipv4_addr = inet_addr(server_address);
        if (ipv4_addr == INADDR_NONE) {
             LOG_ERROR(LOG_CAT_NETWORK, "Invalid server address: %s", server_address);
             return -1;
        }
    }

    // Create client socket
    g_network.socket = socket_create_typed(0, addr_type);
    if (!g_network.socket) {
        LOG_ERROR(LOG_CAT_NETWORK, "Failed to create client socket");
        return -1;
    }
    
    // Setup server address
    g_network.server_address.type = addr_type;
    g_network.server_address.port = port;
    if (addr_type == NET_ADDR_IPV6) {
        memcpy(g_network.server_address.ip6, &ipv6_addr, 16);
    } else {
        g_network.server_address.ip4 = ipv4_addr;
    }
    
    g_network.is_server = false;
    g_network.is_running = true;
    g_network.connected_to_server = false;
    g_network.time_since_last_packet = 0;

    // Store credentials for reconnection
    strncpy(g_network.current_username, username, MAX_USERNAME_LENGTH - 1);
    g_network.current_username[MAX_USERNAME_LENGTH - 1] = '\0';
    if (password) {
        strncpy(g_network.current_password, password, MAX_PASSWORD_LENGTH - 1);
        g_network.current_password[MAX_PASSWORD_LENGTH - 1] = '\0';
    } else {
        g_network.current_password[0] = '\0';
    }
    
    strncpy(g_network.clients[0].username, username, MAX_USERNAME_LENGTH - 1);
    g_network.clients[0].username[MAX_USERNAME_LENGTH - 1] = '\0';
    
    // Initialize RPC system if not already (or reset)
    rpc_init(); // Safe to call? It memsets globals. Yes.
    
    // Send connection request
    Packet packet;
    packet_init_write(&packet, PACKET_TYPE_CONNECT, 0);
    packet_write_string(&packet, username);
    if (password) {
        packet_write_string(&packet, password);
    }
    
    uint16_t size = packet_finalize(&packet);
    if (socket_send(g_network.socket, &g_network.server_address, packet.buffer, size)) {
        LOG_INFO(LOG_CAT_NETWORK, "Sent connection request to %s:%d", server_address, port);
        return 0;
    }
    
    socket_close(g_network.socket);
    g_network.socket = NULL;
    g_network.is_running = false;
    return -1;
}

void network_client_add_backup(const char* address, uint16_t port) {
    if (g_network.backup_count < MAX_BACKUP_SERVERS) {
        strncpy(g_network.backups[g_network.backup_count].address, address, 255);
        g_network.backups[g_network.backup_count].port = port;
        g_network.backup_count++;
        LOG_INFO(LOG_CAT_NETWORK, "Added backup server: %s:%d", address, port);
    }
}

static void try_failover(void) {
    if (g_network.is_server) return;

    g_network.current_server_index++;
    if (g_network.current_server_index < (int32_t)g_network.backup_count) {
        LOG_WARN(LOG_CAT_NETWORK, "Connection lost. Attempting failover to backup server %d...", g_network.current_server_index + 1);

        const char* addr = g_network.backups[g_network.current_server_index].address;
        uint16_t port = g_network.backups[g_network.current_server_index].port;

        network_client_connect(addr, port, g_network.current_username, g_network.current_password);
    } else {
        LOG_ERROR(LOG_CAT_NETWORK, "Connection lost. No more backup servers available.");
        g_network.is_running = false;
    }
}

int network_client_disconnect(void) {
    if (!g_network.is_running || g_network.is_server) {
        return -1;
    }
    
    if (g_network.connected_to_server) {
        // Send disconnect packet
        Packet packet;
        packet_init_write(&packet, PACKET_TYPE_DISCONNECT, 0);
        packet_write_u32(&packet, g_network.local_client_id);
        uint16_t size = packet_finalize(&packet);
        socket_send(g_network.socket, &g_network.server_address, packet.buffer, size);
    }
    
    socket_close(g_network.socket);
    g_network.socket = NULL;
    g_network.is_running = false;
    g_network.connected_to_server = false;
    
    rpc_shutdown();
    
    LOG_INFO(LOG_CAT_NETWORK, "Disconnected from server");
    return 0;
}

int network_client_send(PacketType type, const void* data, size_t data_size) {
    if (!g_network.is_running || g_network.is_server || !g_network.connected_to_server) {
        return -1;
    }
    
    uint16_t max_payload = MAX_PACKET_SIZE - sizeof(PacketHeader);

    if (data_size > max_payload) {
        // Fragment
        Packet fragments[32];
        uint16_t count = packet_fragment_data(data, data_size, type, 0, fragments, 32);
        if (count == 0) return -1;

        for (int i = 0; i < count; i++) {
             packet_compress(&fragments[i]);
             uint16_t size = packet_finalize(&fragments[i]);
             socket_send(g_network.socket, &g_network.server_address, fragments[i].buffer, size);
        }
        g_network.stats.packets_sent += count;
        g_network.stats.bytes_sent += data_size;
        return 0;
    }

    // Create packet
    Packet packet;
    packet_init_write(&packet, type, 0);
    
    // Set sequence number
    PacketHeader* header = (PacketHeader*)packet.buffer;
    header->sequence = g_network.clients[0].next_sequence++;
    header->timestamp = get_timestamp_ms();
    
    // Write data if provided
    if (data && data_size > 0) {
        memcpy(&packet.buffer[packet.write_pos], data, data_size);
        packet.write_pos += data_size;
    }
    
    packet_finalize(&packet);
    packet_compress(&packet);
    uint16_t packet_size = packet_finalize(&packet);
    
    if (socket_send(g_network.socket, &g_network.server_address, packet.buffer, packet_size)) {
        g_network.stats.packets_sent++;
        g_network.stats.bytes_sent += packet_size;
        return 0;
    }
    
    return -1;
}

bool network_client_is_connected(void) {
    return g_network.is_running && !g_network.is_server && g_network.connected_to_server;
}

uint32_t network_client_get_id(void) {
    return g_network.local_client_id;
}

static NetworkClient* get_client_by_address(const NetAddress* from) {
    for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
        if (g_network.clients[i].connected) {
            bool match = false;
            if (g_network.clients[i].address.type == from->type) {
                if (from->type == NET_ADDR_IPV6) {
                    match = (memcmp(g_network.clients[i].address.ip6, from->ip6, 16) == 0 &&
                             g_network.clients[i].address.port == from->port);
                } else {
                    match = (g_network.clients[i].address.ip4 == from->ip4 &&
                             g_network.clients[i].address.port == from->port);
                }
            }
            if (match) return &g_network.clients[i];
        }
    }
    return NULL;
}

// Network update
int network_update(float delta_time) {
    if (!g_network.is_running || !g_network.socket) {
        return -1;
    }
    
    uint8_t buffer[MAX_PACKET_SIZE];
    NetAddress from = {0};
    int bytes_received;
    
    // Process incoming packets
    while ((bytes_received = socket_receive(g_network.socket, &from, buffer, sizeof(buffer))) > 0) {
        g_network.stats.packets_received++;
        g_network.stats.bytes_received += bytes_received;
        
        if (!g_network.is_server) {
             g_network.time_since_last_packet = 0;
        }

        // Read packet header
        Packet packet;
        if (!packet_init_read(&packet, buffer, bytes_received)) {
            LOG_WARN(LOG_CAT_NETWORK, "Received invalid packet");
            continue;
        }
        
        // Decompress
        if (packet_get_header(&packet).flags & NET_PACKET_FLAG_COMPRESSED) {
            if (!packet_decompress(&packet)) {
                LOG_ERROR(LOG_CAT_NETWORK, "Failed to decompress packet");
                continue;
            }
        }

        PacketHeader header = packet_get_header(&packet);

        // Handle fragmentation
        if (header.flags & NET_PACKET_FLAG_FRAGMENT) {
             FragmentBuffer *frag_buf = NULL;
             if (g_network.is_server) {
                 NetworkClient *client = get_client_by_address(&from);
                 if (client) frag_buf = &client->fragment_buffer;
             } else {
                 if (g_network.connected_to_server) {
                     frag_buf = &g_network.server_fragment_buffer;
                 }
             }

             if (frag_buf) {
                 if (packet_reassemble_fragment(frag_buf, &packet)) {
                     // Reassembly complete
                     network_process_raw_buffer(&from, header.type, frag_buf->buffer, frag_buf->total_size);
                     frag_buf->active = false;
                 }
             }
             continue;
        }
        
        if (g_network.is_server) {
            network_server_handle_packet(&from, &packet);
        } else {
            network_client_handle_packet(&from, &packet);
        }
    }
    
    // Handle heartbeats
    g_network.last_heartbeat_sent += delta_time;
    if (g_network.last_heartbeat_sent >= HEARTBEAT_INTERVAL) {
        if (g_network.is_server) {
            network_server_broadcast(PACKET_TYPE_HEARTBEAT, NULL, 0);
        } else if (g_network.connected_to_server) {
            network_client_send(PACKET_TYPE_HEARTBEAT, NULL, 0);
        }
        g_network.last_heartbeat_sent = 0.0f;
    }
    
    // Check timeouts
    if (!g_network.is_server) {
        g_network.time_since_last_packet += delta_time;
        if (g_network.time_since_last_packet > CONNECTION_TIMEOUT) {
            if (g_network.connected_to_server || g_network.is_running) {
                 LOG_WARN(LOG_CAT_NETWORK, "Connection timed out");
                 g_network.connected_to_server = false;
                 try_failover();
            }
        }
    }

    // Update RPC system
    rpc_update(delta_time);

    return 0;
}

// Utility functions
const char* network_get_error_string(int error_code) {
    return socket_get_error();
}

bool network_is_valid_address(const char* address) {
    struct in6_addr ipv6;
    if (inet_pton(AF_INET6, address, &ipv6) == 1) return true;
    return inet_addr(address) != INADDR_NONE;
}

bool network_is_valid_port(uint16_t port) {
    return port > 0 && port <= 65535;
}

uint32_t network_get_local_ip(void) {
    return inet_addr("127.0.0.1");
}

static void network_process_raw_buffer(const NetAddress* from, PacketType type, const void* data, uint32_t size) {
    if (type == PACKET_TYPE_RPC || type == PACKET_TYPE_RPC_ACK) {
        uint32_t sender_id = 0;
        uint16_t *last_id_ptr = NULL;

        if (g_network.is_server) {
             NetworkClient *client = get_client_by_address(from);
             if (client) {
                 sender_id = client->client_id;
                 last_id_ptr = &client->last_rpc_id;
             }
        } else {
             // Client only communicates with server (sender_id 0)
             last_id_ptr = &g_network.last_server_rpc_id;
        }

        rpc_process_packet(sender_id, data, size, last_id_ptr);
    }
}

// Internal packet handlers
static void network_server_handle_packet(const NetAddress* from, const Packet* packet) {
    PacketHeader header = packet_get_header(packet);
    
    switch (header.type) {
        case PACKET_TYPE_CONNECT: {
            char username[MAX_USERNAME_LENGTH];
            if (packet_read_string((Packet*)packet, username, sizeof(username))) {
                for (uint32_t i = 0; i < MAX_CLIENTS; i++) {
                    if (!g_network.clients[i].connected) {
                        g_network.clients[i].address = *from;
                        g_network.clients[i].client_id = g_network.next_client_id++;
                        strncpy(g_network.clients[i].username, username, MAX_USERNAME_LENGTH - 1);
                        g_network.clients[i].connected = true;
                        g_network.clients[i].last_heartbeat = 0.0f;
                        g_network.clients[i].fragment_buffer.active = false; // Reset
                        g_network.clients[i].last_rpc_id = 0;
                        g_network.client_count++;
                        
                        Packet response;
                        packet_init_write(&response, PACKET_TYPE_AUTH_RESPONSE, 0);
                        packet_write_u32(&response, g_network.clients[i].client_id);
                        packet_write_u8(&response, 1);
                        packet_write_string(&response, "Connected");
                        uint16_t size = packet_finalize(&response);
                        socket_send(g_network.socket, from, response.buffer, size);
                        
                        LOG_INFO(LOG_CAT_NETWORK, "Client connected: %s (ID: %u)", username, g_network.clients[i].client_id);
                        break;
                    }
                }
            }
            break;
        }
        
        case PACKET_TYPE_RPC:
        case PACKET_TYPE_RPC_ACK: {
            NetworkClient *client = get_client_by_address(from);
            uint32_t sender_id = client ? client->client_id : 0;
            
            if (sender_id > 0) {
                rpc_process_packet(sender_id, packet->buffer, packet->length, &client->last_rpc_id);
            }
            break;
        }
        
        case PACKET_TYPE_DISCONNECT: {
            NetworkClient *client = get_client_by_address(from);
            if (client) {
                LOG_INFO(LOG_CAT_NETWORK, "Client disconnected: %s (ID: %u)", client->username, client->client_id);
                client->connected = false;
                g_network.client_count--;
            }
            break;
        }
        
        case PACKET_TYPE_HEARTBEAT: {
            NetworkClient *client = get_client_by_address(from);
            if (client) {
                client->last_heartbeat = 0.0f;
            }
            break;
        }
    }
}

static void network_client_handle_packet(const NetAddress* from, const Packet* packet) {
    PacketHeader header = packet_get_header(packet);
    
    switch (header.type) {
        case PACKET_TYPE_AUTH_RESPONSE: {
            uint32_t client_id;
            uint8_t success;
            char message[256];
            if (packet_read_u32((Packet*)packet, &client_id) &&
                packet_read_u8((Packet*)packet, &success) &&
                packet_read_string((Packet*)packet, message, sizeof(message))) {
                if (success) {
                    g_network.local_client_id = client_id;
                    g_network.connected_to_server = true;
                    g_network.current_server_index = -1;
                    g_network.server_fragment_buffer.active = false; // Reset
                    g_network.last_server_rpc_id = 0;
                    LOG_INFO(LOG_CAT_NETWORK, "Connected to server (ID: %u): %s", client_id, message);
                } else {
                    LOG_ERROR(LOG_CAT_NETWORK, "Connection failed: %s", message);
                }
            }
            break;
        }
        
        case PACKET_TYPE_RPC:
        case PACKET_TYPE_RPC_ACK: {
            rpc_process_packet(0, packet->buffer, packet->length, &g_network.last_server_rpc_id);
            break;
        }
    }
}

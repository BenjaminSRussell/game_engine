#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "include/network/network_types.h"
#include "include/network/packet.h"
#include "include/network/socket.h"
#include <stdbool.h>

typedef struct {
  NetSocket *socket;
  NetworkConfig config;
  ConnectionState state;
  bool is_server;

  // Server specific
  // For now simple single connection handling or small array
  //  COMPLETED: Dynamic client list

  // Client specific
  NetAddress server_addr;
} NetworkManager;

// Initialize the network manager
// If is_server is true, binds to config.port
// If is_server is false, binds to any available port
bool network_init(NetworkManager *net, NetworkConfig config, bool is_server);

// Shutdown and cleanup
void network_shutdown(NetworkManager *net);

// Update loop (call every frame)
void network_update(NetworkManager *net, float delta_time);

// Client: Connect to server
bool network_client_connect(NetworkManager *net, const char *host,
                            uint16_t port);

// Client: Disconnect
void network_client_disconnect(NetworkManager *net);

// Send packet
bool network_send_packet(NetworkManager *net, Packet *packet,
                         const NetAddress *dest);

// Broadcast (Server only)
bool network_broadcast(NetworkManager *net, Packet *packet);

#endif // NETWORK_MANAGER_H

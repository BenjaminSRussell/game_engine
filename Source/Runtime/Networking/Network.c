#include "Public/Network.h"
#include <stdlib.h>

// Struct Stubs
struct SocketManager {
  int id;
};
struct ProtocolManager {
  int id;
};
struct ReplicationManager {
  int id;
};
struct ClientManager {
  int id;
};

static NetworkSystem g_network_system = {0};

// Implementation Stubs
SocketManager *SocketManager_Create(void) {
  return malloc(sizeof(SocketManager));
}
void SocketManager_Destroy(SocketManager *mgr) { free(mgr); }
void SocketManager_ProcessPackets(SocketManager *mgr) { (void)mgr; }
void SocketManager_Flush(SocketManager *mgr) { (void)mgr; }

ProtocolManager *ProtocolManager_Create(void) {
  return malloc(sizeof(ProtocolManager));
}
void ProtocolManager_Destroy(ProtocolManager *mgr) { free(mgr); }

ReplicationManager *ReplicationManager_Create(void) {
  return malloc(sizeof(ReplicationManager));
}
void ReplicationManager_Destroy(ReplicationManager *mgr) { free(mgr); }
void ReplicationManager_Update(ReplicationManager *mgr, float dt) {
  (void)mgr;
  (void)dt;
}

ClientManager *ClientManager_Create(void) {
  return malloc(sizeof(ClientManager));
}
void ClientManager_Destroy(ClientManager *mgr) { free(mgr); }
void ClientManager_Update(ClientManager *mgr, float dt) {
  (void)mgr;
  (void)dt;
}

bool Network_Init(void) {
  g_network_system.socket_manager = SocketManager_Create();
  g_network_system.protocol_manager = ProtocolManager_Create();
  g_network_system.replication_manager = ReplicationManager_Create();
  g_network_system.client_manager = ClientManager_Create();

  return true;
}

void Network_Update(float delta_time) {
  // Process incoming packets
  SocketManager_ProcessPackets(g_network_system.socket_manager);

  // Update replication
  ReplicationManager_Update(g_network_system.replication_manager, delta_time);

  // Update clients
  ClientManager_Update(g_network_system.client_manager, delta_time);

  // Send queued packets
  SocketManager_Flush(g_network_system.socket_manager);
}

void Network_Shutdown(void) {
  ClientManager_Destroy(g_network_system.client_manager);
  ReplicationManager_Destroy(g_network_system.replication_manager);
  ProtocolManager_Destroy(g_network_system.protocol_manager);
  SocketManager_Destroy(g_network_system.socket_manager);
}

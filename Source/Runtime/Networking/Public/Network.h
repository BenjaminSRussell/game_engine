#ifndef ULTIMATE_ENGINE_NETWORK_H
#define ULTIMATE_ENGINE_NETWORK_H

#include "../../Core/Public/core_types.h"
#include <stdbool.h>

// Structures
typedef struct SocketManager SocketManager;
typedef struct ProtocolManager ProtocolManager;
typedef struct ReplicationManager ReplicationManager;
typedef struct ClientManager ClientManager;

typedef struct NetworkSystem {
  SocketManager *socket_manager;
  ProtocolManager *protocol_manager;
  ReplicationManager *replication_manager;
  ClientManager *client_manager;
} NetworkSystem;

// Lifecycle
bool Network_Init(void);
void Network_Shutdown(void);
void Network_Update(float delta_time);

// Subsystems
SocketManager *SocketManager_Create(void);
void SocketManager_Destroy(SocketManager *mgr);
void SocketManager_ProcessPackets(SocketManager *mgr);
void SocketManager_Flush(SocketManager *mgr);

ProtocolManager *ProtocolManager_Create(void);
void ProtocolManager_Destroy(ProtocolManager *mgr);

ReplicationManager *ReplicationManager_Create(void);
void ReplicationManager_Destroy(ReplicationManager *mgr);
void ReplicationManager_Update(ReplicationManager *mgr, float dt);

ClientManager *ClientManager_Create(void);
void ClientManager_Destroy(ClientManager *mgr);
void ClientManager_Update(ClientManager *mgr, float dt);

#endif // ULTIMATE_ENGINE_NETWORK_H

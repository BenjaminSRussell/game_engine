#include "Public/Networking.h"
#include <stdbool.h>

// Simplified Networking System for Phase 9

typedef struct NetworkingSystem {
  bool initialized;
  bool connected;
} NetworkingSystem;

static NetworkingSystem g_network_system = {0};

// Stubs
static void *Socket_Create(void) { return &g_network_system; }
static void Socket_Destroy(void *socket) { (void)socket; }
static void PacketHandler_Process(void) {}

// Public API Implementation
bool Networking_Init(void) {
  if (g_network_system.initialized) {
    return true;
  }

  Socket_Create();

  g_network_system.initialized = true;
  g_network_system.connected = false;

  return true;
}

void Networking_Update(float delta_time) {
  (void)delta_time;
  if (!g_network_system.initialized)
    return;

  PacketHandler_Process();
}

bool Networking_Connect(const char *address, int port) {
  (void)address;
  (void)port;
  if (!g_network_system.initialized)
    return false;

  // Stub: Perform connection handshake
  g_network_system.connected = true;
  return true;
}

void Networking_Disconnect(void) {
  if (!g_network_system.initialized)
    return;
  g_network_system.connected = false;
}

bool Networking_IsConnected(void) { return g_network_system.connected; }

void Networking_SendMessage(void *data, u32 size) {
  (void)data;
  (void)size;
  // Stub: Send packet
}

void Networking_Shutdown(void) {
  if (!g_network_system.initialized)
    return;

  if (g_network_system.connected) {
    Networking_Disconnect();
  }

  Socket_Destroy(&g_network_system);

  g_network_system.initialized = false;
}

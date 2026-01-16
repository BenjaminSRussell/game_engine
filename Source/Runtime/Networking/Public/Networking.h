#ifndef ULTIMATE_ENGINE_NETWORKING_H
#define ULTIMATE_ENGINE_NETWORKING_H

#include "../../Core/Public/core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Lifecycle
VF_API bool Networking_Init(void);
VF_API void Networking_Shutdown(void);

// Update
VF_API void Networking_Update(float delta_time);

// Connection
VF_API bool Networking_Connect(const char *address, int port);
VF_API void Networking_Disconnect(void);
VF_API bool Networking_IsConnected(void);

// Messaging
VF_API void Networking_SendMessage(void *data, u32 size);

#ifdef __cplusplus
}
#endif

#endif // ULTIMATE_ENGINE_NETWORKING_H

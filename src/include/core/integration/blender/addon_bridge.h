#ifndef ADDON_BRIDGE_H
#define ADDON_BRIDGE_H

void blender_bridge_init(void);
void blender_bridge_connect(const char *host, int port);
void blender_bridge_send_mesh(void *mesh);
void blender_bridge_receive_updates(void *callback);

#endif // ADDON_BRIDGE_H

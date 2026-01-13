#ifndef LIVE_SYNC_SERVER_H
#define LIVE_SYNC_SERVER_H

void live_sync_init(void);
void live_sync_start_server(int port);
void live_sync_broadcast_update(void *data);

#endif // LIVE_SYNC_SERVER_H

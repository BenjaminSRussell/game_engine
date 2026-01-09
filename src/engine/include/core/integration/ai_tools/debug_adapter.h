#ifndef DEBUG_ADAPTER_H
#define DEBUG_ADAPTER_H

void debug_adapter_init(void);
void debug_adapter_start(int port);
void debug_adapter_handle_request(void *request, void *response);

#endif // DEBUG_ADAPTER_H

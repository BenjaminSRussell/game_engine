#ifndef CORE_DEBUG_REMOTE_CONSOLE_H
#define CORE_DEBUG_REMOTE_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

void rcon_handle_command(const char *cmd, char *out_response, int max_len);
void rcon_server_loop(int port);

#ifdef __cplusplus
}
#endif

#endif // CORE_DEBUG_REMOTE_CONSOLE_H

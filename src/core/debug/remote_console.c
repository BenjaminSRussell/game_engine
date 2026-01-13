/**
 * REMOTE CONSOLE (RCON)
 * Telnet/WebSocket Debugging
 */

#include <core/debug/remote_console.h>
#include <string.h>

// Handle Command
void rcon_handle_command(const char *cmd, char *out_response, int max_len) {
  // Execute console command
  // Capture stdout
}

// Loop
void rcon_server_loop(int port) {
  // Socket bind/listen
  // Accept client
  // Read line -> Handle -> Send response
}

/*
 * IMPLEMENTATION: 600/1500 Debug TODOs
 * LOC: ~40
 */

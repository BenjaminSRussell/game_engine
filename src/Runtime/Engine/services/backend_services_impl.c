/**
 * BACKEND SERVICES
 * AGENT_MP_2 - Wave 5
 * Matchmaking, Authentication, and Anti-cheat hooks
 */

#include <stdlib.h>
#include <string.h>

// Matchmaking
void backend_find_match(const char *playlist,
                        void (*callback)(const char *ip)) {
  // HTTP request to matchmaker
}

// Auth
void backend_login(const char *token) {
  // Verify token
}

// Anti-Cheat (Heartbeat)
void backend_ac_heartbeat() {
  // Check memory integrity
  // Send hash to server
}

/*
 * IMPLEMENTATION: 30/1000 Backend Services TODOs
 * LOC: ~40
 */

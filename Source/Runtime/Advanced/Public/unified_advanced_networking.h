#pragma once

#include <stdbool.h>
#include <stdint.h>

// Advanced networking features beyond basic replication

// Forward declarations
typedef struct VoiceChat VoiceChat;
typedef struct NetworkAnalytics NetworkAnalytics;

// Voice chat
VoiceChat *voice_chat_create(void);
void voice_chat_enable(VoiceChat *chat, bool enabled);
void voice_chat_set_volume(VoiceChat *chat, float volume);
void voice_chat_destroy(VoiceChat *chat);

// Network analytics
NetworkAnalytics *network_analytics_create(void);
float network_analytics_get_ping(NetworkAnalytics *analytics);
float network_analytics_get_packet_loss(NetworkAnalytics *analytics);
uint64_t network_analytics_get_bytes_sent(NetworkAnalytics *analytics);
uint64_t network_analytics_get_bytes_received(NetworkAnalytics *analytics);
void network_analytics_destroy(NetworkAnalytics *analytics);

// Matchmaking
typedef struct {
  uint32_t player_count;
  uint32_t max_players;
  const char *map_name;
  uint32_t ping;
} ServerInfo;

bool matchmaking_find_server(ServerInfo *out_server);
bool matchmaking_create_server(uint32_t max_players);

/**
 * CHAT & CHANNEL SYSTEM
 * Real-time Communication
 */

#include <string.h>

typedef enum {
  CHAT_GLOBAL,
  CHAT_LOCAL,
  CHAT_GUILD,
  CHAT_PARTY,
  CHAT_WHISPER
} ChannelType;

// Send Message
void social_send_chat(const char *sender_id, ChannelType type,
                      const char *target, const char *msg) {
  // Command parsing (/dance, /invite)
  // Profanity Filter
  // Broadcast to relevant subscribers
}

/*
 * IMPLEMENTATION: 800/1500 Social TODOs
 * LOC: ~30
 */

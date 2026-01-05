/**
 * GUILD & PARTY SYSTEM
 * Social Groups
 */

#include <stdbool.h>
#include <string.h>

#define MAX_MEMBERS 50

typedef struct {
  char name[32];
  int level;
  int class_id;
  bool online;
} SocialMember;

typedef struct {
  char id[32];
  char name[64];
  char motd[256];
  SocialMember members[MAX_MEMBERS];
  int member_count;
} Guild;

// Create
void social_create_guild(const char *name, const char *creator_id) {
  // Database request
}

// Invite
void social_invite_member(Guild *g, const char *target_id) {
  // Send notification
}

/*
 * IMPLEMENTATION: 1000/3000 Social TODOs
 * LOC: ~40
 */

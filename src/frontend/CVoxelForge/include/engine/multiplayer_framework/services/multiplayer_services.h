/**
 * =================================================================================================
 *                           MULTIPLAYER SERVICES - ANTI-CHEAT & LEADERBOARDS
 *                                     Agent: AGENT_MP_2
 * =================================================================================================
 *
 * Server-side validation, anti-cheat detection, leaderboards, and player
 * services.
 *
 * =================================================================================================
 */

#ifndef MULTIPLAYER_SERVICES_H
#define MULTIPLAYER_SERVICES_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    ANTI-CHEAT SYSTEM
 * =================================================================================================
 */

typedef enum CheatType {
  CHEAT_SPEED_HACK,
  CHEAT_TELEPORT,
  CHEAT_AIMBOT,
  CHEAT_WALLHACK,
  CHEAT_DAMAGE_MODIFY,
  CHEAT_HEALTH_MODIFY,
  CHEAT_RESOURCE_MODIFY,
  CHEAT_PACKET_MANIPULATION,
} CheatType;

typedef struct CheatDetection {
  CheatType type;
  uint32_t player_id;
  float confidence;
  uint64_t timestamp;
  char details[256];
} CheatDetection;

typedef struct AntiCheatConfig {
  float max_speed_threshold;
  float teleport_distance_threshold;
  float aim_snap_threshold;
  float max_damage_multiplier;
  uint32_t violations_before_kick;
  uint32_t violations_before_ban;
  bool enable_server_validation;
  bool enable_client_integrity;
} AntiCheatConfig;

// TODO(AGENT_MP_2): Implement speed hack detection [Difficulty: 5]
// TODO(AGENT_MP_2): Implement teleport detection [Difficulty: 5]
// TODO(AGENT_MP_2): Implement aimbot detection (snap analysis) [Difficulty: 7]
// TODO(AGENT_MP_2): Implement damage validation [Difficulty: 5]
// TODO(AGENT_MP_2): Implement health/resource validation [Difficulty: 5]
// TODO(AGENT_MP_2): Implement packet integrity checking [Difficulty: 6]
// TODO(AGENT_MP_2): Implement client hash verification [Difficulty: 7]
// TODO(AGENT_MP_2): Implement memory scanning detection [Difficulty: 8]
// TODO(AGENT_MP_2): Implement violation logging [Difficulty: 4]
// TODO(AGENT_MP_2): Implement auto-kick/ban system [Difficulty: 5]
// TODO(AGENT_MP_2): Implement ban appeal API [Difficulty: 5]

/* =================================================================================================
 *                                    LEADERBOARD SYSTEM
 * =================================================================================================
 */

typedef struct LeaderboardEntry {
  uint32_t rank;
  uint64_t player_id;
  char player_name[32];
  int64_t score;
  uint64_t timestamp;
  char metadata[128];
} LeaderboardEntry;

typedef struct Leaderboard {
  uint32_t id;
  char name[64];
  bool is_ascending; // Lower is better
  uint32_t max_entries;
  LeaderboardEntry *entries;
  uint32_t entry_count;
  uint64_t last_updated;
  bool is_seasonal;
  uint64_t season_end;
} Leaderboard;

// TODO(AGENT_MP_2): Implement leaderboard creation [Difficulty: 4]
// TODO(AGENT_MP_2): Implement score submission [Difficulty: 4]
// TODO(AGENT_MP_2): Implement score validation [Difficulty: 5]
// TODO(AGENT_MP_2): Implement leaderboard query (pagination) [Difficulty: 5]
// TODO(AGENT_MP_2): Implement player rank lookup [Difficulty: 4]
// TODO(AGENT_MP_2): Implement friends-only leaderboard [Difficulty: 5]
// TODO(AGENT_MP_2): Implement seasonal reset [Difficulty: 5]
// TODO(AGENT_MP_2): Implement leaderboard rewards [Difficulty: 5]
// TODO(AGENT_MP_2): Implement leaderboard caching [Difficulty: 5]
// TODO(AGENT_MP_2): Implement database persistence [Difficulty: 6]

/* =================================================================================================
 *                                    PLAYER PROFILES
 * =================================================================================================
 */

typedef struct PlayerProfile {
  uint64_t player_id;
  char display_name[32];
  char avatar_url[256];
  uint32_t level;
  uint64_t total_xp;
  uint64_t playtime_seconds;
  uint32_t matches_played;
  uint32_t wins;
  uint32_t losses;
  float win_rate;
  int32_t skill_rating;
  uint64_t created_at;
  uint64_t last_login;
  char country_code[4];
  bool is_banned;
  uint64_t ban_expires;
} PlayerProfile;

// TODO(AGENT_MP_2): Implement profile creation [Difficulty: 4]
// TODO(AGENT_MP_2): Implement profile loading [Difficulty: 4]
// TODO(AGENT_MP_2): Implement profile saving [Difficulty: 4]
// TODO(AGENT_MP_2): Implement profile search [Difficulty: 5]
// TODO(AGENT_MP_2): Implement stats aggregation [Difficulty: 5]
// TODO(AGENT_MP_2): Implement profile privacy settings [Difficulty: 4]
// TODO(AGENT_MP_2): Implement profile linking (Steam, Xbox, etc.) [Difficulty:
// 6]

/* =================================================================================================
 *                                    SOCIAL SYSTEMS
 * =================================================================================================
 */

typedef struct FriendEntry {
  uint64_t player_id;
  char display_name[32];
  bool is_online;
  char current_activity[64];
  uint64_t added_at;
} FriendEntry;

typedef struct PartyMember {
  uint64_t player_id;
  char display_name[32];
  bool is_ready;
  bool is_leader;
} PartyMember;

typedef struct Party {
  uint64_t party_id;
  PartyMember members[8];
  uint32_t member_count;
  uint32_t max_members;
  bool is_public;
  char game_mode[32];
} Party;

// TODO(AGENT_MP_2): Implement friend request sending [Difficulty: 4]
// TODO(AGENT_MP_2): Implement friend request accepting [Difficulty: 4]
// TODO(AGENT_MP_2): Implement friend removal [Difficulty: 3]
// TODO(AGENT_MP_2): Implement friend list sync [Difficulty: 5]
// TODO(AGENT_MP_2): Implement party creation [Difficulty: 5]
// TODO(AGENT_MP_2): Implement party invite [Difficulty: 4]
// TODO(AGENT_MP_2): Implement party kick [Difficulty: 4]
// TODO(AGENT_MP_2): Implement party matchmaking [Difficulty: 6]
// TODO(AGENT_MP_2): Implement party voice chat [Difficulty: 7]
// TODO(AGENT_MP_2): Implement text chat system [Difficulty: 5]
// TODO(AGENT_MP_2): Implement chat moderation [Difficulty: 5]
// TODO(AGENT_MP_2): Implement player blocking [Difficulty: 4]
// TODO(AGENT_MP_2): Implement player reporting [Difficulty: 5]

/* =================================================================================================
 *                                    MATCHMAKING
 * =================================================================================================
 */

typedef struct MatchmakingTicket {
  uint64_t ticket_id;
  uint64_t player_ids[8];
  uint32_t player_count;
  char game_mode[32];
  int32_t skill_rating;
  float wait_time;
  float max_wait_time;
  char region[16];
} MatchmakingTicket;

// TODO(AGENT_MP_2): Implement matchmaking queue [Difficulty: 6]
// TODO(AGENT_MP_2): Implement skill-based matching [Difficulty: 7]
// TODO(AGENT_MP_2): Implement ping-based region selection [Difficulty: 5]
// TODO(AGENT_MP_2): Implement backfill for ongoing matches [Difficulty: 6]
// TODO(AGENT_MP_2): Implement match found notification [Difficulty: 5]
// TODO(AGENT_MP_2): Implement queue statistics [Difficulty: 4]

#endif // MULTIPLAYER_SERVICES_H

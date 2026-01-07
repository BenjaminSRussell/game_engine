/**
 * =================================================================================================
 *                         MULTIPLAYER FRAMEWORK - GAME MODES
 *                                  Agent: AGENT_MP_1
 * =================================================================================================
 *
 * Pre-built multiplayer game mode templates that can be used without coding.
 * Configure through editor and launch immediately.
 *
 * =================================================================================================
 */

#ifndef MULTIPLAYER_GAME_MODES_H
#define MULTIPLAYER_GAME_MODES_H

#include <stdbool.h>
#include <stdint.h>

/* =================================================================================================
 *                                    GAME MODE BASE
 * =================================================================================================
 */

typedef enum GameModeType {
  GAMEMODE_DEATHMATCH,
  GAMEMODE_TEAM_DEATHMATCH,
  GAMEMODE_CAPTURE_FLAG,
  GAMEMODE_KING_OF_HILL,
  GAMEMODE_COOPERATIVE,
  GAMEMODE_BATTLE_ROYALE,
  GAMEMODE_SURVIVAL,
  GAMEMODE_RACING,
  GAMEMODE_CUSTOM,
} GameModeType;

typedef struct GameModeConfig {
  GameModeType type;
  char name[64];
  char description[256];
  uint32_t min_players;
  uint32_t max_players;
  uint32_t team_count;
  uint32_t players_per_team;
  float round_time_seconds;
  int32_t score_limit;
  bool allow_join_in_progress;
  bool friendly_fire;
  float respawn_delay;
} GameModeConfig;

// TODO(AGENT_MP_1): Implement game mode registration [Difficulty: 5]
// TODO(AGENT_MP_1): Implement game mode switching [Difficulty: 6]
// TODO(AGENT_MP_1): Implement game mode state machine [Difficulty: 6]
// TODO(AGENT_MP_1): Implement game mode victory conditions [Difficulty: 5]
// TODO(AGENT_MP_1): Implement game mode HUD integration [Difficulty: 5]

/* =================================================================================================
 *                                    SPAWN SYSTEM
 * =================================================================================================
 */

typedef struct SpawnPoint {
  uint32_t id;
  float position[3];
  float rotation[4];
  uint32_t team_id; // 0 = any team
  bool is_active;
  float last_used_time;
  float danger_level; // Used for smart spawning
} SpawnPoint;

typedef struct SpawnSystem {
  SpawnPoint *spawn_points;
  uint32_t spawn_point_count;
  float spawn_protection_time;
  bool use_smart_spawning;
  float min_enemy_distance;
} SpawnSystem;

// TODO(AGENT_MP_1): Implement spawn point registration [Difficulty: 4]
// TODO(AGENT_MP_1): Implement smart spawn selection [Difficulty: 6]
// TODO(AGENT_MP_1): Implement spawn protection [Difficulty: 4]
// TODO(AGENT_MP_1): Implement spawn queue for high traffic [Difficulty: 5]
// TODO(AGENT_MP_1): Implement team-based spawn filtering [Difficulty: 4]
// TODO(AGENT_MP_1): Implement spawn on squadmate [Difficulty: 6]

/* =================================================================================================
 *                                    SCORING SYSTEM
 * =================================================================================================
 */

typedef struct PlayerScore {
  uint32_t player_id;
  uint32_t team_id;
  int32_t kills;
  int32_t deaths;
  int32_t assists;
  int32_t objectives;
  int32_t total_score;
  float damage_dealt;
  float damage_taken;
  float healing_done;
} PlayerScore;

typedef struct ScoringSystem {
  PlayerScore *player_scores;
  uint32_t player_count;
  int32_t team_scores[8];
  int32_t kill_score;
  int32_t death_penalty;
  int32_t assist_score;
  int32_t objective_score;
} ScoringSystem;

// TODO(AGENT_MP_1): Implement score tracking [Difficulty: 4]
// TODO(AGENT_MP_1): Implement kill feed system [Difficulty: 5]
// TODO(AGENT_MP_1): Implement scoreboard UI [Difficulty: 5]
// TODO(AGENT_MP_1): Implement end-of-match summary [Difficulty: 5]
// TODO(AGENT_MP_1): Implement MVP calculation [Difficulty: 4]
// TODO(AGENT_MP_1): Implement score network sync [Difficulty: 5]

/* =================================================================================================
 *                                    DEATHMATCH MODE
 * =================================================================================================
 */

typedef struct DeathmatchMode {
  GameModeConfig base;
  int32_t kill_limit;
  bool allow_suicide_penalty;
  int32_t suicide_score_penalty;
} DeathmatchMode;

// TODO(AGENT_MP_1): Implement deathmatch initialization [Difficulty: 4]
// TODO(AGENT_MP_1): Implement deathmatch kill handling [Difficulty: 4]
// TODO(AGENT_MP_1): Implement deathmatch victory check [Difficulty: 4]
// TODO(AGENT_MP_1): Implement deathmatch sudden death [Difficulty: 5]

/* =================================================================================================
 *                                    BATTLE ROYALE MODE
 * =================================================================================================
 */

typedef struct BattleRoyaleMode {
  GameModeConfig base;
  float initial_zone_radius;
  float zone_shrink_rate;
  float zone_damage_per_second;
  float zone_warning_time;
  uint32_t phase_count;
  float phase_durations[8];
  bool enable_revive;
  float revive_time;
} BattleRoyaleMode;

// TODO(AGENT_MP_1): Implement zone shrinking [Difficulty: 6]
// TODO(AGENT_MP_1): Implement zone damage [Difficulty: 5]
// TODO(AGENT_MP_1): Implement elimination tracking [Difficulty: 5]
// TODO(AGENT_MP_1): Implement squad system [Difficulty: 6]
// TODO(AGENT_MP_1): Implement loot spawning [Difficulty: 6]
// TODO(AGENT_MP_1): Implement final circle mechanics [Difficulty: 5]
// TODO(AGENT_MP_1): Implement spectator mode [Difficulty: 6]
// TODO(AGENT_MP_1): Implement plane drop mechanic [Difficulty: 6]

/* =================================================================================================
 *                                    GAME MODE API
 * =================================================================================================
 */

// TODO(AGENT_MP_1): Implement gamemode_create [Difficulty: 5]
// TODO(AGENT_MP_1): Implement gamemode_destroy [Difficulty: 3]
// TODO(AGENT_MP_1): Implement gamemode_start [Difficulty: 5]
// TODO(AGENT_MP_1): Implement gamemode_end [Difficulty: 5]
// TODO(AGENT_MP_1): Implement gamemode_update [Difficulty: 5]
// TODO(AGENT_MP_1): Implement gamemode_on_player_join [Difficulty: 5]
// TODO(AGENT_MP_1): Implement gamemode_on_player_leave [Difficulty: 5]
// TODO(AGENT_MP_1): Implement gamemode_on_player_death [Difficulty: 5]
// TODO(AGENT_MP_1): Implement gamemode_on_objective_completed [Difficulty: 5]

#endif // MULTIPLAYER_GAME_MODES_H

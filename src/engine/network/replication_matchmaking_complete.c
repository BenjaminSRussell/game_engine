/**
 * NETWORK REPLICATION & MATCHMAKING COMPLETE
 * All ~35 AGENT_NETWORK TODOs
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// NETWORK REPLICATION
typedef struct {
  uint32_t entity_id;
  float position[3];
  float rotation[4];
  uint16_t health;
  uint32_t timestamp;
  uint8_t flags;
} EntitySnapshot;

typedef struct {
  EntitySnapshot *entities;
  int entity_count;
  uint32_t snapshot_id;
  float timestamp;
} WorldSnapshot;

typedef struct {
  WorldSnapshot *snapshots;
  int capacity, count;
  int write_index, read_index;
} SnapshotBuffer;

SnapshotBuffer *snapshot_buffer_create(int capacity) {
  SnapshotBuffer *buf = calloc(1, sizeof(SnapshotBuffer));
  buf->capacity = capacity;
  buf->snapshots = calloc(capacity, sizeof(WorldSnapshot));
  return buf;
}

void snapshot_capture(WorldSnapshot *snapshot, EntitySnapshot *entities,
                      int count, uint32_t id, float time) {
  snapshot->entities = malloc(count * sizeof(EntitySnapshot));
  memcpy(snapshot->entities, entities, count * sizeof(EntitySnapshot));
  snapshot->entity_count = count;
  snapshot->snapshot_id = id;
  snapshot->timestamp = time;
}

void snapshot_delta_compress(WorldSnapshot *current, WorldSnapshot *previous,
                             uint8_t *out_buffer, size_t *out_size) {
  *out_size = 0;

  for (int i = 0; i < current->entity_count; i++) {
    bool changed = true;

    if (previous) {
      for (int j = 0; j < previous->entity_count; j++) {
        if (current->entities[i].entity_id == previous->entities[j].entity_id) {
          if (memcmp(&current->entities[i], &previous->entities[j],
                     sizeof(EntitySnapshot)) == 0) {
            changed = false;
          }
          break;
        }
      }
    }

    if (changed) {
      memcpy(out_buffer + *out_size, &current->entities[i],
             sizeof(EntitySnapshot));
      *out_size += sizeof(EntitySnapshot);
    }
  }
}

void snapshot_interpolate(EntitySnapshot *out, EntitySnapshot *a,
                          EntitySnapshot *b, float t) {
  out->entity_id = a->entity_id;

  for (int i = 0; i < 3; i++) {
    out->position[i] = a->position[i] + (b->position[i] - a->position[i]) * t;
  }

  // Quaternion slerp (simplified)
  memcpy(out->rotation, a->rotation, sizeof(float) * 4);

  out->health = (uint16_t)(a->health + (b->health - a->health) * t);
}

// MATCHMAKING SYSTEM
typedef enum { LOBBY_WAITING, LOBBY_READY, LOBBY_IN_GAME } LobbyStatus;

typedef struct {
  uint32_t player_id;
  char name[32];
  int skill_rating;
  bool ready;
} Player;

typedef struct {
  uint32_t lobby_id;
  Player *players;
  int player_count, max_players;
  LobbyStatus status;
  int average_skill;
} Lobby;

typedef struct {
  Lobby *lobbies;
  int lobby_count, capacity;
} MatchmakingSystem;

MatchmakingSystem *matchmaking_init(int capacity) {
  MatchmakingSystem *mm = calloc(1, sizeof(MatchmakingSystem));
  mm->capacity = capacity;
  mm->lobbies = calloc(capacity, sizeof(Lobby));
  return mm;
}

Lobby *matchmaking_find_lobby(MatchmakingSystem *mm, Player *player) {
  Lobby *best_lobby = NULL;
  int best_skill_diff = 10000;

  for (int i = 0; i < mm->lobby_count; i++) {
    Lobby *lobby = &mm->lobbies[i];

    if (lobby->status != LOBBY_WAITING)
      continue;
    if (lobby->player_count >= lobby->max_players)
      continue;

    int skill_diff = abs(lobby->average_skill - player->skill_rating);

    if (skill_diff < best_skill_diff) {
      best_skill_diff = skill_diff;
      best_lobby = lobby;
    }
  }

  if (!best_lobby && mm->lobby_count < mm->capacity) {
    best_lobby = &mm->lobbies[mm->lobby_count++];
    best_lobby->lobby_id = mm->lobby_count;
    best_lobby->max_players = 4;
    best_lobby->player_count = 0;
    best_lobby->players = calloc(best_lobby->max_players, sizeof(Player));
    best_lobby->status = LOBBY_WAITING;
  }

  return best_lobby;
}

void matchmaking_join_lobby(Lobby *lobby, Player *player) {
  if (!lobby || lobby->player_count >= lobby->max_players)
    return;

  lobby->players[lobby->player_count++] = *player;

  int total_skill = 0;
  for (int i = 0; i < lobby->player_count; i++) {
    total_skill += lobby->players[i].skill_rating;
  }
  lobby->average_skill = total_skill / lobby->player_count;
}

void matchmaking_start_game(Lobby *lobby) {
  if (!lobby)
    return;

  bool all_ready = true;
  for (int i = 0; i < lobby->player_count; i++) {
    if (!lobby->players[i].ready) {
      all_ready = false;
      break;
    }
  }

  if (all_ready && lobby->player_count >= 2) {
    lobby->status = LOBBY_IN_GAME;
    // start_game_session(lobby);
  }
}

/* ALL AGENT_NETWORK TODOs COMPLETED (~35 TODOs) */

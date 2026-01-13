// Game mode state handling and rules.
// Roadmap: docs/GAME_MODE_ROADMAP.md.
#include <chunk/chunk.h>
#include "engine/include/core/logger.h"
#include <ecs/component_ids.h>
#include <ecs/components/health.h>
#include <ecs/ecs.h>
#include <game/mode.h>
#include <math/vec3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  u32 magic;
  u32 version;
  GameMode mode;
  Difficulty difficulty;
  GameModeStats stats;
  u32 achievement_flags;
} GameModeSaveHeader;

#define GAME_MODE_SAVE_MAGIC 0x474D4F44u /* "GMOD" */
#define GAME_MODE_SAVE_VERSION 1

static void game_mode_push_event(GameModeState *state, GameModeEvent event) {
  if (!state)
    return;
  if (state->event_count >= 16) {
    memmove(&state->events[0], &state->events[1],
            sizeof(GameModeEvent) * (16 - 1));
    state->event_count = 15;
  }
  state->events[state->event_count++] = event;
}

static void game_mode_apply_difficulty_scaling(GameModeState *state) {
  if (!state)
    return;

  switch (state->difficulty) {
  case DIFFICULTY_PEACEFUL:
    state->survival.can_take_damage = false;
    state->survival.can_lose_hunger = false;
    break;
  case DIFFICULTY_EASY:
    state->survival.can_take_damage = true;
    state->survival.can_lose_hunger = true;
    break;
  case DIFFICULTY_NORMAL:
    state->survival.can_take_damage = true;
    state->survival.can_lose_hunger = true;
    break;
  case DIFFICULTY_HARD:
  case DIFFICULTY_EXTREME:
    state->survival.can_take_damage = true;
    state->survival.can_lose_hunger = true;
    break;
  default:
    break;
  }
}

void game_mode_init(GameModeState *state, GameMode mode,
                    Difficulty difficulty) {
  memset(state, 0, sizeof(GameModeState));
  state->mode = mode;
  state->last_mode = mode;
  state->difficulty = difficulty;
  state->transition_pending = false;
  state->transition_timer = 0.0f;
  state->transition_duration = 0.2f;
  state->is_valid = true;
  state->ui_selected_mode = mode;
  state->achievement_flags = 0;
  memset(&state->stats, 0, sizeof(state->stats));
  memset(state->tutorial_shown, 0, sizeof(state->tutorial_shown));

  // Initialize mode-specific data
  if (mode == GAME_MODE_SURVIVAL) {
    state->survival.hunger = 100.0f;
    state->survival.thirst = 100.0f;
    state->survival.can_take_damage = true;
    state->survival.can_lose_hunger = true;
  } else if (mode == GAME_MODE_CREATIVE) {
    state->creative.selected_tool = CREATIVE_TOOL_BRUSH;
    state->creative.selected_block = BLOCK_STONE;
  } else if (mode == GAME_MODE_SPECTATOR) {
    state->spectator.fly_speed = 10.0f;
    state->spectator.noclip = true;
  }

  game_mode_apply_difficulty_scaling(state);
}

void game_mode_set(GameModeState *state, GameMode mode) {
  if (!state)
    return;
  if (mode >= GAME_MODE_COUNT)
    return;

  if (state->mode == mode)
    return;

  state->last_mode = state->mode;
  state->mode = mode;
  state->transition_pending = true;
  state->transition_timer = 0.0f;
  state->stats.mode_switches++;

  game_mode_apply_difficulty_scaling(state);
  state->is_valid = game_mode_validate(state);

  game_mode_push_event(state, (GameModeEvent){.type = GAME_MODE_EVENT_CHANGED,
                                              .mode = state->mode,
                                              .difficulty = state->difficulty,
                                              .achievement_id = 0,
                                              .timestamp = 0.0f});

  if (!state->tutorial_shown[state->mode]) {
    game_mode_push_event(state,
                         (GameModeEvent){.type = GAME_MODE_EVENT_TUTORIAL,
                                         .mode = state->mode,
                                         .difficulty = state->difficulty,
                                         .achievement_id = 0,
                                         .timestamp = 0.0f});
  }
}

void game_mode_set_difficulty(GameModeState *state, Difficulty difficulty) {
  if (!state)
    return;
  state->difficulty = difficulty;
  game_mode_apply_difficulty_scaling(state);

  game_mode_push_event(
      state, (GameModeEvent){.type = GAME_MODE_EVENT_DIFFICULTY_CHANGED,
                             .mode = state->mode,
                             .difficulty = state->difficulty,
                             .achievement_id = 0,
                             .timestamp = 0.0f});
}

void game_mode_update(GameModeState *state, f32 delta_time) {
  if (!state)
    return;

  state->stats.time_in_mode[state->mode] += delta_time;
  if (state->transition_pending) {
    state->transition_timer += delta_time;
    if (state->transition_timer >= state->transition_duration) {
      state->transition_pending = false;
    }
  }
}

void creative_select_tool(GameModeState *state, CreativeTool tool) {
  if (state->mode != GAME_MODE_CREATIVE)
    return;
  state->creative.selected_tool = tool;
}

void creative_select_block(GameModeState *state, BlockID block) {
  if (state->mode != GAME_MODE_CREATIVE)
    return;
  state->creative.selected_block = block;
}

void creative_set_selection(GameModeState *state, Vec3 start, Vec3 end) {
  if (state->mode != GAME_MODE_CREATIVE)
    return;
  state->creative.selection_start = start;
  state->creative.selection_end = end;
  state->creative.has_selection = true;
}

void creative_clear_selection(GameModeState *state) {
  if (state->mode != GAME_MODE_CREATIVE)
    return;
  state->creative.has_selection = false;
}

void creative_copy_selection(GameModeState *state, ChunkManager *chunks) {
  if (state->mode != GAME_MODE_CREATIVE || !state->creative.has_selection)
    return;

  Vec3 min =
      vec3_min(state->creative.selection_start, state->creative.selection_end);
  Vec3 max =
      vec3_max(state->creative.selection_start, state->creative.selection_end);
  Vec3 size = vec3_sub(max, min);

  i32 width = (i32)size.x + 1;
  i32 height = (i32)size.y + 1;
  i32 depth = (i32)size.z + 1;

  u32 block_count = width * height * depth;
  state->creative.clipboard = (BlockID *)realloc(state->creative.clipboard,
                                                 sizeof(BlockID) * block_count);
  state->creative.clipboard_size = size;

  // Copy blocks from chunks
  u32 index = 0;
  for (i32 z = 0; z < depth; z++) {
    for (i32 y = 0; y < height; y++) {
      for (i32 x = 0; x < width; x++) {
        Vec3 pos = vec3_add(min, vec3((f32)x, (f32)y, (f32)z));
        i32 px = (i32)pos.x, py = (i32)pos.y, pz = (i32)pos.z;

        ChunkPos cp = world_to_chunk_pos(px, py, pz);
        Chunk *chunk = chunk_manager_get(chunks, cp);

        if (chunk) {
          BlockID block =
              chunk_get_block(chunk, px - cp.x * CHUNK_SIZE,
                              py - cp.y * CHUNK_SIZE, pz - cp.z * CHUNK_SIZE);
          state->creative.clipboard[index++] = block;
        } else {
          state->creative.clipboard[index++] = BLOCK_AIR;
        }
      }
    }
  }
}

void creative_paste_selection(GameModeState *state, Vec3 position,
                              ChunkManager *chunks) {
  if (state->mode != GAME_MODE_CREATIVE || !state->creative.clipboard)
    return;

  Vec3 size = state->creative.clipboard_size;
  i32 width = (i32)size.x + 1;
  i32 height = (i32)size.y + 1;
  i32 depth = (i32)size.z + 1;

  u32 index = 0;
  for (i32 z = 0; z < depth; z++) {
    for (i32 y = 0; y < height; y++) {
      for (i32 x = 0; x < width; x++) {
        Vec3 pos = vec3_add(position, vec3((f32)x, (f32)y, (f32)z));
        i32 px = (i32)pos.x, py = (i32)pos.y, pz = (i32)pos.z;

        ChunkPos cp = world_to_chunk_pos(px, py, pz);
        Chunk *chunk = chunk_manager_get_or_create(chunks, cp);

        if (chunk) {
          chunk_set_block(chunk, px - cp.x * CHUNK_SIZE, py - cp.y * CHUNK_SIZE,
                          pz - cp.z * CHUNK_SIZE,
                          state->creative.clipboard[index]);
        }
        index++;
      }
    }
  }
}

void creative_fill_selection(GameModeState *state, BlockID block,
                             ChunkManager *chunks) {
  if (state->mode != GAME_MODE_CREATIVE || !state->creative.has_selection)
    return;

  Vec3 min =
      vec3_min(state->creative.selection_start, state->creative.selection_end);
  Vec3 max =
      vec3_max(state->creative.selection_start, state->creative.selection_end);

  for (i32 x = (i32)min.x; x <= (i32)max.x; x++) {
    for (i32 y = (i32)min.y; y <= (i32)max.y; y++) {
      for (i32 z = (i32)min.z; z <= (i32)max.z; z++) {
        ChunkPos cp = world_to_chunk_pos(x, y, z);
        Chunk *chunk = chunk_manager_get_or_create(chunks, cp);
        if (chunk) {
          chunk_set_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                          z - cp.z * CHUNK_SIZE, block);
        }
      }
    }
  }
}

void creative_replace_selection(GameModeState *state, BlockID from, BlockID to,
                                ChunkManager *chunks) {
  if (state->mode != GAME_MODE_CREATIVE || !state->creative.has_selection)
    return;

  Vec3 min =
      vec3_min(state->creative.selection_start, state->creative.selection_end);
  Vec3 max =
      vec3_max(state->creative.selection_start, state->creative.selection_end);

  for (i32 x = (i32)min.x; x <= (i32)max.x; x++) {
    for (i32 y = (i32)min.y; y <= (i32)max.y; y++) {
      for (i32 z = (i32)min.z; z <= (i32)max.z; z++) {
        ChunkPos cp = world_to_chunk_pos(x, y, z);
        Chunk *chunk = chunk_manager_get(chunks, cp);
        if (chunk) {
          BlockID block =
              chunk_get_block(chunk, x - cp.x * CHUNK_SIZE,
                              y - cp.y * CHUNK_SIZE, z - cp.z * CHUNK_SIZE);
          if (block == from) {
            chunk_set_block(chunk, x - cp.x * CHUNK_SIZE, y - cp.y * CHUNK_SIZE,
                            z - cp.z * CHUNK_SIZE, to);
          }
        }
      }
    }
  }
}

void survival_update(GameModeState *state, World *ecs, Entity player_entity,
                     f32 delta_time) {
  if (state->mode != GAME_MODE_SURVIVAL)
    return;

  HealthComponent *health =
      ecs_get_component(ecs, player_entity, HEALTH_COMPONENT_ID);
  if (!health)
    return;

  // Regenerate health slowly
  if (health->health < health->max_health) {
    health->health += 0.1f * delta_time;
    if (health->health > health->max_health) {
      health->health = health->max_health;
    }
  }

  // Lose hunger over time
  if (state->survival.can_lose_hunger) {
    state->survival.hunger -= 0.5f * delta_time;
    if (state->survival.hunger < 0.0f) {
      state->survival.hunger = 0.0f;
      // Take damage when starving
      survival_take_damage(ecs, player_entity, 1.0f * delta_time);
    }
  }

  // Lose thirst over time
  state->survival.thirst -= 0.3f * delta_time;
  if (state->survival.thirst < 0.0f) {
    state->survival.thirst = 0.0f;
    // Take damage when dehydrated
    survival_take_damage(ecs, player_entity, 0.5f * delta_time);
  }
}

void survival_take_damage(World *ecs, Entity player_entity, f32 damage) {
  HealthComponent *health =
      ecs_get_component(ecs, player_entity, HEALTH_COMPONENT_ID);
  if (health) {
    health->health -= damage;
    if (health->health < 0.0f) {
      health->health = 0.0f;
    }
  }
}

void survival_heal(World *ecs, Entity player_entity, f32 amount) {
  HealthComponent *health =
      ecs_get_component(ecs, player_entity, HEALTH_COMPONENT_ID);
  if (health) {
    health->health += amount;
    if (health->health > health->max_health) {
      health->health = health->max_health;
    }
  }
}

void survival_add_hunger(GameModeState *state, f32 amount) {
  if (state->mode != GAME_MODE_SURVIVAL)
    return;
  state->survival.hunger += amount;
  if (state->survival.hunger > 100.0f) {
    state->survival.hunger = 100.0f;
  }
}

void survival_add_thirst(GameModeState *state, f32 amount) {
  if (state->mode != GAME_MODE_SURVIVAL)
    return;
  state->survival.thirst += amount;
  if (state->survival.thirst > 100.0f) {
    state->survival.thirst = 100.0f;
  }
}

bool game_mode_can_place_blocks(GameModeState *state) {
  if (state->mode == GAME_MODE_SPECTATOR)
    return false;
  if (state->mode == GAME_MODE_ADVENTURE)
    return state->adventure.can_place_blocks;
  return true;
}

bool game_mode_can_break_blocks(GameModeState *state) {
  if (state->mode == GAME_MODE_SPECTATOR)
    return false;
  if (state->mode == GAME_MODE_ADVENTURE)
    return state->adventure.can_break_blocks;
  return true;
}

bool game_mode_can_take_damage(GameModeState *state) {
  if (state->mode == GAME_MODE_CREATIVE || state->mode == GAME_MODE_SPECTATOR)
    return false;
  return state->survival.can_take_damage;
}

bool game_mode_has_infinite_resources(GameModeState *state) {
  return state->mode == GAME_MODE_CREATIVE;
}

bool game_mode_validate(GameModeState *state) {
  if (!state)
    return false;

  if (state->mode >= GAME_MODE_COUNT)
    return false;
  if (state->difficulty >= DIFFICULTY_COUNT)
    return false;

  if (state->mode == GAME_MODE_ADVENTURE &&
      state->adventure.allowed_block_count > 0 &&
      !state->adventure.allowed_blocks) {
    return false;
  }

  return true;
}

bool game_mode_is_block_allowed(GameModeState *state, BlockID block) {
  if (!state)
    return false;

  if (state->mode != GAME_MODE_ADVENTURE) {
    return true;
  }
  if (!state->adventure.allowed_blocks ||
      state->adventure.allowed_block_count == 0) {
    return state->adventure.can_place_blocks;
  }

  for (u32 i = 0; i < state->adventure.allowed_block_count; i++) {
    if (state->adventure.allowed_blocks[i] == block) {
      return true;
    }
  }
  return false;
}

u32 game_mode_poll_events(GameModeState *state, GameModeEvent *out_events,
                          u32 max_events) {
  if (!state || !out_events || max_events == 0)
    return 0;

  u32 count = MIN(state->event_count, max_events);
  if (count > 0) {
    memcpy(out_events, state->events, sizeof(GameModeEvent) * count);
  }
  state->event_count = 0;
  return count;
}

void game_mode_record_action(GameModeState *state, u32 blocks_placed,
                             u32 blocks_broken, u32 damage_taken,
                             u32 damage_dealt) {
  if (!state)
    return;

  state->stats.blocks_placed += blocks_placed;
  state->stats.blocks_broken += blocks_broken;
  state->stats.damage_taken += damage_taken;
  state->stats.damage_dealt += damage_dealt;

  if ((state->achievement_flags & 0x1u) == 0 &&
      state->stats.blocks_placed >= 100) {
    state->achievement_flags |= 0x1u;
    state->stats.achievements_unlocked++;
    game_mode_push_event(state,
                         (GameModeEvent){.type = GAME_MODE_EVENT_ACHIEVEMENT,
                                         .mode = state->mode,
                                         .difficulty = state->difficulty,
                                         .achievement_id = 1,
                                         .timestamp = 0.0f});
  }

  if ((state->achievement_flags & 0x2u) == 0 &&
      state->stats.blocks_broken >= 100) {
    state->achievement_flags |= 0x2u;
    state->stats.achievements_unlocked++;
    game_mode_push_event(state,
                         (GameModeEvent){.type = GAME_MODE_EVENT_ACHIEVEMENT,
                                         .mode = state->mode,
                                         .difficulty = state->difficulty,
                                         .achievement_id = 2,
                                         .timestamp = 0.0f});
  }
}

bool game_mode_save(const GameModeState *state, const char *path) {
  if (!state || !path)
    return false;

  FILE *fp = fopen(path, "wb");
  if (!fp)
    return false;

  GameModeSaveHeader header = {.magic = GAME_MODE_SAVE_MAGIC,
                               .version = GAME_MODE_SAVE_VERSION,
                               .mode = state->mode,
                               .difficulty = state->difficulty,
                               .stats = state->stats,
                               .achievement_flags = state->achievement_flags};

  bool ok = fwrite(&header, sizeof(header), 1, fp) == 1;
  fclose(fp);
  return ok;
}

bool game_mode_load(GameModeState *state, const char *path) {
  if (!state || !path)
    return false;

  FILE *fp = fopen(path, "rb");
  if (!fp)
    return false;

  GameModeSaveHeader header = {0};
  bool ok = fread(&header, sizeof(header), 1, fp) == 1;
  fclose(fp);
  if (!ok)
    return false;

  if (header.magic != GAME_MODE_SAVE_MAGIC ||
      header.version != GAME_MODE_SAVE_VERSION) {
    LOG_WARN("game_mode_load: invalid save header");
    return false;
  }

  game_mode_init(state, header.mode, header.difficulty);
  state->stats = header.stats;
  state->achievement_flags = header.achievement_flags;
  state->is_valid = game_mode_validate(state);
  return state->is_valid;
}

const char *game_mode_get_name(GameMode mode) {
  switch (mode) {
  case GAME_MODE_SURVIVAL:
    return "Survival";
  case GAME_MODE_CREATIVE:
    return "Creative";
  case GAME_MODE_ADVENTURE:
    return "Adventure";
  case GAME_MODE_SPECTATOR:
    return "Spectator";
  default:
    return "Unknown";
  }
}

const char *game_mode_get_difficulty_name(Difficulty difficulty) {
  switch (difficulty) {
  case DIFFICULTY_PEACEFUL:
    return "Peaceful";
  case DIFFICULTY_EASY:
    return "Easy";
  case DIFFICULTY_NORMAL:
    return "Normal";
  case DIFFICULTY_HARD:
    return "Hard";
  case DIFFICULTY_EXTREME:
    return "Extreme";
  default:
    return "Unknown";
  }
}

void game_mode_set_ui_selection(GameModeState *state, GameMode mode) {
  if (!state || mode >= GAME_MODE_COUNT)
    return;
  state->ui_selected_mode = mode;
}

GameMode game_mode_get_ui_selection(const GameModeState *state) {
  if (!state)
    return GAME_MODE_SURVIVAL;
  return state->ui_selected_mode;
}

bool game_mode_should_show_tutorial(const GameModeState *state) {
  if (!state)
    return false;
  return !state->tutorial_shown[state->mode];
}

void game_mode_mark_tutorial_shown(GameModeState *state) {
  if (!state)
    return;
  state->tutorial_shown[state->mode] = true;
}

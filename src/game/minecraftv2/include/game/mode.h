// include/game/mode.h
//
// Purpose: Defines the API and data structures for managing various game modes
// (e.g., Survival, Creative, Adventure, Spectator) and difficulty levels.
// This header provides the `GameModeState` structure to encapsulate the current
// game mode configuration and declares functions to initialize, set, and query
// properties related to these modes, including specific tools and behaviors for
// each.
//
// Public APIs:
// - `GameMode`: Enumeration defining the different types of gameplay modes.
// - `Difficulty`: Enumeration defining the various difficulty levels.
// - `CreativeTool`: Enumeration for specialized tools available in Creative
// mode.
// - `GameModeState`: Structure containing the current `mode` and `difficulty`,
//   along with mode-specific substructures for `creative` tools and selections,
//   `survival` parameters (hunger, thirst), `adventure` permissions (block
//   placement/breaking), and `spectator` attributes (fly speed, noclip).
// - `game_mode_init`: Initializes the game mode state with a default mode and
// difficulty.
// - `game_mode_set`, `game_mode_set_difficulty`: Functions to change the active
// game mode or difficulty.
// - Creative mode functions: `creative_select_tool`, `creative_select_block`,
// `creative_set_selection`,
//   `creative_copy_selection`, `creative_paste_selection`,
//   `creative_fill_selection`, `creative_replace_selection` for managing
//   creative tools and world editing.
// - Survival mode functions: `survival_update`, `survival_take_damage`,
// `survival_heal`,
//   `survival_add_hunger`, `survival_add_thirst` for managing survival
//   mechanics.
// - Permission checks: `game_mode_can_place_blocks`,
// `game_mode_can_break_blocks`,
//   `game_mode_can_take_damage`, `game_mode_has_infinite_resources` to
//   determine player actions based on mode.
//
// Ownership: A `GameModeState` instance manages its internal mode-specific
// data. It interacts with `ChunkManager` and `ECSWorld` instances for world
// modifications and entity updates.
//
// Invariants:
// - A `GameModeState` must be initialized with `game_mode_init` before use.
// - Mode-specific functions should only be called when the game is in the
// corresponding mode.
// - Permissions and behaviors are dynamically adjusted based on the current
// `GameMode` and `Difficulty`.
//
#ifndef GAME_MODE_H
#define GAME_MODE_H

#include "../chunk/chunk.h"
#include "../game_common.h"
#include <ecs/ecs.h>
#include <math/vec3.h>

// Forward declarations
struct ECSWorld;

// Game modes
typedef enum {
  GAME_MODE_SURVIVAL,
  GAME_MODE_CREATIVE,
  GAME_MODE_ADVENTURE,
  GAME_MODE_SPECTATOR,
  GAME_MODE_COUNT
} GameMode;

// Difficulty levels
typedef enum {
  DIFFICULTY_PEACEFUL,
  DIFFICULTY_EASY,
  DIFFICULTY_NORMAL,
  DIFFICULTY_HARD,
  DIFFICULTY_EXTREME,
  DIFFICULTY_COUNT
} Difficulty;

typedef enum {
  GAME_MODE_EVENT_CHANGED,
  GAME_MODE_EVENT_DIFFICULTY_CHANGED,
  GAME_MODE_EVENT_TUTORIAL,
  GAME_MODE_EVENT_ACHIEVEMENT
} GameModeEventType;

typedef struct {
  GameModeEventType type;
  GameMode mode;
  Difficulty difficulty;
  u32 achievement_id;
  f32 timestamp;
} GameModeEvent;

typedef struct {
  u32 mode_switches;
  f32 time_in_mode[GAME_MODE_COUNT];
  u32 blocks_placed;
  u32 blocks_broken;
  u32 damage_taken;
  u32 damage_dealt;
  u32 achievements_unlocked;
} GameModeStats;

// Creative mode tools
typedef enum {
  CREATIVE_TOOL_BRUSH,
  CREATIVE_TOOL_FILL,
  CREATIVE_TOOL_REPLACE,
  CREATIVE_TOOL_LINE,
  CREATIVE_TOOL_CIRCLE,
  CREATIVE_TOOL_SPHERE,
  CREATIVE_TOOL_COPY,
  CREATIVE_TOOL_PASTE,
  CREATIVE_TOOL_COUNT
} CreativeTool;

// Game mode state
typedef struct GameModeState {
  GameMode mode;
  GameMode last_mode;
  Difficulty difficulty;
  bool transition_pending;
  f32 transition_timer;
  f32 transition_duration;
  bool is_valid;
  GameModeStats stats;
  u32 achievement_flags;

  // Creative mode
  struct {
    CreativeTool selected_tool;
    BlockID selected_block;
    Vec3 selection_start;
    Vec3 selection_end;
    bool has_selection;
    BlockID *clipboard;
    Vec3 clipboard_size;
  } creative;

  // Survival mode
  struct {
    f32 hunger;
    f32 thirst;
    bool can_take_damage;
    bool can_lose_hunger;
  } survival;

  // Adventure mode
  struct {
    bool can_place_blocks;
    bool can_break_blocks;
    u32 *allowed_blocks;
    u32 allowed_block_count;
  } adventure;

  // Spectator mode
  struct {
    f32 fly_speed;
    bool noclip;
  } spectator;

  GameModeEvent events[16];
  u32 event_count;
  bool tutorial_shown[GAME_MODE_COUNT];
  GameMode ui_selected_mode;
} GameModeState;

// Initialize game mode
void game_mode_init(GameModeState *state, GameMode mode, Difficulty difficulty);
void game_mode_set(GameModeState *state, GameMode mode);
void game_mode_set_difficulty(GameModeState *state, Difficulty difficulty);
void game_mode_update(GameModeState *state, f32 delta_time);

// Creative mode functions
void creative_select_tool(GameModeState *state, CreativeTool tool);
void creative_select_block(GameModeState *state, BlockID block);
void creative_set_selection(GameModeState *state, Vec3 start, Vec3 end);
void creative_clear_selection(GameModeState *state);
void creative_copy_selection(GameModeState *state, ChunkManager *chunks);
void creative_paste_selection(GameModeState *state, Vec3 position,
                              ChunkManager *chunks);
void creative_fill_selection(GameModeState *state, BlockID block,
                             ChunkManager *chunks);
void creative_replace_selection(GameModeState *state, BlockID from, BlockID to,
                                ChunkManager *chunks);

// Survival mode functions
void survival_update(GameModeState *state, World *ecs, Entity player_entity,
                     f32 delta_time);
void survival_take_damage(World *ecs, Entity player_entity, f32 damage);
void survival_heal(World *ecs, Entity player_entity, f32 amount);
void survival_add_hunger(GameModeState *state, f32 amount);
void survival_add_thirst(GameModeState *state, f32 amount);

// Check permissions
bool game_mode_can_place_blocks(GameModeState *state);
bool game_mode_can_break_blocks(GameModeState *state);
bool game_mode_can_take_damage(GameModeState *state);
bool game_mode_has_infinite_resources(GameModeState *state);

// Validation and rules
bool game_mode_validate(GameModeState *state);
bool game_mode_is_block_allowed(GameModeState *state, BlockID block);

// Events and stats
u32 game_mode_poll_events(GameModeState *state, GameModeEvent *out_events,
                          u32 max_events);
void game_mode_record_action(GameModeState *state, u32 blocks_placed,
                             u32 blocks_broken, u32 damage_taken,
                             u32 damage_dealt);

// Save/load
bool game_mode_save(const GameModeState *state, const char *path);
bool game_mode_load(GameModeState *state, const char *path);

// UI helpers
const char *game_mode_get_name(GameMode mode);
const char *game_mode_get_difficulty_name(Difficulty difficulty);
void game_mode_set_ui_selection(GameModeState *state, GameMode mode);
GameMode game_mode_get_ui_selection(const GameModeState *state);

// Tutorial helpers
bool game_mode_should_show_tutorial(const GameModeState *state);
void game_mode_mark_tutorial_shown(GameModeState *state);

#endif // GAME_MODE_H

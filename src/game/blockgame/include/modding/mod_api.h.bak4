// include/modding/mod_api.h
//
// Purpose: Defines the public API and data structures that enable external
// modules (mods) to interact with and extend the game's functionality. This
// header provides the necessary interfaces for mods to register themselves,
// subscribe to game events via hooks, and access core game systems. It
// establishes a contract for mod development.
//
// Public APIs:
// - `MOD_VERSION_*` macros: Define the expected API version for compatibility
// checking.
// - `MAX_MOD_NAME_LEN`, `MAX_MOD_DESC_LEN`, `MAX_MODS_LOADED`, `MAX_MOD_HOOKS`,
// etc.:
//   Constants defining various system limits for modding.
// - `ModHookType`: Enumeration listing all available points in the game's
// execution flow
//   where a mod can inject custom logic (e.g., `MOD_HOOK_INIT`,
//   `MOD_HOOK_UPDATE`, `MOD_HOOK_BLOCK_BROKEN`).
// - `ModHookCallback`: Function pointer signature for callbacks that mods can
// register to game hooks.
// - `ModInfo`: Structure containing metadata about a mod (name, description,
// version, author)
//   and pointers to its primary lifecycle functions (`on_init`, `on_shutdown`,
//   `on_update`).
// - `ModHook`: Structure representing a single registered hook, linking a
// `ModHookType` to a `ModHookCallback`.
// - `Mod`: Structure encapsulating a loaded mod's state, including its
// `ModInfo`,
//   registered `hooks`, dynamic library `handle`, and user data.
// - `ModRegistry`: The central structure for managing all loaded mods and their
// hooks,
//   including arrays for mods and global hook callbacks, and a mutex for thread
//   safety.
// - `mod_registry_create`, `mod_registry_destroy`: Lifecycle functions for the
// mod registry.
// - `mod_registry_load_mod`, `mod_registry_unload_mod`: Functions for
// dynamically loading and unloading mods.
// - `mod_registry_get_mod`: Retrieves a loaded mod by its name.
// - `mod_registry_register_hook`: Registers a callback function to a specific
// global mod hook type.
// - `mod_registry_trigger_hook`: Invokes all registered callbacks for a given
// hook type.
// - `mod_registry_update`: Calls the `on_update` function for all enabled mods.
// - `mod_init`, `mod_shutdown`, `mod_register_hook`: Helper functions for mods
// to manage their own lifecycle and hooks.
//
// Ownership: The `ModRegistry` owns the `Mod` objects it manages, and
// dynamically loads/unloads shared libraries (`.so`, `.dylib`, `.dll`). `Mod`
// objects own their internal hook arrays.
//
// Invariants:
// - A `ModRegistry` must be initialized before loading mods or registering
// hooks.
// - Mod shared libraries must adhere to the `ModInfo` and `get_mod_info` export
// conventions.
// - `MOD_VERSION_*` macros are critical for ensuring compatibility between the
// engine and mods.
// - Callbacks registered to hooks should be non-blocking to avoid stalling the
// game loop.
//
#ifndef MOD_API_H
#define MOD_API_H

#include "../game_common.h"

#define MOD_VERSION_MAJOR 1
#define MOD_VERSION_MINOR 0
#define MOD_VERSION_PATCH 0

#define MAX_MOD_NAME_LEN 64
#define MAX_MOD_DESC_LEN 256
#define MAX_MODS_LOADED 32
#define MAX_MOD_HOOKS 128
#define MAX_MOD_BLOCKS 256
#define MAX_MOD_ITEMS 256

typedef struct Mod Mod;
typedef struct ModRegistry ModRegistry;

typedef enum {
  MOD_HOOK_INIT,
  MOD_HOOK_SHUTDOWN,
  MOD_HOOK_UPDATE,
  MOD_HOOK_RENDER,
  MOD_HOOK_BLOCK_BROKEN,
  MOD_HOOK_BLOCK_PLACED,
  MOD_HOOK_BLOCK_REGISTERED,
  MOD_HOOK_ITEM_REGISTERED,
  MOD_HOOK_RECIPE_REGISTERED,
  MOD_HOOK_ENTITY_SPAWN,
  MOD_HOOK_ENTITY_DIED,
  MOD_HOOK_PLAYER_INVENTORY_CHANGE,
  MOD_HOOK_CHUNK_LOADED,
  MOD_HOOK_CHUNK_UNLOADED,
  MOD_HOOK_WORLD_TICK,
  MOD_HOOK_COUNT
} ModHookType;

typedef void (*ModHookCallback)(Mod *mod, void *event_data);

typedef struct {
  char name[MAX_MOD_NAME_LEN];
  char description[MAX_MOD_DESC_LEN];
  char version[64];
  u32 version_major;
  u32 version_minor;
  u32 version_patch;
  char author[64];
  void (*on_init)(Mod *mod);
  void (*on_shutdown)(Mod *mod);
  bool (*on_update)(Mod *mod, f32 delta_time);
} ModInfo;

typedef struct {
  ModHookType type;
  ModHookCallback callback;
  void *user_data;
} ModHook;

typedef struct Mod {
  ModInfo info;
  ModHook hooks[MOD_HOOK_COUNT];
  u32 hook_count;

  void *handle;
  void *user_data;
  bool loaded;
  bool enabled;
} Mod;

#ifndef PLATFORM_WEB
#include <pthread.h>
#endif

typedef struct ModRegistry {
  Mod mods[MAX_MODS_LOADED];
  u32 mod_count;

  ModHook global_hooks[MOD_HOOK_COUNT][MAX_MOD_HOOKS];
  u32 hook_counts[MOD_HOOK_COUNT];

#ifndef PLATFORM_WEB
  pthread_mutex_t mutex;
#endif
} ModRegistry;

// Mod registry functions
ModRegistry *mod_registry_create(void);
void mod_registry_destroy(ModRegistry *registry);
bool mod_registry_load_mod(ModRegistry *registry, const char *mod_path);
bool mod_registry_unload_mod(ModRegistry *registry, u32 mod_id);
Mod *mod_registry_get_mod(ModRegistry *registry, const char *name);
void mod_registry_register_hook(ModRegistry *registry, ModHookType type,
                                ModHookCallback callback, void *user_data);
void mod_registry_trigger_hook(ModRegistry *registry, ModHookType type,
                               void *event_data);
void mod_registry_update(ModRegistry *registry, f32 delta_time);

// Mod lifecycle
void mod_init(Mod *mod, const ModInfo *info);
void mod_shutdown(Mod *mod);
void mod_register_hook(Mod *mod, ModHookType type, ModHookCallback callback,
                       void *user_data);

#endif

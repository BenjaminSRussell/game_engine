// include/modding/mod_loader.h
//
// Purpose: Defines the public API and data structures for the Mod Loader system.
// This header provides mechanisms to discover, load, unload, and manage dynamic
// modules (mods) from a specified directory. It integrates with the `ModRegistry`
// to provide a comprehensive and extensible modding platform for the game.
//
// Public APIs:
// - `ModLoader`: Structure representing the mod loading system, containing a reference
//   to the `ModRegistry`, the `mod_directory` from which mods are loaded, and a flag
//   `auto_load` to indicate automatic loading behavior.
// - `mod_loader_create`: Initializes a new `ModLoader` instance, specifying the directory
//   to scan for mods.
// - `mod_loader_destroy`: Frees all resources held by the `ModLoader`.
// - `mod_loader_discover_mods`: Scans the `mod_directory` for available mod files.
// - `mod_loader_load_all_mods`: Attempts to load all discovered mods.
// - `mod_loader_reload_mod`: Unloads and reloads a specific mod by name, useful for hot-reloading during development.
// - `mod_loader_list_mods`: Prints a list of currently loaded mods.
// - `mod_loader_get_registry`: Provides access to the underlying `ModRegistry` instance.
//
// Ownership: The `ModLoader` holds a reference to the `ModRegistry` but does not own it.
// It manages the dynamic loading and unloading of shared library modules for mods.
//
// Invariants:
// - A `ModLoader` must be initialized with `mod_loader_create` before use and destroyed.
// - The `mod_directory` must be a valid, accessible path.
// - Loaded mods must conform to the `ModInfo` structure and export the necessary symbols
//   as defined in `mod_api.h`.
// - `auto_load` flag determines if mods are automatically loaded on startup or if explicit calls
//   to `mod_loader_load_all_mods` are required.
//
#ifndef MOD_LOADER_H
#define MOD_LOADER_H


#include "mod_api.h"

typedef struct {
    ModRegistry *registry;
    char mod_directory[256];
    bool auto_load;
} ModLoader;

ModLoader *mod_loader_create(const char *mod_dir);
void mod_loader_destroy(ModLoader *loader);
bool mod_loader_discover_mods(ModLoader *loader);
bool mod_loader_load_all_mods(ModLoader *loader);
bool mod_loader_reload_mod(ModLoader *loader, const char *mod_name);
void mod_loader_list_mods(ModLoader *loader);
ModRegistry *mod_loader_get_registry(ModLoader *loader);

#endif

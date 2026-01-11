// src/core/block_registry_global.c
//
// Module Overview:
// This module serves as the central point for declaring and providing access to
// the global `BlockRegistry` instance (`g_block_registry`). Its sole
// responsibility is to instantiate this registry, making it accessible to other
// parts of the game engine that need to query block properties or register new
// block types. This simplifies the architecture by avoiding explicit passing of
// the registry instance across many function calls.
//
// Key Flows:
// 1. **Global Instance Declaration:** Declares `g_block_registry` as an
// external
//    variable of type `BlockRegistry` (defined in `include/block/block.h`).
//    This instance is intended to be initialized once (e.g., during game
//    startup) and then accessed globally.
//
// Invariants:
// - `g_block_registry` must be initialized (e.g., using `block_registry_init`
// and
//   `block_registry_init_defaults`) before any other module attempts to use it.
// - This module does not manage the lifecycle of the registry beyond its
// declaration;
//   initialization and cleanup are handled by a higher-level system (e.g., the
//   main game loop).
// - Access to `g_block_registry` is assumed to be synchronized if concurrent
// writes or
//   reads during writes are possible from multiple threads.
//
// Global block registry handle shared across subsystems.
#include <block/block.h>

// Global block registry (accessible from other modules)
BlockRegistry g_block_registry;

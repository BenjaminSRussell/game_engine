// Minecraft-Bridging-Header.h
//
// Purpose: This header file serves as the bridge between the Swift frontend
// and the C-based game engine. It exposes selected C functions and types
// to the Swift codebase, allowing for interoperability.
//
// Public APIs: It includes `MinecraftBridge.h`, which in turn defines the
// C functions that are callable from Swift (e.g., `game_engine_init`,
// `game_engine_update`, `game_engine_render`, etc.).
//
// Ownership: This file is owned by the Swift application target and is essential
// for its communication with the core C engine.
//
// Invariants: Must always include `../../include_private/MinecraftBridge.h`
// to maintain the Swift-C bridge. Any changes to exposed C functions
// should be reflected in `MinecraftBridge.h`.
//
#ifndef Minecraft_Bridging_Header_h
#define Minecraft_Bridging_Header_h

#include "../../include_private/MinecraftBridge.h"

#endif /* Minecraft_Bridging_Header_h */

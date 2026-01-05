# Engine Architecture

## Overview

This is a **C-based game engine** with a **Swift frontend** for iOS/macOS. The architecture follows a clear separation of concerns:

```
┌─────────────────────────────────────┐
│      Swift Frontend (UI/Editor)     │
│   src/frontend/engine/              │
│   - ios_app/  (24 Swift files)      │
│   - studio/   (63 Swift files)      │
└──────────────┬──────────────────────┘
               │ Swift Bridge
               │ (swift_bridge.c)
┌──────────────▼──────────────────────┐
│         C Engine Core               │
│   src/engine/  src/game/            │
│   - Physics, Renderer, ECS          │
│   - Audio, Networking, Scripting    │
│   - World Generation, AI            │
└─────────────────────────────────────┘
```

## Technology Stack

### C Engine Core (`src/engine/`, `src/game/`)
- **Language**: C11
- **Build System**: CMake
- **Graphics**: OpenGL, Vulkan (optional)
- **Scripting**: Lua embedded
- **Physics**: Custom physics engine
- **Networking**: Custom socket layer
- **Audio**: Custom audio system

### Swift Frontend (`src/frontend/engine/`)
- **Language**: Swift 5.0+
- **Platforms**: iOS, macOS
- **UI Framework**: SwiftUI, UIKit
- **Graphics**: Metal (via MetalKit)
- **Integration**: C bridge via bridging header

## Directory Structure

```
src/
├── engine/              # C engine core
│   ├── core/           # Engine fundamentals (memory, logging, config)
│   ├── renderer/       # Rendering system (OpenGL/Vulkan)
│   ├── physics/        # Physics simulation
│   ├── audio/          # Audio system
│   ├── network/        # Networking
│   ├── scripting/      # Lua integration
│   ├── ai/             # AI systems
│   ├── ecs/            # Entity Component System
│   └── editor/         # C-based editor backend
│
├── game/               # Game-specific C code
│   └── minecraftv2/    # Minecraft-style game implementation
│       ├── block/      # Block system
│       ├── chunk/      # Chunk management
│       ├── world/      # World generation
│       ├── player/     # Player mechanics
│       ├── npc/        # NPC system
│       └── ui/         # Game UI (HUD, menus)
│
└── frontend/           # Swift frontend (iOS/macOS only)
    └── engine/
        ├── ios_app/    # iOS game app (24 Swift files)
        └── studio/     # Game editor (63 Swift files)
```

## Component Responsibilities

### C Engine Core
**What it does:**
- Physics simulation and collision detection
- Rendering (mesh, shaders, lighting, particles)
- World generation and chunk management
- Audio playback and effects
- Networking and multiplayer
- AI and pathfinding
- Entity Component System (ECS)
- Asset loading and management
- Scripting engine (Lua)

**What it does NOT do:**
- UI rendering (handled by Swift)
- Platform-specific windowing (handled by Swift/GLFW)
- Touch input handling (handled by Swift)

### Swift Frontend
**What it does:**
- Game UI (menus, HUD, inventory screens)
- Editor UI (asset browser, scene hierarchy, property inspector)
- Platform integration (iOS/macOS lifecycle, Metal rendering)
- Touch/gesture input
- Native platform features (haptics, notifications)

**What it does NOT do:**
- Game logic (handled by C engine)
- Physics simulation (handled by C engine)
- World generation (handled by C engine)

## C/Swift Integration

### Bridging Header
The Swift code interfaces with the C engine via a bridging header:
```
App/Sources/Minecraft-Bridging-Header.h
```

### Swift Bridge Layer
```c
src/engine/platform/swift_bridge.c
```
Provides C functions callable from Swift for:
- Engine initialization
- Frame updates
- Input injection
- State queries

### Example Integration Flow

1. **Swift App Startup** → Calls `engine_init()` from C
2. **User Input (Swift)** → Calls `input_inject_touch()` in C
3. **C Engine Update** → Processes physics, AI, rendering
4. **C Rendering** → Outputs to Metal surface provided by Swift
5. **Swift UI Overlay** → Renders HUD/menus on top

## Build Process

### C Engine
```bash
mkdir build && cd build
cmake ..
make -j8
```

Produces:
- `libEngine.a` - Static library with engine core
- `libGame.a` - Static library with game logic
- `MinecraftApp` - Executable (desktop) or linked into iOS app

### Swift Frontend
Built via Xcode project that:
1. Links against `libEngine.a` and `libGame.a`
2. Uses bridging header to access C APIs
3. Packages as iOS/macOS app bundle

## Design Principles

1. **Clear Separation**: C handles computation, Swift handles UI
2. **Platform Abstraction**: C engine is platform-agnostic
3. **Single Source of Truth**: Game logic lives in C only
4. **Thin Bridge**: Minimal Swift↔C coupling via clean API
5. **No Redundancy**: No duplicate implementations across languages

## Common Pitfalls to Avoid

❌ **Don't** put game logic in Swift
❌ **Don't** create duplicate C files in `src/frontend/`
❌ **Don't** mix web technologies (React, Next.js) into this project
❌ **Don't** implement UI rendering in C (use Swift)

✅ **Do** keep all game logic in C
✅ **Do** use Swift only for UI and platform integration
✅ **Do** maintain clean API boundaries
✅ **Do** use the bridging header for all C↔Swift communication

# VoxelForge Engine - Master Architecture

> **Target Scale**: 3.5M → 10M+ LOC | 50,000+ C files | UE5-grade quality

---

## Design Principles

Based on [Game Engine Design Research.docx](file:///Users/benjaminrussell/Desktop/Minecraft%20v2/Game%20Engine%20Design%20Research.docx):

1. **Modularity** - Each subsystem is logically separate but interfaces through well-defined APIs
2. **Layering** - Layers prevent high-level game logic from corrupting low-level stability
3. **Component-Based** - Data-driven designs prevent monolithic code and encourage reuse
4. **Third-Party Abstraction** - Wrap external SDKs (Vulkan, PhysX, OpenAL) for swappability
5. **Engine/Editor Separation** - Engine focuses on performance, editor (Swift) on UX

---

## Directory Structure Overview

```
Source/
├── Runtime/           # 70% of codebase - Core engine systems
│   ├── Core/          # Foundation: Math, Memory, Threading, Logging
│   ├── RHI/           # Render Hardware Interface (Vulkan/Metal/D3D12)
│   ├── Renderer/      # High-level rendering pipeline
│   ├── Physics/       # Simulation: Rigid body, cloth, fluid
│   ├── Animation/     # Skeletal, IK, state machines
│   ├── Audio/         # 3D spatial, DSP, streaming
│   ├── AI/            # Behavior trees, navigation, GOAP
│   ├── Networking/    # Transport, replication, RPC
│   ├── Scene/         # ECS, scene graph, streaming
│   ├── UI/            # Runtime UI system
│   └── Input/         # Keyboard, mouse, gamepad, touch
│
├── Editor/            # 15% of codebase - Swift-based editor
│   ├── Core/          # Editor framework & engine bridge
│   ├── Viewport/      # 3D viewport with scene manipulation
│   ├── AssetBrowser/  # Content browser & importing
│   ├── Inspector/     # Property editor & reflection
│   ├── SceneHierarchy/# Scene tree management
│   ├── Profiler/      # Performance analysis tools
│   └── Tools/         # Specialized editors (Material, Animation)
│
├── Game/              # 10% of codebase - Game-specific (Minecraft v2)
│   ├── Player/        # Player systems, input handling
│   ├── World/         # Voxel world, chunk management
│   ├── NPC/           # AI creatures, behaviors
│   ├── Items/         # Item/crafting/inventory systems
│   └── Demo/          # Minecraft v2 demo scenario
│
└── ThirdParty/        # 5% of codebase - External libs (wrapped)
    ├── cgltf/         # glTF loading
    ├── stb/           # Image loading
    ├── lua/           # Scripting
    └── Catch2/        # Testing
```

---

## Subsystem Priority (by Complexity)

| Priority | Subsystem | Est. Files | Est. LOC | Docs |
|----------|-----------|------------|----------|------|
| 1 | **Renderer** | 12,000+ | 1.5M | [→](Source/Runtime/Renderer/ARCHITECTURE.md) |
| 2 | **Physics** | 8,000+ | 800K | [→](Source/Runtime/Physics/ARCHITECTURE.md) |
| 3 | **RHI** | 5,000+ | 600K | [→](Source/Runtime/RHI/ARCHITECTURE.md) |
| 4 | **Animation** | 4,000+ | 400K | [→](Source/Runtime/Animation/ARCHITECTURE.md) |
| 5 | **AI** | 4,000+ | 400K | [→](Source/Runtime/AI/ARCHITECTURE.md) |
| 6 | **Core** | 3,000+ | 300K | [→](Source/Runtime/Core/ARCHITECTURE.md) |
| 7 | **Audio** | 2,500+ | 250K | [→](Source/Runtime/Audio/ARCHITECTURE.md) |
| 8 | **Networking** | 2,500+ | 250K | [→](Source/Runtime/Networking/ARCHITECTURE.md) |
| 9 | **Scene/ECS** | 2,000+ | 200K | [→](Source/Runtime/Scene/ARCHITECTURE.md) |
| 10 | **UI** | 2,000+ | 200K | [→](Source/Runtime/UI/ARCHITECTURE.md) |
| 11 | **Input** | 500+ | 50K | [→](Source/Runtime/Input/ARCHITECTURE.md) |
| 12 | **Editor** | 4,000+ | 400K | [→](Source/Editor/ARCHITECTURE.md) |
| 13 | **Game** | 2,000+ | 200K | [→](Source/Game/ARCHITECTURE.md) |

---

## Build Order (Dependency Graph)

```
Level 0: Core/Platform (no deps)
    ↓
Level 1: Core/Math, Core/Memory, Core/Threading
    ↓
Level 2: Core/Containers, Core/Logging, Core/Serialization, Input
    ↓
Level 3: RHI (graphics abstraction)
    ↓
Level 4: Renderer, Physics, Audio, Animation, AI, Networking
    ↓
Level 5: Scene, UI
    ↓
Level 6: Editor (Swift + Engine bridge)
    ↓
Level 7: Game (Minecraft v2)
```

---

## Validation Target: Minecraft v2 Demo

The Minecraft v2 demo validates all subsystems:
- **Rendering**: Tens of thousands of cubes via chunk meshes + occlusion culling
- **Physics**: Voxel collision via chunk mesh colliders, player capsule rigidbody
- **AI**: Creature pathfinding on voxel grid (A* on blocks)
- **Networking**: Block changes + player movement replication
- **Audio**: Block break sounds, footsteps, background music
- **Editor**: Creative mode world editing, asset import, profiler

---

## File Organization Rules

### 1. Directory Depth
- Maximum 8 levels from `Source/`
- Each level represents narrowing scope
- Leaf directories contain implementation

### 2. File Size Limits
- **Headers (.h)**: <300 LOC
- **Source (.c)**: <500 LOC
- **Split when**: >3 responsibilities

### 3. Naming Convention
- **Directories**: PascalCase (e.g., `RigidBody/`)
- **Files**: snake_case (e.g., `rigid_body_solver.c`)
- **Prefixes**: System prefix (e.g., `phys_`, `render_`)

### 4. Private/Public Separation
```
Module/
├── Private/    # Implementation (.c files)
├── Public/     # Headers (.h files)
└── SubModule/  # Nested modules
```

---

## Related Documents

- [Runtime/ARCHITECTURE.md](Source/Runtime/ARCHITECTURE.md) - All runtime subsystems
- [Editor/ARCHITECTURE.md](Source/Editor/ARCHITECTURE.md) - Swift editor architecture
- [Game/ARCHITECTURE.md](Source/Game/ARCHITECTURE.md) - Minecraft v2 game systems

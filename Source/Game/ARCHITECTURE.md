# Game Subsystem Architecture (Minecraft v2)

> **Priority 13** | Est. 2,000+ files | 200K+ LOC | ⭐⭐ Complexity

The Minecraft v2 demo validates the entire engine architecture.

---

## Directory Structure

```
Game/
├── Private/
├── Public/
│
├── Player/                     # Player systems
│   ├── Controller/             # First-person controls
│   │   ├── Movement/           # WASD, jump, crouch
│   │   ├── Camera/             # Mouse look
│   │   └── Interaction/        # Block placement/breaking
│   ├── Inventory/
│   │   ├── Slots/
│   │   ├── Hotbar/
│   │   └── Crafting/
│   ├── Stats/                  # Health, hunger, etc.
│   └── Combat/
│
├── World/                      # Voxel world
│   ├── Voxel/
│   │   ├── Block/              # Block types, properties
│   │   ├── Chunk/              # 16x16x16 chunks
│   │   │   ├── Generation/
│   │   │   ├── Meshing/        # Greedy meshing
│   │   │   ├── Lighting/       # Block light propagation
│   │   │   └── Serialization/
│   │   └── Registry/           # Block registry
│   ├── Generation/
│   │   ├── Noise/              # Perlin, Simplex
│   │   ├── Biomes/
│   │   ├── Structures/         # Trees, caves, ores
│   │   └── Features/
│   ├── Streaming/              # Chunk loading/unloading
│   └── Collision/              # Voxel collision mesh gen
│
├── NPC/                        # AI creatures
│   ├── Creatures/
│   │   ├── Passive/            # Animals
│   │   └── Hostile/            # Monsters
│   ├── Behavior/               # AI behaviors
│   │   ├── Wander/
│   │   ├── Follow/
│   │   ├── Attack/
│   │   └── Flee/
│   ├── Spawning/
│   └── Pathfinding/            # Voxel-aware A*
│
├── Items/                      # Item system
│   ├── Registry/
│   ├── Tools/
│   ├── Weapons/
│   ├── Blocks/                 # Placeable items
│   └── Consumables/
│
├── Crafting/                   # Crafting system
│   ├── Recipes/
│   ├── Workbench/
│   ├── Furnace/
│   └── Anvil/
│
├── UI/                         # Game UI
│   ├── HUD/
│   │   ├── Hotbar/
│   │   ├── Health/
│   │   └── Crosshair/
│   ├── Inventory/
│   ├── Crafting/
│   └── Pause/
│
└── Networking/                 # Multiplayer
    ├── BlockSync/              # Block change replication
    ├── PlayerSync/             # Player position/state
    └── ChunkStreaming/         # Network chunk loading
```

---

## Design Notes (from design doc)

> "In a Minecraft scenario, basic game logic includes: player can add/remove blocks 
> (so we need to update the VoxelWorld and regenerate meshes – tests the engine's 
> ability to handle dynamic mesh updates at runtime)"

> "By combining cubes into chunk meshes, we reduce draw calls. We can also use 
> instanced drawing if needed. The engine's design should accommodate this."

---

## Validation Goals

The Minecraft v2 demo must prove:

| System | Validation Target |
|--------|-------------------|
| Rendering | 10,000+ visible cubes at 60fps |
| Physics | Player capsule + voxel collision |
| Audio | Positional block sounds |
| AI | Creature pathfinding on voxel grid |
| Networking | Real-time block sync across clients |
| Editor | Creative mode world editing |

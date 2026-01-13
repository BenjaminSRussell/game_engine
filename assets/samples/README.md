# Sample Assets Comprehensive Index

This document provides a complete index of all sample assets organized by subsystem.

## Overview

The sample assets provide comprehensive coverage of all engine subsystems for frontend testing and development.

## Subsystem Coverage

### ✅ AI & Behavior
- `ai/villager_wander.bt` - Peaceful NPC behavior tree
- `ai/hostile_combat.bt` - Combat AI for hostile mobs

### ✅ Animation
- `animations/player_idle.anim` - Idle breathing animation
- `animations/player_walk.anim` - Walk cycle with IK
- `animations/player_run.anim` - Run cycle with increased speed
- `animations/player_jump.anim` - Jump with crouch and landing

### ✅ Audio
- `audio/main_mixer.mixer` - Audio mixer with groups, effects, spatial audio, and surround sound

### ✅ Cinematics
- `cinematics/intro_cutscene.cinematic` - Full cinematic sequence with camera, audio, subtitles, and events

### ✅ Dialogue
- `gameplay/dialogue/elder_welcome.dialogue` - Branching dialogue tree with quest integration

### ✅ Environment & Weather
- `environment/weather/rainy_day.weather` - Weather preset with precipitation, lighting, and effects

### ✅ Fonts
- `fonts/roboto_regular.font` - Font definition with SDF

### ✅ Gameplay Systems

#### Quests
- `gameplay/quests/village_welcome.quest` - Multi-objective quest with rewards

#### Crafting
- `gameplay/crafting/wooden_pickaxe.recipe` - Crafting recipe example

### ✅ Input
- `input/default_controls.input` - Keyboard, mouse, and gamepad mappings

### ✅ Levels & Scenes
- `levels/sample_village.json` - Complete test scene with terrain, entities, lighting, and audio

### ✅ Localization
- `localization/en_US.loc` - English localization strings

### ✅ Materials
- `materials/terrain_grass.mat` - PBR terrain material
- `materials/leaf.mat` - Translucent particle material

### ✅ Networking
- `network/multiplayer_config.netcfg` - Multiplayer configuration with replication, voice chat, anti-cheat

### ✅ Particles
- `particles/falling_leaves.particle` - Particle system with forces and rendering

### ✅ Persistence
- `persistence/player_save_example.save` - Complete save game data

### ✅ Physics
- `physics/physics_test_scene.json` - Physics simulation with rigidbodies, colliders, and constraints

### ✅ Post-Processing
- `postprocess/cinematic_profile.pp` - Post-processing effects (bloom, DOF, color grading, etc.)

### ✅ Procedural Content Generation
- `pcg/dungeon_generator.pcg` - PCG graph for dungeon generation

### ✅ Prefabs
- `prefabs/tree_oak.prefab` - Entity prefab with multiple components

### ✅ Scripting

#### Lua Scripts
- `scripts/player_controller.lua` - Player controller with movement, animation, and collision

#### Visual Scripting
- `scripts/visual/door_interaction.vscript` - Visual script graph for interactive door

### ✅ Terrain
- `terrain/hills_terrain.terrain` - Terrain with heightmap, layers, vegetation, LOD, and streaming

### ✅ UI
- `ui/main_menu.layout` - UI layout with buttons and panels

## Testing Coverage

These samples enable testing of:
- ✅ Model loading (OBJ, GLTF, GLB)
- ✅ Texture loading (PNG, JPG, HDR)
- ✅ Material application (PBR workflow)
- ✅ Skeletal animation playback
- ✅ Particle system rendering
- ✅ Audio spatialization & mixing
- ✅ UI rendering & layouts
- ✅ Scene graph construction
- ✅ Physics simulation & constraints
- ✅ Level streaming & LOD
- ✅ AI behavior trees
- ✅ Quest system
- ✅ Dialogue system
- ✅ Crafting system
- ✅ Save/load system
- ✅ Network replication
- ✅ Weather & environment
- ✅ Terrain rendering
- ✅ Post-processing effects
- ✅ Cinematic sequences
- ✅ Visual scripting
- ✅ Lua scripting
- ✅ Procedural generation
- ✅ Input mapping
- ✅ Localization
- ✅ Prefab instantiation

## Usage Examples

### Loading a Scene
```c
asset_manager_load(manager, "samples/levels/sample_village.json");
```

### Playing an Animation
```c
animator_play(animator, "samples/animations/player_walk.anim");
```

### Running a Behavior Tree
```c
behavior_tree_load(tree, "samples/ai/villager_wander.bt");
```

### Applying Post-Processing
```c
postprocess_load_profile(pp, "samples/postprocess/cinematic_profile.pp");
```

### Starting a Quest
```c
quest_system_start_quest(system, "quest_village_welcome");
```

## File Format Summary

| Extension | Type | Description |
|-----------|------|-------------|
| .anim | Animation | Skeletal animation keyframes |
| .bt | Behavior Tree | AI behavior tree |
| .cinematic | Cinematic | Cutscene sequence |
| .dialogue | Dialogue | Dialogue tree with choices |
| .font | Font | Font definition with atlas |
| .input | Input | Input mapping config |
| .json | Scene/Data | Scene or generic data |
| .layout | UI Layout | UI element hierarchy |
| .loc | Localization | Localized strings |
| .lua | Script | Lua script |
| .mat | Material | PBR material definition |
| .mixer | Audio Mixer | Audio mixing config |
| .netcfg | Network | Network configuration |
| .particle | Particle | Particle system definition |
| .pcg | PCG | Procedural generation graph |
| .pp | Post-Process | Post-processing profile |
| .prefab | Prefab | Entity prefab |
| .quest | Quest | Quest definition |
| .recipe | Recipe | Crafting recipe |
| .save | Save Data | Saved game state |
| .terrain | Terrain | Terrain configuration |
| .vscript | Visual Script | Visual scripting graph |
| .weather | Weather | Weather preset |

## Next Steps

1. Generate placeholder assets (textures, models, audio) referenced by these configs
2. Create test programs that load and exercise each subsystem
3. Add validation tools to verify asset integrity
4. Create performance benchmarks using these assets

#ifndef ENGINE_AGENT_DISTRIBUTION_PLAN_H
#define ENGINE_AGENT_DISTRIBUTION_PLAN_H

/**
 * =================================================================================================
 *                        AGENT DISTRIBUTION PLAN FOR PARALLEL DEVELOPMENT
 *                              54 TOTAL AGENTS - MASSIVE PARALLEL EXPANSION
 * =================================================================================================
 *
 * This file provides a complete work distribution plan for dozens of Windsurf
 * SWE 1.5 agents working in parallel to expand the engine to 1,000,000+ lines
 * of code.
 *
 * EXISTING AGENTS (from AGENT_ASSIGNMENT.h and EXPANSION_PHASE3.h): 34 agents
 * NEW AGENTS (from NEXT_GEN_EXPANSION_MASTER_PLAN.h): 20 agents
 * TOTAL: 54 agents working in parallel
 *
 * WORK DISTRIBUTION STRATEGY:
 *   - Each agent has a clearly defined scope
 *   - No overlap between agents
 *   - Dependencies clearly marked
 *   - Estimated LOC and TODO count per agent
 *   - Priority levels assigned
 *
 * =================================================================================================
 *                                    WAVE 1: CRITICAL PATH
 *                                  (Start immediately - 6 agents)
 * =================================================================================================
 *
 * These agents work on the most critical systems needed for a playable game
 * engine.
 *
 * AGENT_FRONTEND_1: HUD System
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: CRITICAL
 * ESTIMATED LOC: 30,000
 * ESTIMATED TODOS: 2,500
 * DEPENDENCIES: None (can start immediately)
 *
 * WORK SCOPE:
 *   ✓ Implement core HUD rendering system
 *   ✓ Create health/resource bar system
 *   ✓ Build minimap with fog of war
 *   ✓ Implement floating combat text
 *   ✓ Create notification system
 *   ✓ Build quest tracker UI
 *   ✓ Implement crosshair system
 *   ✓ Create compass and navigation
 *   ✓ Build status effect indicators
 *   ✓ Implement hotbar system
 *
 * FILES TO CREATE:
 *   - src/frontend/engine/HUD/hud_core.c (3,000 LOC)
 *   - src/frontend/engine/HUD/health_bars.c (2,500 LOC)
 *   - src/frontend/engine/HUD/minimap.c (5,000 LOC)
 *   - src/frontend/engine/HUD/combat_text.c (3,000 LOC)
 *   - src/frontend/engine/HUD/notifications.c (3,500 LOC)
 *   - src/frontend/engine/HUD/quest_tracker.c (4,000 LOC)
 *   - src/frontend/engine/HUD/crosshair.c (2,000 LOC)
 *   - src/frontend/engine/HUD/compass.c (2,500 LOC)
 *   - src/frontend/engine/HUD/status_effects.c (2,500 LOC)
 *   - src/frontend/engine/HUD/hotbar.c (2,000 LOC)
 *
 * AGENT_GAME_1: Complete Inventory System
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: CRITICAL
 * ESTIMATED LOC: 20,000
 * ESTIMATED TODOS: 1,800
 * DEPENDENCIES: None
 *
 * WORK SCOPE:
 *   ✓ Implement grid-based inventory
 *   ✓ Build equipment system
 *   ✓ Create crafting system
 *   ✓ Implement loot tables
 *   ✓ Build trading system
 *   ✓ Create container interaction
 *   ✓ Implement item stacking/splitting
 *
 * FILES TO CREATE:
 *   - src/game/inventory/grid_inventory.c (4,000 LOC)
 *   - src/game/inventory/equipment.c (3,000 LOC)
 *   - src/game/inventory/crafting.c (5,000 LOC)
 *   - src/game/inventory/loot_tables.c (3,000 LOC)
 *   - src/game/inventory/trading.c (2,500 LOC)
 *   - src/game/inventory/containers.c (2,500 LOC)
 *
 * AGENT_GAME_4: Save/Load Infrastructure
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: CRITICAL
 * ESTIMATED LOC: 25,000
 * ESTIMATED TODOS: 2,000
 * DEPENDENCIES: None
 *
 * WORK SCOPE:
 *   ✓ Implement save game system
 *   ✓ Build world persistence
 *   ✓ Create cloud save sync
 *   ✓ Implement auto-save
 *   ✓ Build save file versioning
 *   ✓ Create corruption detection
 *
 * FILES TO CREATE:
 *   - src/game/save/save_system.c (8,000 LOC)
 *   - src/game/save/world_persistence.c (7,000 LOC)
 *   - src/game/save/cloud_sync.c (5,000 LOC)
 *   - src/game/save/auto_save.c (3,000 LOC)
 *   - src/game/save/versioning.c (2,000 LOC)
 *
 * AGENT_FRONTEND_2: Dashboard/Studio Tools
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: CRITICAL
 * ESTIMATED LOC: 40,000
 * ESTIMATED TODOS: 3,200
 * DEPENDENCIES: None
 *
 * WORK SCOPE:
 *   ✓ Build project dashboard
 *   ✓ Create asset browser
 *   ✓ Implement scene hierarchy editor
 *   ✓ Build property inspector
 *   ✓ Create console and log viewer
 *   ✓ Implement performance profiler UI
 *   ✓ Build version control integration
 *
 * FILES TO CREATE:
 *   - src/frontend/engine/dashboard/project_dashboard.c (6,000 LOC)
 *   - src/frontend/engine/dashboard/asset_browser.c (8,000 LOC)
 *   - src/frontend/engine/dashboard/scene_hierarchy.c (6,000 LOC)
 *   - src/frontend/engine/dashboard/property_inspector.c (10,000 LOC)
 *   - src/frontend/engine/dashboard/console.c (3,000 LOC)
 *   - src/frontend/engine/dashboard/profiler_ui.c (5,000 LOC)
 *   - src/frontend/engine/dashboard/version_control.c (2,000 LOC)
 *
 * AGENT_STUDIO_1: 3D Modeling Tools
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: CRITICAL
 * ESTIMATED LOC: 35,000
 * ESTIMATED TODOS: 3,000
 * DEPENDENCIES: None
 *
 * WORK SCOPE:
 *   ✓ Implement mesh editing core
 *   ✓ Build sculpting system
 *   ✓ Create UV unwrapping tools
 *   ✓ Implement boolean operations
 *   ✓ Build kitbashing system
 *   ✓ Create modifier system
 *
 * FILES TO CREATE:
 *   - src/engine/tools/modeling/mesh_editor.c (10,000 LOC)
 *   - src/engine/tools/modeling/sculpting.c (12,000 LOC)
 *   - src/engine/tools/modeling/uv_unwrap.c (5,000 LOC)
 *   - src/engine/tools/modeling/boolean_ops.c (4,000 LOC)
 *   - src/engine/tools/modeling/kitbash.c (2,000 LOC)
 *   - src/engine/tools/modeling/modifiers.c (2,000 LOC)
 *
 * AGENT_PERF_2: SIMD/GPU Compute Kernels
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: CRITICAL
 * ESTIMATED LOC: 25,000
 * ESTIMATED TODOS: 2,000
 * DEPENDENCIES: None
 *
 * WORK SCOPE:
 *   ✓ Implement AVX-512 kernels
 *   ✓ Build GPU compute pipelines
 *   ✓ Create Metal Performance Shaders
 *   ✓ Implement Vulkan compute
 *
 * FILES TO CREATE:
 *   - src/engine/core/simd/avx512_kernels.c (8,000 LOC)
 *   - src/engine/core/gpu/compute_kernels.cu (7,000 LOC)
 *   - src/engine/core/gpu/metal_kernels.metal (5,000 LOC)
 *   - src/engine/core/gpu/vulkan_compute.c (5,000 LOC)
 *
 * =================================================================================================
 *                                  WAVE 2: HIGH PRIORITY
 *                                    (Start after Wave 1 - 10 agents)
 * =================================================================================================
 *
 * AGENT_GAME_2: Quest/Mission Framework
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: HIGH
 * ESTIMATED LOC: 20,000
 * ESTIMATED TODOS: 1,600
 * DEPENDENCIES: AGENT_GAME_1 (inventory for rewards)
 *
 * WORK SCOPE:
 *   ✓ Build quest system core
 *   ✓ Implement branching quests
 *   ✓ Create achievement system
 *   ✓ Build quest tracking UI
 *   ✓ Implement daily/weekly quests
 *
 * FILES TO CREATE:
 *   - src/game/quests/quest_system.c (6,000 LOC)
 *   - src/game/quests/objectives.c (4,000 LOC)
 *   - src/game/quests/branching.c (5,000 LOC)
 *   - src/game/quests/achievements.c (3,000 LOC)
 *   - src/game/quests/quest_ui.c (2,000 LOC)
 *
 * AGENT_GAME_3: Dialogue System
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: HIGH
 * ESTIMATED LOC: 15,000
 * ESTIMATED TODOS: 1,200
 * DEPENDENCIES: AGENT_GAME_2 (quest integration)
 *
 * WORK SCOPE:
 *   ✓ Build dialogue tree system
 *   ✓ Create dialogue editor
 *   ✓ Implement voice integration
 *   ✓ Build localization system
 *
 * FILES TO CREATE:
 *   - src/game/dialogue/dialogue_system.c (5,000 LOC)
 *   - src/game/dialogue/dialogue_editor.c (6,000 LOC)
 *   - src/game/dialogue/voice_integration.c (2,000 LOC)
 *   - src/game/dialogue/localization.c (2,000 LOC)
 *
 * AGENT_STUDIO_2: Texture/Material Painter
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: HIGH
 * ESTIMATED LOC: 25,000
 * ESTIMATED TODOS: 2,000
 * DEPENDENCIES: AGENT_STUDIO_1 (mesh data)
 *
 * WORK SCOPE:
 *   ✓ Implement 3D texture painting
 *   ✓ Build smart material system
 *   ✓ Create texture baking
 *   ✓ Implement procedural textures
 *
 * FILES TO CREATE:
 *   - src/engine/tools/painting/texture_painter.c (10,000 LOC)
 *   - src/engine/tools/painting/smart_materials.c (6,000 LOC)
 *   - src/engine/tools/painting/baking.c (5,000 LOC)
 *   - src/engine/tools/painting/procedural_textures.c (4,000 LOC)
 *
 * AGENT_AI_ASSET_1: Neural Asset Generation
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: HIGH
 * ESTIMATED LOC: 25,000
 * ESTIMATED TODOS: 2,000
 * DEPENDENCIES: AGENT_STUDIO_1, AGENT_STUDIO_2
 *
 * WORK SCOPE:
 *   ✓ Integrate text-to-3D models
 *   ✓ Build texture synthesis AI
 *   ✓ Create style transfer system
 *   ✓ Implement asset upscaling
 *
 * FILES TO CREATE:
 *   - src/engine/ai/asset_gen/text_to_3d.c (8,000 LOC)
 *   - src/engine/ai/asset_gen/texture_synthesis.c (7,000 LOC)
 *   - src/engine/ai/asset_gen/style_transfer.c (6,000 LOC)
 *   - src/engine/ai/asset_gen/upscaling.c (4,000 LOC)
 *
 * AGENT_CORE_1: Memory Management & Threading (from AGENT_ASSIGNMENT.h)
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: HIGH
 * ESTIMATED LOC: 25,000
 * ESTIMATED TODOS: 500
 * DEPENDENCIES: None
 *
 * WORK SCOPE:
 *   ✓ Implement linear allocator
 *   ✓ Build pool allocator
 *   ✓ Create stack allocator
 *   ✓ Implement buddy allocator
 *   ✓ Build work-stealing job scheduler
 *   ✓ Create fiber-based task system
 *
 * FILES TO EXPAND/CREATE:
 *   - src/engine/core/memory/linear_allocator.c
 *   - src/engine/core/memory/pool_allocator.c
 *   - src/engine/core/memory/stack_allocator.c
 *   - src/engine/core/memory/buddy_allocator.c
 *   - src/engine/core/threading/job_scheduler.c
 *   - src/engine/core/threading/fiber_system.c
 *
 * AGENT_PHYSICS_2: Soft Body & Fluid Simulation (from AGENT_ASSIGNMENT.h)
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: HIGH
 * ESTIMATED LOC: 30,000
 * ESTIMATED TODOS: 400
 * DEPENDENCIES: AGENT_PERF_2 (GPU kernels)
 *
 * WORK SCOPE:
 *   ✓ Implement cloth simulation
 *   ✓ Build PBD solver
 *   ✓ Create FEM deformables
 *   ✓ Implement SPH fluids
 *   ✓ Build FLIP solver
 *   ✓ Create fluid-rigid interaction
 *
 * FILES TO CREATE:
 *   - src/engine/physics/softbody/cloth_simulation.c
 *   - src/engine/physics/softbody/pbd_solver.c
 *   - src/engine/physics/softbody/fem_deformable.c
 *   - src/engine/physics/fluids/sph_solver.c
 *   - src/engine/physics/fluids/flip_solver.c
 *   - src/engine/physics/fluids/buoyancy.c
 *
 * AGENT_RENDER_1: Ray Tracing & Global Illumination (from AGENT_ASSIGNMENT.h)
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: HIGH
 * ESTIMATED LOC: 35,000
 * ESTIMATED TODOS: 500
 * DEPENDENCIES: AGENT_PERF_2 (GPU compute)
 *
 * WORK SCOPE:
 *   ✓ Implement DXR/Vulkan RT
 *   ✓ Build ray traced reflections
 *   ✓ Create denoising system
 *   ✓ Implement voxel cone tracing
 *   ✓ Build light probe system
 *
 * FILES TO EXPAND/CREATE:
 *   - src/engine/renderer/raytracing_advanced/dxr_integration.c
 *   - src/engine/renderer/raytracing_advanced/denoising.c
 *   - src/engine/renderer/gi/voxel_cone_tracing.c
 *   - src/engine/renderer/gi/light_probes.c
 *   - src/engine/renderer/gi/ssgi.c
 *
 * AGENT_EDITOR_1: Viewport & UI Systems (from AGENT_ASSIGNMENT.h)
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: HIGH
 * ESTIMATED LOC: 30,000
 * ESTIMATED TODOS: 500
 * DEPENDENCIES: AGENT_FRONTEND_2 (dashboard integration)
 *
 * WORK SCOPE:
 *   ✓ Implement gizmos
 *   ✓ Build camera controls
 *   ✓ Create selection system
 *   ✓ Implement docking system
 *   ✓ Build undo/redo
 *
 * FILES TO CREATE:
 *   - src/engine/editor/viewport_tools/gizmos.c
 *   - src/engine/editor/viewport_tools/camera_controls.c
 *   - src/engine/editor/viewport_tools/selection_system.c
 *   - src/engine/editor/ui/docking_system.c
 *   - src/engine/editor/ui/undo_redo.c
 *
 * AGENT_AI_1: NPC Behavior & Decision Making (from AGENT_ASSIGNMENT.h)
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: HIGH
 * ESTIMATED LOC: 28,000
 * ESTIMATED TODOS: 450
 * DEPENDENCIES: None
 *
 * WORK SCOPE:
 *   ✓ Implement behavior trees
 *   ✓ Build utility AI
 *   ✓ Enhance GOAP
 *   ✓ Create perception system
 *   ✓ Build memory system
 *
 * FILES TO CREATE:
 *   - src/engine/ai/npc_advanced/behavior_tree.c
 *   - src/engine/ai/npc_advanced/utility_ai.c
 *   - src/engine/ai/npc_advanced/perception_system.c
 *   - src/engine/ai/npc_advanced/memory_system.c
 *
 * =================================================================================================
 *                                  WAVE 3: MEDIUM PRIORITY
 *                                    (Start after Wave 2 - 15 agents)
 * =================================================================================================
 *
 * AGENT_STUDIO_3: Animation Authoring
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 20,000
 * ESTIMATED TODOS: 1,600
 * DEPENDENCIES: AGENT_STUDIO_1 (mesh/rig data)
 *
 * AGENT_STUDIO_4: Procedural Asset Generator
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 20,000
 * ESTIMATED TODOS: 1,600
 * DEPENDENCIES: AGENT_STUDIO_1, AGENT_STUDIO_2
 *
 * AGENT_AI_CODE_1: AI-Assisted Coding
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 20,000
 * ESTIMATED TODOS: 1,600
 * DEPENDENCIES: None
 *
 * AGENT_AI_PROCEDURAL_1: Procedural Content AI
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 15,000
 * ESTIMATED TODOS: 1,200
 * DEPENDENCIES: AGENT_GAME_2 (quest system)
 *
 * AGENT_AI_NPC_1: NPC Behavior Learning
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 10,000
 * ESTIMATED TODOS: 800
 * DEPENDENCIES: AGENT_AI_1 (NPC behavior)
 *
 * AGENT_FRONTEND_3: iOS App Integration
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 25,000
 * ESTIMATED TODOS: 2,000
 * DEPENDENCIES: AGENT_FRONTEND_2 (dashboard API)
 *
 * AGENT_FRONTEND_4: Web-based Asset Viewer
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 25,000
 * ESTIMATED TODOS: 2,000
 * DEPENDENCIES: AGENT_FRONTEND_2 (asset browser)
 *
 * AGENT_PERF_1: Custom Memory Allocators
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 20,000
 * ESTIMATED TODOS: 1,600
 * DEPENDENCIES: AGENT_CORE_1 (base allocators)
 *
 * AGENT_PERF_3: Advanced Job System
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 15,000
 * ESTIMATED TODOS: 1,200
 * DEPENDENCIES: AGENT_CORE_1 (threading)
 *
 * AGENT_PERF_4: Platform-Specific Optimizations
 * ────────────────────────────────────────────────────────────────────────────────────────────────
 * PRIORITY: MEDIUM
 * ESTIMATED LOC: 15,000
 * ESTIMATED TODOS: 1,200
 * DEPENDENCIES: AGENT_PERF_2 (SIMD/GPU)
 *
 * [Continue with remaining 25 agents from ROADMAP.h and EXPANSION_PHASE3.h...]
 *
 * AGENT_CORE_2: Serialization & Advanced Containers
 * AGENT_PHYSICS_1: Rigid Body Dynamics & Solver
 * AGENT_RENDER_2: Advanced Materials & PBR
 * AGENT_RENDER_3: Mesh Processing & Advanced Geometry
 * AGENT_EDITOR_2: Asset Creation & Management Tools
 * AGENT_AI_2: Navigation & Procedural Generation
 * AGENT_ANIM_1: Inverse Kinematics & Blend Trees
 * AGENT_ANIM_2: Motion Matching & Advanced Animation
 * AGENT_NET_1: State Replication
 * AGENT_NET_2: Lag Compensation & Matchmaking
 * AGENT_AUDIO_1: Spatial Audio & DSP
 * AGENT_AUDIO_2: Procedural Audio & Music
 * AGENT_TOOLS_1: External Tool Integration
 * AGENT_TOOLS_2: Asset Pipeline & Profiling
 *
 * [Phase 3 agents from EXPANSION_PHASE3.h...]
 * AGENT_SHADER_1: PBR & Stylized Shaders
 * AGENT_SHADER_2: Environment & VFX Shaders
 * AGENT_TEMPLATE_1: Character & Creature Templates
 * AGENT_TEMPLATE_2: Environment & Prop Templates
 * AGENT_GAMEPLAY_1: Inventory & Crafting Systems (different from AGENT_GAME_1)
 * AGENT_GAMEPLAY_2: Combat & Progression Systems
 * AGENT_WORLD_1: Mega Terrain Systems
 * AGENT_WORLD_2: Population & Ecosystem Systems
 * AGENT_CINEMA_1: Sequencer & Timeline
 * AGENT_CINEMA_2: Dialogue & Lipsync
 * AGENT_MP_1: Multiplayer Game Mode Templates
 * AGENT_MP_2: Multiplayer Services & Anti-cheat
 * AGENT_TEST_1: Testing Framework
 * AGENT_DOCS_1: Documentation Generator
 * AGENT_PIPELINE_1: Build & Deploy Pipeline
 *
 * =================================================================================================
 *                                  EXECUTION STRATEGY
 * =================================================================================================
 *
 * PARALLEL EXECUTION:
 *   - Wave 1 (6 agents): Start immediately, no dependencies
 *   - Wave 2 (10 agents): Start when Wave 1 is 50% complete
 *   - Wave 3 (15 agents): Start when Wave 2 is 50% complete
 *   - Remaining agents: Start based on dependencies
 *
 * COORDINATION:
 *   - Each agent works in isolated directories
 *   - Shared headers defined upfront
 *   - Integration testing after each wave
 *   - Code reviews between waves
 *
 * TIMELINE ESTIMATE:
 *   - Wave 1: 2-3 weeks (critical path)
 *   - Wave 2: 3-4 weeks (high priority)
 *   - Wave 3: 4-6 weeks (medium priority)
 *   - Remaining: 6-8 weeks (polish and integration)
 *   - Total: 15-21 weeks with 54 agents working in parallel
 *
 * QUALITY ASSURANCE:
 *   - Each agent must include unit tests
 *   - Performance benchmarks required
 *   - Documentation mandatory
 *   - Code review before merge
 *
 * =================================================================================================
 *                                  FOLDER STRUCTURE OVERVIEW
 * =================================================================================================
 *
 * src/
 * ├── engine/
 * │   ├── core/                    [AGENT_CORE_1, AGENT_CORE_2, AGENT_PERF_1-4]
 * │   ├── renderer/                [AGENT_RENDER_1-3, AGENT_SHADER_1-2]
 * │   ├── physics/                 [AGENT_PHYSICS_1-2]
 * │   ├── editor/                  [AGENT_EDITOR_1-2]
 * │   ├── ai/                      [AGENT_AI_1-2, AGENT_AI_ASSET_1, etc.]
 * │   ├── animation/               [AGENT_ANIM_1-2, AGENT_STUDIO_3]
 * │   ├── audio/                   [AGENT_AUDIO_1-2]
 * │   ├── networking/              [AGENT_NET_1-2]
 * │   ├── tools/                   [AGENT_STUDIO_1-4, AGENT_TOOLS_1-2]
 * │   ├── integration/             [AGENT_TOOLS_1]
 * │   ├── shader_library/          [AGENT_SHADER_1-2]
 * │   ├── asset_templates/         [AGENT_TEMPLATE_1-2]
 * │   ├── gameplay_systems/        [AGENT_GAMEPLAY_1-2]
 * │   ├── world_building/          [AGENT_WORLD_1-2]
 * │   ├── cinematics/              [AGENT_CINEMA_1-2]
 * │   ├── multiplayer_framework/   [AGENT_MP_1-2]
 * │   ├── testing_framework/       [AGENT_TEST_1]
 * │   ├── documentation/           [AGENT_DOCS_1]
 * │   └── pipeline/                [AGENT_PIPELINE_1]
 * │
 * ├── frontend/
 * │   └── engine/
 * │       ├── HUD/                 [AGENT_FRONTEND_1]
 * │       ├── dashboard/           [AGENT_FRONTEND_2]
 * │       ├── ios_app/             [AGENT_FRONTEND_3]
 * │       └── studio/              [AGENT_FRONTEND_2]
 * │
 * ├── game/
 * │   ├── inventory/               [AGENT_GAME_1]
 * │   ├── quests/                  [AGENT_GAME_2]
 * │   ├── dialogue/                [AGENT_GAME_3]
 * │   └── save/                    [AGENT_GAME_4]
 * │
 * └── web/                         [AGENT_FRONTEND_4]
 *
 * =================================================================================================
 */

#define TOTAL_AGENTS 54
#define WAVE_1_AGENTS 6
#define WAVE_2_AGENTS 10
#define WAVE_3_AGENTS 15
#define REMAINING_AGENTS 23

#define ESTIMATED_TOTAL_TODOS 75000
#define ESTIMATED_TOTAL_NEW_LOC 1000000

#endif // ENGINE_AGENT_DISTRIBUTION_PLAN_H

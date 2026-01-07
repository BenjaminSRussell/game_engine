#ifndef ENGINE_EXPANSION_AGENTS_H
#define ENGINE_EXPANSION_AGENTS_H

/**
 * =================================================================================================
 *                       EXPANSION AGENT ASSIGNMENT - PHASE 2 (NON-OVERLAPPING)
 *                              12 Specialized Agents - 17,630+ TODOs
 * =================================================================================================
 *
 * This file defines NEW agent assignments for Phase 2 expansion features.
 * These DO NOT OVERLAP with agents defined in AGENT_ASSIGNMENT.h.
 *
 * All features focus on:
 *   - macOS-specific optimizations (Metal, FSEvents, dylib hot-reload)
 *   - Front-end asset creation (Unreal-style drag-and-drop editor)
 *   - Prebuilt NPC systems (no coding required for game designers)
 *   - Blender & AI integration (VSCode/Antigravity workflow)
 *
 * TODO FORMAT: // TODO(AGENT_XXX_N): [Description] [Difficulty: 1-10]
 *
 * =================================================================================================
 */

/* =================================================================================================
 * AGENT_MACOS_1: Metal Rendering Backend
 * =================================================================================================
 * ESTIMATED TODOS: ~1,730
 * ESTIMATED LOC: 45,000
 *
 * RESPONSIBILITIES:
 *   - Metal device initialization and management
 *   - Command buffer recording with triple buffering
 *   - Render pipeline state (PSO) creation and caching
 *   - Metal texture and buffer management
 *   - Shader compilation from metallib files
 *   - Compute shader pipelines for physics/particles
 *   - Metal ray tracing acceleration structures
 *   - Mesh shader amplification
 *   - Bindless textures via argument buffers
 *
 * NEW FILES (src/engine/renderer/metal/):
 *   - metal_device.m
 *   - metal_command_buffer.m
 *   - metal_render_pipeline.m
 *   - metal_texture.m
 *   - metal_buffer.m
 *   - metal_shader_compiler.m
 *   - metal_compute_pipeline.m
 *   - metal_raytracing.m
 *   - metal_mesh_shaders.m
 *   - metal_argument_buffers.m
 */

/* =================================================================================================
 * AGENT_MACOS_2: macOS Platform Services
 * =================================================================================================
 * ESTIMATED TODOS: ~1,150
 * ESTIMATED LOC: 25,000
 *
 * RESPONSIBILITIES:
 *   - FSEvents directory watching with 200ms debounce
 *   - Versioned dylib hot-reload (game_v1.dylib -> game_v2.dylib)
 *   - Metal I/O for DirectStorage-style async loading
 *   - CoreML integration for on-device AI inference
 *   - Game Controller framework with haptic feedback
 *   - Swift/Objective-C bridge layer
 *   - Instruments integration with signpost API
 *   - LLDB debugger integration
 *
 * NEW FILES (src/engine/platform/macos_advanced/):
 *   - fsevents_watcher.m
 *   - dylib_hot_reload.m
 *   - metal_io.m
 *   - coreml_inference.m
 *   - gamecontroller_haptics.m
 *   - swift_bridge.swift
 *   - instruments_profiling.m
 *   - xcode_debugger.m
 */

/* =================================================================================================
 * AGENT_FRONTEND_1: Drag-and-Drop Asset Editor
 * =================================================================================================
 * ESTIMATED TODOS: ~1,750
 * ESTIMATED LOC: 30,000
 *
 * RESPONSIBILITIES:
 *   - OS-native drag-and-drop handlers (NSPasteboard on macOS)
 *   - 3D asset placement with snapping to grid/surface
 *   - Multi-object transform gizmos with constraints
 *   - World-space and local-space grid snapping
 *   - Instance duplication with randomization (scale, rotation)
 *   - Real-time 3D preview renderer for assets
 *   - Right-click context menus with action system
 *   - Component property editing with sliders/fields
 *   - Box and lasso selection tools
 *   - Multi-step undo/redo with command grouping
 *
 * NEW FILES (src/engine/editor/asset_creation/):
 *   - drag_drop_system.c
 *   - asset_placement.c
 *   - transform_gizmo_advanced.c
 *   - grid_snapping.c
 *   - duplication_randomization.c
 *   - asset_preview_3d.c
 *   - context_menu_system.c
 *   - property_inspector.c
 *   - selection_marquee.c
 *   - undo_redo_advanced.c
 */

/* =================================================================================================
 * AGENT_FRONTEND_2: Asset Browser & Library
 * =================================================================================================
 * ESTIMATED TODOS: ~1,350
 * ESTIMATED LOC: 25,000
 *
 * RESPONSIBILITIES:
 *   - GPU-accelerated thumbnail rendering for all asset types
 *   - Full-text search with tag-based filtering
 *   - Hierarchical tag system with categories
 *   - User collections and favorites
 *   - Multi-format import wizard (FBX, OBJ, glTF, PNG, etc.)
 *   - Dependency graph visualization
 *   - Asset integrity checking and validation
 *   - Browse packed .pak files
 *   - Recent assets tracking with history
 *
 * NEW FILES (src/engine/editor/asset_library/):
 *   - thumbnail_generator.c
 *   - asset_search_filter.c
 *   - asset_tagging.c
 *   - asset_collections.c
 *   - asset_import_wizard.c
 *   - asset_dependency_graph.c
 *   - asset_validation_tool.c
 *   - pak_browser.c
 *   - recent_assets.c
 */

/* =================================================================================================
 * AGENT_FRONTEND_3: Scene Editing Tools
 * =================================================================================================
 * ESTIMATED TODOS: ~1,250
 * ESTIMATED LOC: 25,000
 *
 * RESPONSIBILITIES:
 *   - Scene hierarchy tree view with drag-drop parenting
 *   - Multi-object property editing with conflict resolution
 *   - Find/replace functionality in scene
 *   - Camera bookmark system for quick navigation
 *   - Layer-based organization with visibility toggles
 *   - Level streaming management (load/unload)
 *   - World partition tools for large worlds
 *   - Scene validation and error checking
 *
 * NEW FILES (src/engine/editor/scene_tools/):
 *   - scene_hierarchy.c
 *   - multi_edit.c
 *   - search_replace.c
 *   - scene_bookmarks.c
 *   - scene_layers.c
 *   - level_streaming.c
 *   - world_partition.c
 *   - scene_validation.c
 */

/* =================================================================================================
 * AGENT_NPC_1: Prebuilt NPC Behaviors
 * =================================================================================================
 * ESTIMATED TODOS: ~2,300
 * ESTIMATED LOC: 35,000
 *
 * RESPONSIBILITIES:
 *   - Patrol route system with waypoints
 *   - Guard behavior with alert/combat states
 *   - Shopkeeper with trading dialogue and inventory
 *   - Companion AI with follow/combat behaviors
 *   - Civilian daily routine simulation
 *   - Melee combat AI with attack patterns
 *   - Ranged combat AI with cover system
 *   - Multi-phase boss encounter system
 *   - Faction relationship system
 *   - Time-based behavior scheduling
 *
 * NEW FILES (src/engine/npc/prebuilt/):
 *   - npc_patrol.c
 *   - npc_guard.c
 *   - npc_shopkeeper.c
 *   - npc_companion.c
 *   - npc_civilian.c
 *   - npc_enemy_melee.c
 *   - npc_enemy_ranged.c
 *   - npc_boss.c
 *   - npc_faction_system.c
 *   - npc_scheduling.c
 */

/* =================================================================================================
 * AGENT_NPC_2: NPC Configuration Editor UI
 * =================================================================================================
 * ESTIMATED TODOS: ~1,250
 * ESTIMATED LOC: 20,000
 *
 * RESPONSIBILITIES:
 *   - Browse and select NPC templates
 *   - Visual appearance customization (no code)
 *   - Drag-drop behavior assignment to NPCs
 *   - Branching dialogue tree editor
 *   - Loot table and inventory configuration
 *   - Stats and abilities editor
 *   - Real-time NPC preview in editor
 *
 * NEW FILES (src/engine/editor/npc_editor/):
 *   - npc_template_browser.c
 *   - npc_appearance_editor.c
 *   - npc_behavior_picker.c
 *   - npc_dialogue_editor.c
 *   - npc_inventory_editor.c
 *   - npc_stat_editor.c
 *   - npc_preview.c
 */

/* =================================================================================================
 * AGENT_BLENDER_1: Blender Export Pipeline (C/C++)
 * =================================================================================================
 * ESTIMATED TODOS: ~1,550
 * ESTIMATED LOC: 25,000
 *
 * RESPONSIBILITIES:
 *   - glTF/FBX mesh import with triangulation
 *   - Blender material to engine PBR conversion
 *   - Skeleton and rig import with hierarchy
 *   - Animation import with retargeting support
 *   - Automatic collision mesh generation
 *   - LOD chain import and processing
 *   - Real-time sync with Blender (live link)
 *   - Batch export pipeline for automation
 *
 * NEW FILES (src/engine/integration/blender_pipeline/):
 *   - blender_mesh_import.c
 *   - blender_material_convert.c
 *   - blender_skeleton_import.c
 *   - blender_animation_import.c
 *   - blender_collision_import.c
 *   - blender_lod_import.c
 *   - blender_live_link.c
 *   - blender_batch_export.c
 */

/* =================================================================================================
 * AGENT_BLENDER_2: Blender Addon (Python)
 * =================================================================================================
 * ESTIMATED TODOS: ~1,180
 * ESTIMATED LOC: 12,000
 *
 * RESPONSIBILITIES:
 *   - Blender addon registration and preferences
 *   - Export operators for mesh/skeleton/animation
 *   - Engine settings panel in Blender UI
 *   - Material baking to PBR maps
 *   - WebSocket live link client
 *   - Automatic collision mesh generation tool
 *   - Automatic LOD generation tool
 *
 * NEW FILES (src/engine/integration/blender_addon/):
 *   - __init__.py
 *   - export_operators.py
 *   - engine_panel.py
 *   - material_baker.py
 *   - live_link_client.py
 *   - collision_generator.py
 *   - lod_generator.py
 */

/* =================================================================================================
 * AGENT_AI_TOOLS_1: VSCode/Antigravity Integration
 * =================================================================================================
 * ESTIMATED TODOS: ~1,200
 * ESTIMATED LOC: 20,000
 *
 * RESPONSIBILITIES:
 *   - Language Server Protocol implementation
 *   - Project context export for AI assistants
 *   - Template code generation with snippets
 *   - Shader language autocompletion
 *   - Debug Adapter Protocol for engine debugging
 *   - Performance telemetry analysis
 *
 * NEW FILES (src/engine/integration/ai_tools/):
 *   - lsp_server.c
 *   - project_context_exporter.c
 *   - code_snippet_generator.c
 *   - shader_autocomplete.c
 *   - debug_adapter.c
 *   - telemetry_analyzer.c
 */

/* =================================================================================================
 * AGENT_ENVIRONMENT_1: Realistic Environment Builder
 * =================================================================================================
 * ESTIMATED TODOS: ~1,600
 * ESTIMATED LOC: 30,000
 *
 * RESPONSIBILITIES:
 *   - Biome painting tool with heat/moisture maps
 *   - Procedural vegetation placement (grass, trees, shrubs)
 *   - Rock and debris scattering with physics
 *   - Water body creation (lakes, rivers, oceans)
 *   - Road and path spline tools
 *   - Modular building placement system
 *   - Weather zone configuration
 *   - Time-of-day lighting presets
 *   - Atmosphere and volumetric fog editor
 *
 * NEW FILES (src/engine/environment/builder/):
 *   - biome_painter.c
 *   - vegetation_placement.c
 *   - rock_scatter.c
 *   - water_body_editor.c
 *   - road_spline_tool.c
 *   - building_placer.c
 *   - weather_zone_editor.c
 *   - lighting_scenario.c
 *   - atmosphere_editor.c
 */

/* =================================================================================================
 * AGENT_ENVIRONMENT_2: Non-Realistic/Stylized Environment Tools
 * =================================================================================================
 * ESTIMATED TODOS: ~1,320
 * ESTIMATED LOC: 25,000
 *
 * RESPONSIBILITIES:
 *   - Voxel-style environment builder
 *   - Automatic low-poly mesh generation
 *   - Toon/cel shading materials
 *   - Outline post-processing effect
 *   - Color palette constraint system
 *   - Pixel art 3D rendering mode
 *   - Sketch/hand-drawn filter effect
 *   - Stylized water shader
 *
 * NEW FILES (src/engine/environment/stylized/):
 *   - voxel_builder.c
 *   - low_poly_generator.c
 *   - cel_shading_material.c
 *   - outline_renderer.c
 *   - color_palette_system.c
 *   - pixel_art_renderer.c
 *   - hand_drawn_effect.c
 *   - stylized_water.c
 */

/* =================================================================================================
 *                                 AGENT SUMMARY TABLE
 * =================================================================================================
 *
 * | Agent ID          | Focus Area           | Est. LOC  | Est. TODOs |
 * |-------------------|----------------------|-----------|------------|
 * | AGENT_MACOS_1     | Metal Rendering      | 45,000    | 1,730      |
 * | AGENT_MACOS_2     | macOS Platform       | 25,000    | 1,150      |
 * | AGENT_FRONTEND_1  | Drag-Drop Editor     | 30,000    | 1,750      |
 * | AGENT_FRONTEND_2  | Asset Library        | 25,000    | 1,350      |
 * | AGENT_FRONTEND_3  | Scene Tools          | 25,000    | 1,250      |
 * | AGENT_NPC_1       | Prebuilt NPCs        | 35,000    | 2,300      |
 * | AGENT_NPC_2       | NPC Editor UI        | 20,000    | 1,250      |
 * | AGENT_BLENDER_1   | Blender Pipeline     | 25,000    | 1,550      |
 * | AGENT_BLENDER_2   | Blender Addon        | 12,000    | 1,180      |
 * | AGENT_AI_TOOLS_1  | AI Integration       | 20,000    | 1,200      |
 * | AGENT_ENVIRONMENT_1| Realistic Env       | 30,000    | 1,600      |
 * | AGENT_ENVIRONMENT_2| Stylized Env        | 25,000    | 1,320      |
 * |-------------------|----------------------|-----------|------------|
 * | TOTAL             |                      | 317,000   | 17,630     |
 *
 * Combined with existing ~33,000 LOC from ROADMAP.h = 350,000+ LOC
 *
 * =================================================================================================
 */

/* =================================================================================================
 *                             QUICK REFERENCE: Agent by New File Pattern
 * =================================================================================================
 *
 * renderer/metal/*                      -> AGENT_MACOS_1
 * platform/macos_advanced/*             -> AGENT_MACOS_2
 * editor/asset_creation/*               -> AGENT_FRONTEND_1
 * editor/asset_library/*                -> AGENT_FRONTEND_2
 * editor/scene_tools/*                  -> AGENT_FRONTEND_3
 * npc/prebuilt/*                        -> AGENT_NPC_1
 * editor/npc_editor/*                   -> AGENT_NPC_2
 * integration/blender_pipeline/*        -> AGENT_BLENDER_1
 * integration/blender_addon/*           -> AGENT_BLENDER_2
 * integration/ai_tools/*                -> AGENT_AI_TOOLS_1
 * environment/builder/*                 -> AGENT_ENVIRONMENT_1
 * environment/stylized/*                -> AGENT_ENVIRONMENT_2
 *
 * =================================================================================================
 */

#endif // ENGINE_EXPANSION_AGENTS_H

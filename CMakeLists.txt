# CMakeLists.txt for MinecraftV2 - Refactored for Modularity
# TODO: High - Investigate dependency management (e.g., Conan, vcpkg) for third-party libraries.
# TODO: Medium - Add support for other platforms (Linux, Windows) by abstracting platform-specific code.

cmake_minimum_required(VERSION 3.20)
project(Minecraft C CXX OBJC)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# PHASE 1: Build System Restructuring - Runtime/Editor Separation
option(BUILD_EDITOR "Build editor executable with ImGui and editor tools" ON)
option(BUILD_RUNTIME_ONLY "Build lean runtime without any editor code" OFF)

if(BUILD_EDITOR)
    message(STATUS "========================================")
    message(STATUS "BUILD MODE: Editor (WITH_EDITOR=1)")
    message(STATUS "Building EditorApp with ImGui, tools, and full reflection")
    message(STATUS "========================================")
else()
    message(STATUS "========================================")
    message(STATUS "BUILD MODE: Runtime Only (Editor code excluded)")
    message(STATUS "Building lean game binary for shipping")
    message(STATUS "========================================")
endif()

# Find dependencies
find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW3 not found via find_package, trying pkg-config...")
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW3 glfw3)
        if(GLFW3_FOUND)
            message(STATUS "GLFW3 found via pkg-config")
            include_directories(${GLFW3_INCLUDE_DIRS})
            link_directories(${GLFW3_LIBRARY_DIRS})
        endif()
    endif()
endif()

if(NOT glfw3_FOUND AND NOT GLFW3_FOUND)
    message(WARNING "GLFW3 not found - window functionality will be limited")
else()
    message(STATUS "GLFW3 found and will be linked")
    message(STATUS "  glfw3_FOUND: ${glfw3_FOUND}")
    message(STATUS "  GLFW3_FOUND: ${GLFW3_FOUND}")
    message(STATUS "  glfw3_LIBRARIES: ${glfw3_LIBRARIES}")
    message(STATUS "  GLFW3_LIBRARIES: ${GLFW3_LIBRARIES}")
    message(STATUS "  GLWF3_INCLUDE_DIRS: ${GLFW3_INCLUDE_DIRS}")
    message(STATUS "  glfw3_INCLUDE_DIRS: ${glfw3_INCLUDE_DIRS}")
endif()

include_directories(
    include 
    include_private 
    include/vendor 
    include/vendor/lua
    src/engine/include
    src/game/minecraftv2/include
    src/engine
    src
    src/editor/Core/Bridges/include
)


# --- Custom Targets ---
# TODO: High - Create a separate target for running tests.
add_custom_target(validate_assets
    COMMAND python3 ${CMAKE_SOURCE_DIR}/src/engine/scripts/validate_assets.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating assets (PNG sizes, atlas constraints)"
)

add_custom_target(compile_shaders
    COMMAND ${CMAKE_SOURCE_DIR}/src/engine/scripts/compile_shaders.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling Vulkan shaders to SPIR-V"
)

# --- Source File Lists ---

# Engine Sources
set(ENGINE_SOURCES
    # Core
    # src/engine/core/main.c
    src/engine/core/engine.c
    src/engine/core/stubs.c
    src/engine/platform/swift_bridge.c
    src/engine/renderer/grid_renderer.c
    src/engine/core/logger.c
    src/engine/core/memory.c
    src/engine/core/performance.c
    src/engine/core/string_utils.c
    src/engine/core/containers.c
    src/engine/core/introspection/type_registry.c  # New AI Introspection
    src/engine/ai/api/ai_command_processor.c       # New AI API
    src/engine/ai/content/content_manager.c        # New AI Content Gen
    src/engine/core/cache.c
    src/engine/core/config_loader.c
    src/engine/core/config.c
    src/engine/core/utils.c
    src/engine/core/big_int.c
    src/engine/core/pool.c
    src/engine/core/job.c
    src/engine/core/asset_compression.c
    src/engine/core/dynamic_array.c
    src/engine/core/hashmap.c
    src/engine/core/file_watcher.c
    src/engine/tools/hot_reload.c
    # src/engine/assets/importers/audio_importer.c
    src/engine/core/time_system.c
    src/engine/core/config_system.c
    src/engine/core/unified_engine_integration.c
    # Resource
    src/engine/resource/asset_manager.c
    src/engine/resource/asset_instance.c
    src/engine/resource/vfs.c
    src/engine/scripting/script_system.c
    src/engine/physics/solver/debug_visualization.c
    src/engine/resource/vfs_async.c
    # Audio
    src/engine/audio/audio_system.c
    src/engine/audio/audio_loader.c
    src/engine/audio/audio_reverb.c
    src/engine/audio/audio_event_queue.c
    src/engine/audio/audio_effects.c
    src/engine/audio/audio_occlusion.c
    src/engine/audio/audio_streaming.c
    src/engine/audio/music_system.c
    src/engine/audio/ambient_system.c
    src/engine/audio/audio_manager.c
    src/engine/audio/underwater_filter.c
    # Spatial Audio
    src/engine/audio/spatial/spatial_audio_config.c
    src/engine/audio/spatial/dolby_atmos_renderer.c
    src/engine/audio/spatial/spatial_audio_bridge.c
    # Physics
    src/engine/physics/simulation_loop.c
    src/engine/physics/simulation_impl.c
    src/engine/physics/physics_advanced_impl.c
    src/engine/physics/collision_detection.c
    src/engine/physics/continuous_collision.c
    # src/engine/physics/physics_api_impl.c
    src/engine/physics/core/physics_shapes.c
    src/engine/physics/core/physics_core.c
    src/engine/physics/core/physics_rigid_body_helpers.c
    src/engine/physics/dynamics/physics_solver.c
    src/engine/physics/collision/broadphase.c
    # Memory
    src/engine/core/memory/aligned_allocator.c
    # SIMD Math Library (Physics DOD Foundation)
    src/engine/core/simd/simd_math_scalar.c
    # SoA Containers (Physics DOD - Phase 2)
    src/engine/core/containers/soa_rigidbody.c
    # Physics Integration (Physics DOD - Phase 2)
    src/engine/physics/integration/physics_integration.c
    # Broadphase AABB Tree (Physics DOD - Phase 3)
    src/engine/physics/broadphase/aabb_tree.c
    # Narrowphase GJK/EPA (Physics DOD - Phase 4)
    src/engine/physics/narrowphase/collision_gjk_epa.c
    src/engine/physics/narrowphase/contact_manifold.c
    # Input
    src/engine/input/input_factory.c
    src/engine/renderer/metal/metal_backend.c
    src/engine/renderer/metal/metal_device.m

    # ... (rest of sources)


    # Window
    src/engine/core/window.c
    src/engine/core/game_loop.c
    # Swift Bridge for Editor (temporarily disabled during Phase 1)
    # TODO: Enable after resolving editor_enhancements.c type dependencies
    # src/editor/Core/Bridges/engine_bridge.c
    # Renderer
    src/engine/renderer/core/renderer.c
    src/engine/renderer/core/lighting.c
    src/engine/renderer/core/mesh.c
    src/engine/renderer/core/scene.c
    src/engine/scene/scene_system_impl.c
    src/engine/renderer/core/shader.c
    src/engine/renderer/core/texture_atlas.c
    src/engine/renderer/core/texture_system.c
    src/engine/renderer/core/texture_loader_impl.c
    src/engine/renderer/core/render_graph.c        # New Glass-Box Render Graph
    src/engine/renderer/core/graph_renderer.c      # New Glass-Box Renderer Implementation
    # Frame Graph System (Weeks 1-4)
    src/engine/renderer/frame_graph/frame_graph.c
    src/engine/renderer/frame_graph/compile_execute.c
    src/engine/renderer/frame_graph/resource_pool.c
    # Material System V2 (Weeks 5-12)
    src/engine/renderer/materials/shader_permutations.c
    src/engine/renderer/materials/material_instance.c
    src/engine/renderer/materials/material_serialization.c
    src/engine/renderer/materials/material_hot_reload.c
    src/engine/renderer/materials/material_templates.c
    # Post-Processing Foundation (Week 6+)
    src/engine/renderer/post_processing/taa.c
    src/engine/renderer/post_processing/post_processing_pipeline.c
    src/engine/renderer/systems/camera.c
    src/engine/renderer/systems/animation_system.c
    src/engine/renderer/systems/sprite_3d_renderer.c
    # Vulkan-specific files - disabled for OpenGL-only build
    src/engine/renderer/systems/particle_renderer.c
    src/engine/renderer/systems/ui_renderer.c
    # src/engine/renderer/mesh_optimizer.c
    src/engine/renderer/post_processing.c
    src/engine/renderer/systems/voxel_renderer.c
    # Math (needed by renderer)
    src/engine/math/vec3.c
    src/engine/math/mat4.c
    src/engine/math/quat.c
    # ECS
    src/engine/ecs/ecs.c
#DISABLED     src/engine/ecs/ecs_impl.c
#DISABLED     src/engine/ecs/ecs_advanced_impl.c
#DISABLED     src/engine/ecs/ecs_system_impl.c
    # src/engine/ecs/ecs_archetype.c (not implemented yet)
    src/engine/ecs/systems/asset_render_system.c
    src/engine/ecs/components/transform_component.c
    src/engine/ecs/components/asset_instance_component.c
    src/engine/physics/physics_api_impl.c
    src/engine/physics/core_rigid_body.c
    # Renderer

    src/engine/renderer/renderer_complete.c
    src/engine/renderer/mesh.c
    # src/engine/renderer/culling.c
    # Input
    src/engine/input/input.c
    # UI System (layout and text - enabled)
    src/engine/ui/layout/layout_solver.c
    src/engine/ui/text/rich_text_parser.c
    # UI System (other components - temporarily disabled, need proper type definitions)
    # src/engine/ui/ui_widgets.c
    # src/engine/ui/ui_accessibility.c
    # src/engine/ui/ui_layout.c
    # src/engine/ui/ui_animation.c
    # src/engine/ui/ui_layout_system.c
    # src/engine/ui/core/ui_layout_engine.c
    # src/engine/ui/core/ui_event_dispatcher.c
    # src/engine/ui/text/font_atlas_gen.c
    # Editor Sources (Disabled for Phase 1 to fix compilation)
    # Note: These files will be re-enabled incrementally in Phase 2
    # src/engine/editor/editor_main.c
    src/engine/editor/editor_enhancements.c
    # ... (all editor subdirectories excluded)
    # editor/material_editor/material_editor_preview.c -- Included in node_graph.c
    
    # Editor - Terrain Editor
    editor/terrain_editor/sculpting.c
    editor/terrain_editor/terrain_editor_ui.c
    editor/terrain_editor/terrain_editor_viewport.c
    editor/editor_context.c
    # src/engine/editor/terrain_editor/brush_system.c # Replaced by sculpting.c
    # Editor - Console
    src/engine/editor/console/console_system.c
    src/engine/editor/asset_creation/grid_snapping.c
    src/engine/editor/asset_creation/undo_redo_advanced.c
    src/engine/core/json.c
    # OpenGL Backend
    src/engine/renderer/opengl/opengl_backend.c
    # Vendor (Lua)
    src/engine/vendor/lua/lapi.c
    src/engine/vendor/lua/lauxlib.c
    src/engine/vendor/lua/lbaselib.c
    src/engine/vendor/lua/lcode.c
    src/engine/vendor/lua/lcorolib.c
    src/engine/vendor/lua/lctype.c
    src/engine/vendor/lua/ldblib.c
    src/engine/vendor/lua/ldebug.c
    src/engine/vendor/lua/ldo.c
    src/engine/vendor/lua/ldump.c
    src/engine/vendor/lua/lfunc.c
    src/engine/vendor/lua/lgc.c
    src/engine/vendor/lua/linit.c
    src/engine/vendor/lua/liolib.c
    src/engine/vendor/lua/llex.c
    src/engine/vendor/lua/lmathlib.c
    src/engine/vendor/lua/lmem.c
    src/engine/vendor/lua/loadlib.c
    src/engine/vendor/lua/lobject.c
    src/engine/vendor/lua/lopcodes.c
    src/engine/vendor/lua/loslib.c
    src/engine/vendor/lua/lparser.c
    src/engine/vendor/lua/lstate.c
    src/engine/vendor/lua/lstring.c
    src/engine/vendor/lua/lstrlib.c
    src/engine/vendor/lua/ltable.c
    src/engine/vendor/lua/ltablib.c
    src/engine/vendor/lua/ltm.c
    src/engine/vendor/lua/lundump.c
    src/engine/vendor/lua/lutf8lib.c
    src/engine/vendor/lua/lvm.c
    src/engine/vendor/lua/lzio.c
    
    # AI Advanced
    src/engine/ai/npc_advanced/utility_ai.c
    src/engine/ai/npc_advanced/goap_enhanced.c
    src/engine/ai/npc_advanced/goap_planner_enhanced.c
    src/engine/ai/npc_advanced/memory_system.c
    src/engine/ai/npc/perception_system.c
    src/engine/ai/npc_advanced/behavior_tree_utility.c
    
    # Gameplay Core
    src/engine/gameplay/combat_system_impl.c
    src/engine/gameplay/combat/damage_system.c
    src/engine/gameplay/combat/projectile.c
    src/engine/gameplay/combat/hitbox.c
    src/engine/gameplay/inventory/item_db.c
    src/engine/gameplay/inventory/item.c
    src/engine/gameplay/inventory/inventory.c
    src/engine/gameplay/crafting/crafting_system.c
    src/engine/core/threading/mutex.c
)

# Game Sources
set(GAME_SOURCES
    # Game Logic
    src/game/minecraftv2/game.c
    src/game/minecraftv2/game_hooks.c
    src/game/minecraftv2/minecraft_v2_module.c
    src/game/minecraftv2/tech_crafting_impl.c
    src/game/minecraftv2/api.c
    src/game/minecraftv2/block_registry_global.c
    src/game/minecraftv2/mode.c
    # Game - Block & World
    src/game/minecraftv2/block/block.c
    src/game/minecraftv2/block/block_states.c
    src/game/minecraftv2/block/interaction.c
    src/game/minecraftv2/block/mining.c
    src/game/minecraftv2/block/lighting.c
    src/game/minecraftv2/block/liquid_blocks.c
    src/game/minecraftv2/block/falling_blocks.c
    src/game/minecraftv2/block/water_physics.c
    src/game/minecraftv2/block/water_bucket.c
    src/game/minecraftv2/chunk/chunk.c
    src/game/minecraftv2/player/player.c
    src/game/minecraftv2/player/player_movement_physics.c
    src/game/minecraftv2/player/player_combat.c
    src/game/minecraftv2/player/player_damage.c
    src/game/minecraftv2/player/player_food.c
    src/game/minecraftv2/player/player_magic.c
    src/game/minecraftv2/player/player_system_update.c
    src/game/minecraftv2/player/player_vehicle.c
    src/game/minecraftv2/player/spirit_model.c
    src/game/minecraftv2/player/status_effects.c
    src/game/minecraftv2/chunk/chunk_buffers.c
    src/game/minecraftv2/chunk/chunk_manager_optimized.c
    src/game/minecraftv2/world/generator.c
    src/game/minecraftv2/world/generator_basic.c
    src/game/minecraftv2/world/noise.c
    src/game/minecraftv2/world/biome_system.c
    src/game/minecraftv2/world/biome_transitions.c
    src/game/minecraftv2/world/biome_generator.c
    src/game/minecraftv2/world/biome_features.c
    src/game/minecraftv2/world/cave_generation.c
    src/game/minecraftv2/world/loot_tables.c
    src/game/minecraftv2/world/plant_varieties.c
    src/game/minecraftv2/world/plant_vfx.c
    src/game/minecraftv2/world/rail_system.c
    src/game/minecraftv2/world/voxel_world_data.c
    src/game/minecraftv2/ore/ore_generation.c
    src/game/minecraftv2/world/water_integration.c
    src/game/minecraftv2/world/water_physics.c
    src/game/minecraftv2/world/water_system.c
    # Game - Weather
    src/game/minecraftv2/weather/weather.c
    src/game/minecraftv2/weather/weather_audio.c
    src/game/minecraftv2/weather/weather_particles.c
    # src/game/minecraftv2/weather/seasonal_system.c
    # src/game/minecraftv2/weather/environmental_effects.c
    # Swift-C Bridge
    # src/engine/platform/engine_bridge.c
    # Game - Combat (TODO: Update to match current ECS API)
    src/game/minecraftv2/combat/combat_animations.c
    # src/game/minecraftv2/combat/combat_system.c
    src/game/minecraftv2/combat/equipment.c
    # src/game/minecraftv2/combat/projectile.c
    src/game/minecraftv2/combat/shield.c
    # Game - World
    src/game/minecraftv2/world/structures.c
    src/game/minecraftv2/world/tree_generation.c
    src/game/minecraftv2/world/tree_varieties.c
    src/game/minecraftv2/world/dungeon_generation.c
    src/game/minecraftv2/world/settlement_generation.c
    src/game/minecraftv2/world/plant_generation.c
    # Game - Crafting & Items
    src/game/minecraftv2/crafting/recipe_system.c
    # TODO: Update these files to use the new recipe_system API
    # src/game/minecraftv2/crafting/crafting_table.c
    # src/game/minecraftv2/crafting/furnace.c
    # src/game/minecraftv2/crafting/resource_processing.c
    # src/game/minecraftv2/crafting/advanced_crafting.c
    # src/game/minecraftv2/crafting/enchanting.c
    # src/game/minecraftv2/crafting/brewing.c
    src/game/minecraftv2/inventory/inventory.c
    src/game/minecraftv2/ui/hotbar.c
    src/game/minecraftv2/inventory/item_registry.c
    # Game - NPC (Temporarily Disabled due to ECS API mismatches - TODO: Refactor)
    # src/game/minecraftv2/npc/dialogue.c
    # src/game/minecraftv2/npc/dialogue_builder.c
    # src/game/minecraftv2/npc/dialogue_manager.c
    # src/game/minecraftv2/npc/dialogue_tree.c
    # src/game/minecraftv2/npc/npc_abilities.c
    # src/game/minecraftv2/npc/npc_combat.c
    # src/game/minecraftv2/npc/npc_combat_behavior.c
    # src/game/minecraftv2/npc/npc_housing.c
    # src/game/minecraftv2/npc/npc_jobs.c
    # src/game/minecraftv2/npc/npc_lod.c
    # src/game/minecraftv2/npc/npc_profiler.c
    # src/game/minecraftv2/npc/npc_schedule.c
    # src/game/minecraftv2/npc/npc_scheduler.c
    # src/game/minecraftv2/npc/npc_serialization.c
    # src/game/minecraftv2/npc/npc_social.c
    # src/game/minecraftv2/npc/npc_spawning.c
    # src/game/minecraftv2/npc/npc_system.c
    # src/game/minecraftv2/npc/npc_visuals.c
    # src/game/minecraftv2/npc/npc_work.c
    # src/game/minecraftv2/npc/pathfinding.c
    # src/game/minecraftv2/npc/villager_trading.c
    # Game - UI
    src/game/minecraftv2/ui/hud_main.c
    src/game/minecraftv2/ui/hud_bars.c
    src/game/minecraftv2/ui/hud_overlay.c
    src/game/minecraftv2/ui/hud_logging.c
    src/game/minecraftv2/ui/hud_debug.c
    src/game/minecraftv2/ui/hud_renderer.c
    src/game/minecraftv2/ui/floating_text.c
    src/game/minecraftv2/ui/menu.c
    src/game/minecraftv2/ui/menu_renderer.c
    src/game/minecraftv2/ui/main_menu.c
    src/game/minecraftv2/ui/pause_menu.c
    src/game/minecraftv2/ui/crafting_ui.c
    src/game/minecraftv2/ui/inventory_ui.c
    # Game - Tech
    src/game/minecraftv2/tech/solar_energy.c
    src/game/minecraftv2/tech/energy_system.c
    src/game/minecraftv2/tech/tech_machines.c
    # Game - Save
    src/game/minecraftv2/save/save.c
    # Monolithic main - to be refactored
    src/engine/core/engine_integration.c
    src/engine/core/gamestate_main.c
)


# --- Vulkan ---
find_package(Vulkan)
if(Vulkan_FOUND)
    list(APPEND ENGINE_SOURCES
        src/engine/renderer/vulkan/vulkan.c
        src/engine/renderer/vulkan/vulkan_buffers.c
        src/engine/renderer/vulkan/vulkan_surface.c
        src/engine/renderer/vulkan/vulkan_framebuffer.c
        src/engine/renderer/vulkan/vulkan_pipeline.c
        src/engine/renderer/vulkan/vulkan_render.c
        src/engine/renderer/vulkan/vulkan_capabilities.c
        src/engine/renderer/vulkan/vulkan_image.c
        src/engine/renderer/vulkan/vulkan_math.c
        src/engine/renderer/vulkan/vulkan_streaming.c
        # Phase 1: Memory
        src/engine/core/vulkan_memory.c
        src/engine/core/vma_integration.c
        # Phase 2: Dispatch
        src/engine/renderer/vulkan/vulkan_dispatch.c
        # Phase 3: Dynamic Rendering
        src/engine/renderer/vulkan/dynamic_rendering.c
        # Phase 4: MoltenVK
        src/engine/renderer/vulkan/moltenvk_config.c
        src/engine/renderer/vulkan/ray_query.c
        # Phase 5: Async Compute
        src/engine/renderer/vulkan/queue_ownership.c
    )
    add_definitions(-DVULKAN_BUILD)
    include_directories(${Vulkan_INCLUDE_DIRS})
    set(VULKAN_LIBS ${Vulkan_LIBRARIES})
else()
    message(STATUS "Vulkan not found, building without Vulkan renderer")
endif()

# --- Libraries ---
# TODO: Medium - Implement a more robust build options system (e.g., for enabling/disabling features like Vulkan, debugging, etc.).
add_library(Engine STATIC ${ENGINE_SOURCES})
target_include_directories(Engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include_private>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/engine/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/game/minecraftv2/include>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(Engine PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)

# SIMD Math Library Platform-Specific Sources
# Detect platform and add appropriate SIMD implementation
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(X86)|(amd64)|(AMD64)")
    # x86/x86_64 platform
    message(STATUS "Detected x86/x86_64 platform - enabling SSE2/AVX2 SIMD")
    
    # Check for AVX2 support
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
    
    if(COMPILER_SUPPORTS_AVX2)
        message(STATUS "AVX2 support detected, compiling with AVX2 optimizations")
        set(SIMD_SOURCE src/engine/core/simd/simd_math_x86.c)
        set_source_files_properties(${SIMD_SOURCE} PROPERTIES COMPILE_FLAGS "-mavx2 -mfma")
        target_compile_definitions(Engine PRIVATE SIMD_AVX2)
    else()
        message(STATUS "Compiling with SSE2 optimizations")
        set(SIMD_SOURCE src/engine/core/simd/simd_math_x86.c)
        set_source_files_properties(${SIMD_SOURCE} PROPERTIES COMPILE_FLAGS "-msse2")
        target_compile_definitions(Engine PRIVATE SIMD_SSE2)
    endif()
    
    target_sources(Engine PRIVATE ${SIMD_SOURCE})
    
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    # ARM platform
    message(STATUS "Detected ARM platform - enabling NEON SIMD")
    set(SIMD_SOURCE src/engine/core/simd/simd_math_neon.c)
    
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR APPLE)
        # AArch64/Apple Silicon has NEON by default, no -mfpu flag needed
        message(STATUS "ARM64/Apple Silicon detected, NEON enabled by default")
    else()
        # ARMv7 need explicit flag
        message(STATUS "ARMv7 detected, enabling -mfpu=neon")
        set_source_files_properties(${SIMD_SOURCE} PROPERTIES COMPILE_FLAGS "-mfpu=neon")
    endif()
    
    target_compile_definitions(Engine PRIVATE SIMD_NEON)
    target_sources(Engine PRIVATE ${SIMD_SOURCE})
    
else()
    message(STATUS "Unknown platform, using scalar SIMD fallback")
endif()


# Link GLFW if found
if(GLFW3_FOUND)
    target_link_libraries(Engine PUBLIC ${GLFW3_LIBRARIES})
elseif(glfw3_FOUND)
    if(TARGET glfw3::glfw)
        target_link_libraries(Engine PUBLIC glfw3::glfw)
    elseif(TARGET glfw)
        target_link_libraries(Engine PUBLIC glfw)
    else()
        target_link_libraries(Engine PUBLIC glfw3)
    endif()
endif()

if(VULKAN_LIBS)
    target_link_libraries(Engine PUBLIC ${VULKAN_LIBS})
endif()

# Link OpenGL framework on macOS
if(APPLE)
    target_link_libraries(Engine PUBLIC "-framework OpenGL" "-framework CoreFoundation" "-framework CoreServices")
endif()

# Find ZLIB and Threads before they're needed
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

add_library(Game STATIC ${GAME_SOURCES})
target_link_libraries(Game PUBLIC Engine ZLIB::ZLIB)
target_include_directories(Game PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/game/minecraftv2/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/engine/include>
)

# --- App Executable ---
if(APPLE AND IOS)
    set(APP_SOURCES
        App/Sources/AppDelegate.swift
        App/Sources/SceneDelegate.swift
        App/Sources/GameViewController.swift
        App/Sources/MainMenuViewController.swift
        App/Sources/SettingsViewController.swift
        App/Sources/AssetEditorViewController.swift
        App/Sources/Minecraft-Bridging-Header.h
    )
    add_executable(MinecraftApp MACOSX_BUNDLE ${APP_SOURCES})
    set_target_properties(MinecraftApp PROPERTIES
        BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/App/Resources/Info.plist
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.benjaminrussell.MinecraftV2"
        XCODE_ATTRIBUTE_INFOPLIST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/App/Resources/Info.plist
        XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/App/Sources/Minecraft-Bridging-Header.h
        XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone, iPad
    )

if(APPLE)
    target_link_libraries(MinecraftApp PRIVATE Game ZLIB::ZLIB Threads::Threads "-framework UIKit" "-framework SwiftUI" "-framework Metal" "-framework MetalKit")
else()
    target_link_libraries(MinecraftApp PRIVATE Game Engine ZLIB::ZLIB Threads::Threads)
endif()
endif()

# Placeholder for other platforms (e.g. macOS Desktop)
if(NOT IOS)
    add_executable(MinecraftApp src/engine/core/main.c)
    target_link_libraries(MinecraftApp PRIVATE Game Engine ZLIB::ZLIB Threads::Threads "-framework Metal" "-framework MetalKit" "-framework Cocoa" "-framework IOKit" "-framework QuartzCore")
endif()

# ===========================================
# Demo Applications (Added 2026-01-04)
# ===========================================

# Physics Sandbox Demo
add_executable(demo_sandbox src/game/demo_physics_sandbox.c
    src/engine/core/logger.c
    src/engine/core/memory.c
    # Engine sources are linked via the Engine library
)
if(APPLE)
    target_link_libraries(demo_sandbox PRIVATE Engine glfw "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
else()
    target_link_libraries(demo_sandbox PRIVATE Engine glfw)
endif()
# Simple Terrain Demo
add_executable(demo_terrain src/game/demo_simple_terrain.c
    src/engine/core/logger.c
    src/engine/core/memory.c
    src/engine/core/window.c
    src/engine/core/utils.c
    src/engine/core/string_utils.c
)

if(APPLE)
    target_link_libraries(demo_terrain PRIVATE glfw "-framework OpenGL" "-framework Cocoa" "-framework IOKit")
endif()

message(STATUS "Demo applications configured: demo_sandbox, demo_terrain")

# Unit Tests
# add_subdirectory(src/engine/test)

# Unit Tests with Catch2 (Phase 1)
# TODO: Resolve CXX compiler feature detection issue
# if(EXISTS "${CMAKE_SOURCE_DIR}/tests/unit/CMakeLists.txt")
#     add_subdirectory(tests/unit)
# endif()

# add_executable(test_asset_instances test/test_asset_instances.c)

# target_link_libraries(test_asset_instances Engine)
# target_include_directories(test_asset_instances PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/engine/include
#     ${CMAKE_SOURCE_DIR}/src/engine
# )

# ===========================================
# Unit Tests (Temporarily disabled due to missing files)
# ===========================================
# enable_testing()

# PHASE 1: Integrate Catch2 testing framework
if(EXISTS "${CMAKE_SOURCE_DIR}/Vendor/Catch2/CMakeLists.txt")
    message(STATUS "Catch2 found, enabling modern unit tests")
    add_subdirectory(Vendor/Catch2)
    
    # Phase 1 Build System Tests
    # add_executable(test_phase1_build_system tests/unit/test_phase1_build_system.cpp)
    # target_link_libraries(test_phase1_build_system PRIVATE Engine Catch2::Catch2WithMain)
    # target_include_directories(test_phase1_build_system PRIVATE
    #     ${CMAKE_SOURCE_DIR}/src/engine/include
    #     ${CMAKE_SOURCE_DIR}/src/engine
    # )
    # add_test(NAME Phase1_BuildSystem COMMAND test_phase1_build_system)
else()
    message(WARNING "Catch2 not found in Vendor/Catch2")
endif()

# Helper macro to add test
macro(add_engine_test name source)
    # add_executable(${name} ${source} ${CMAKE_SOURCE_DIR}/tests/test_framework_unified.c)
    # target_link_libraries(${name} PRIVATE Engine)
    # target_include_directories(${name} PRIVATE
    #     ${CMAKE_SOURCE_DIR}/src/engine/include
    #     ${CMAKE_SOURCE_DIR}/src/engine
    #     ${CMAKE_SOURCE_DIR}/test
    #     ${CMAKE_SOURCE_DIR}/tests
    # )
    # add_test(NAME ${name} COMMAND ${name})
endmacro()

# Render Graph Tests
# add_engine_test(test_render_graph test/test_render_graph.c)

# SIMD Math Tests (Physics DOD Foundation - Phase 1)
# add_engine_test(test_simd_math tests/test_simd_math.c)

# SoA Container Tests (Physics DOD - Phase 2)
# add_engine_test(test_soa_container tests/test_soa_container.c)

# Physics Integration Tests & Benchmark (Physics DOD - Phase 2)
# add_engine_test(test_physics_integration tests/test_physics_integration.c)

# AABB Tree Tests (Physics DOD - Phase 3)
# add_engine_test(test_aabb_tree tests/test_aabb_tree.c)

# GJK/EPA Tests (Physics DOD - Phase 4)
# add_engine_test(test_collision_gjk_epa tests/test_collision_gjk_epa.c)

# Material System Tests
# add_engine_test(test_material_system test/test_material_system.c)

# Post-Processing Tests
# add_engine_test(test_post_processing test/test_post_processing.c)

# ===========================================
# Unit Tests - AI Subsystem
# ===========================================
# add_engine_test(test_ai_command_processor tests/unit/ai/test_ai_command_processor.c)
# add_engine_test(test_content_manager tests/unit/ai/test_content_manager.c)
# add_engine_test(test_perception_system tests/unit/ai/test_perception_system.c)
# add_engine_test(test_utility_ai tests/unit/ai/test_utility_ai.c)
# add_engine_test(test_goap_enhanced tests/unit/ai/test_goap_enhanced.c)
# add_engine_test(test_memory_system tests/unit/ai/test_memory_system.c)

# ===========================================
# Unit Tests - Audio Subsystem
# ===========================================
# add_engine_test(test_audio_system tests/unit/audio/test_audio_system.c)
# add_engine_test(test_audio_manager tests/unit/audio/test_audio_manager.c)

# ===========================================
# Unit Tests - Core Subsystem
# ===========================================
# add_engine_test(test_logger tests/unit/core/test_logger.c)
# add_engine_test(test_memory tests/unit/core/test_memory.c)
# add_engine_test(test_string_utils tests/unit/core/test_string_utils.c)
# add_engine_test(test_cache tests/unit/core/test_cache.c)
# add_engine_test(test_hashmap tests/unit/core/test_hashmap.c)

# ===========================================
# Unit Tests - ECS Subsystem
# ===========================================
# add_engine_test(test_ecs tests/unit/ecs/test_ecs.c)

# ===========================================
# Unit Tests - Math Subsystem
# ===========================================
# add_engine_test(test_vec3 tests/unit/math/test_vec3.c)
# add_engine_test(test_mat4 tests/unit/math/test_mat4.c)
# add_engine_test(test_quat tests/unit/math/test_quat.c)

# ===========================================
# Physics Verification Protocols
# AAA-Grade "Proof of Life" Tests
# ===========================================
# add_engine_test(test_protocol1_energy tests/physics_verification/test_protocol1_energy.c)
# add_engine_test(test_protocol2_stacking tests/physics_verification/test_protocol2_stacking.c)
# add_engine_test(test_protocol3_friction tests/physics_verification/test_protocol3_friction.c)
# add_engine_test(test_protocol4_ccd tests/physics_verification/test_protocol4_ccd.c)

# add_engine_test(test_protocol5_determinism tests/physics_verification/test_protocol5_determinism.c)

# ===========================================
# Modern Architecture Tests (Phase 6)
# ===========================================
# add_engine_test(test_vulkan_memory tests/unit/core/test_vulkan_memory.c)
# add_engine_test(test_dynamic_rendering tests/unit/renderer/test_dynamic_rendering.c)
# add_engine_test(test_moltenvk tests/unit/renderer/test_moltenvk.c)
# add_engine_test(test_async_compute tests/unit/renderer/test_async_compute.c)






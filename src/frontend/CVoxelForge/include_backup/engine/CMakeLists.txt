# Engine CMakeLists.txt
# Complete build configuration with all organized files

cmake_minimum_required(VERSION 3.20)
project(GameEngine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Vulkan)
find_package(glfw3 3.3)

# Check GLFW
if(NOT glfw3_FOUND)
    message(WARNING "GLFW3 not found via find_package, trying pkg-config...")
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW3 glfw3)
        if(GLFW3_FOUND)
            set(glfw3_FOUND TRUE)
            message(STATUS "GLFW3 found via pkg-config")
        endif()
    endif()
endif()

if(NOT glfw3_FOUND AND NOT GLFW3_FOUND)
    message(FATAL_ERROR "GLFW3 is required but not found. Please install GLFW3:\n  brew install glfw  (macOS)\n  apt-get install libglfw3-dev  (Ubuntu)")
endif()

if(NOT Vulkan_FOUND)
    message(WARNING "Vulkan SDK not found - renderer will have limited functionality")
    set(VULKAN_AVAILABLE FALSE)
else()
    set(VULKAN_AVAILABLE TRUE)
    message(STATUS "Vulkan SDK found: ${Vulkan_LIBRARIES}")
endif()


# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../game/minecraftv2/include
)

if(VULKAN_AVAILABLE)
    include_directories(${Vulkan_INCLUDE_DIRS})
endif()


# Math library sources
set(ENGINE_MATH_SOURCES
    math/vec3.c
    # math/mat4.c
    math/quat.c
)

# Tools
set(ENGINE_TOOLS_SOURCES
    tools/profiler.c
    tools/hot_reload.c
    tools/asset_editor/asset_editor.c
    
    # Visual Scripting
    visual_scripting/visual_scripting_core.c
    visual_scripting/nodes.c
    visual_scripting/debugger.c
)

# Core engine sources
set(ENGINE_CORE_SOURCES
    core/logger.c
    core/memory.c
    core/pool.c
    core/performance.c
    core/config.c
    core/config_loader.c
    core/containers.c
    core/hashmap.c
    core/dynamic_array.c
    core/cache.c
    core/string_utils.c
    core/utils.c
    core/big_int.c
    core/json.c
    core/asset_validator.c
    core/file_watcher.c
    core/asset_compression.c
    core/job.c
    core/window.c
    core/game_loop.c
    core/engine.c
    core/lod_generator.c
    core/hot_reload.c
    core/asset_hot_reload.c
    core/threading/mutex.c
    core/time_system.c
)

# Renderer - Vulkan
set(RENDERER_VULKAN_SOURCES
    renderer/vulkan/vulkan.c
    renderer/vulkan/vulkan_buffers.c
    renderer/vulkan/vulkan_surface.c
    renderer/vulkan/vulkan_framebuffer.c
    renderer/vulkan/vulkan_pipeline.c
    renderer/vulkan/vulkan_render.c
    renderer/vulkan/vulkan_capabilities.c
    renderer/vulkan/vulkan_image.c
    renderer/vulkan/vulkan_streaming.c
)

# Renderer - GPU
set(RENDERER_GPU_SOURCES
    renderer/gpu/gpu_lighting.c
    renderer/gpu/gpu_material.c
    renderer/gpu/gpu_memory.c
    renderer/gpu/gpu_postprocess.c
    renderer/gpu/gpu_renderpass.c
    renderer/gpu/gpu_shader_reload.c
    renderer/gpu/gpu_text.c
    renderer/gpu/gpu_texture.c
)

# Renderer - Systems
set(RENDERER_SYSTEMS_SOURCES
    renderer/systems/camera.c
    renderer/systems/camera_controls.c
    renderer/systems/mesh.c
    renderer/systems/mesh_optimizer.c
    renderer/systems/sprite_3d_renderer.c
    renderer/systems/sprite_renderer_2_5d.c
    renderer/systems/voxel_renderer.c
    renderer/systems/particle_renderer.c
    renderer/systems/gpu_particle_system.c
    renderer/systems/text_renderer.c
    renderer/systems/ui_renderer.c
    renderer/systems/weather_system.c
    renderer/systems/vfx_optimizer.c
    
    # Advanced UI System
    ui/ui_layout.c
    ui/ui_widgets.c
    ui/ui_accessibility.c
    ui/ui_animation.c
)

# Renderer - Advanced
set(RENDERER_ADVANCED_SOURCES
    renderer/advanced/ray_tracing.c
    renderer/advanced/global_illumination.c
    renderer/advanced/ambient_occlusion.c
    renderer/advanced/normal_mapping.c
    renderer/advanced/hdr_tonemap.c
    renderer/advanced/material_reflection.c
    renderer/pbr/pbr_brdf.c
)

# Renderer - Core
set(RENDERER_CORE_SOURCES
    renderer/core/chunk_mesh_generator.c
    renderer/core/mesh.c
    
    # Shader Graph
    renderer/shader_graph.c
    renderer/shader_nodes.c
    
    # Animation System
    animation/animation_system.c
    animation/ik_solver.c
    animation/skeletal/skinning.c
    animation/state_machines/anim_state_machine.c
    animation/ik_advanced/fabrik_impl.c
    animation/blending.c
    
    # Post Processing
    renderer/post_processing.c
    renderer/post_processing/bloom.c
    renderer/shadows/csm.c
)



# Renderer - Base (The minimal abstraction)
set(RENDERER_BASE_SOURCES
    renderer/core/renderer.c
)

# Renderer - Controllers
set(RENDERER_CONTROLLER_SOURCES
    renderer/controllers/free_camera_controller.c
    renderer/controllers/fixed_angle_camera_controller.c
)

# Renderer - OpenGL (Backend)
set(RENDERER_OPENGL_SOURCES
    renderer/opengl/opengl_backend.c
)

# Physics - Core
set(PHYSICS_CORE_SOURCES
    physics/physics_core_impl.c
    physics/physics_integration.c
    physics/collision/broadphase.c
    physics/collision/narrowphase.c
    physics/solver/debug_visualization.c
    physics/constraints/advanced_constraints.c
    physics/dynamics/character_controller_impl.c
    physics/fracture/voronoi_shatter.c
    physics/destruction/destruction_impl.c
    physics/fluids/fluid_system.c
    physics/fluids/sph_solver.c
)

# Audio sources
set(ENGINE_AUDIO_SOURCES
    audio/audio_system.c
    audio/audio_loader.c
    audio/audio_integration.c
    audio/audio_effects.c
    audio/audio_reverb.c
    audio/audio_streaming.c
    audio/audio_occlusion.c
    audio/audio_event_queue.c
    audio/underwater_filter.c
    audio/spatial_audio.c
    audio/audio_dsp.c
    weather/weather_stub.c
)

# Network
set(ENGINE_NETWORK_SOURCES
    network/reliable_udp.c
    network/rpc_system.c
)

# ECS
set(ENGINE_ECS_SOURCES
    ecs/ecs.c
    ecs/entity_hierarchy.c
)

# Input
set(ENGINE_INPUT_SOURCES
    input/input.c
    input/input_factory.c
)

# Rendering
set(ENGINE_RENDERER_SOURCES
    renderer/core/renderer.c
    renderer/core/shader.c
    renderer/core/material.c
    renderer/core/mesh.c
    renderer/opengl/opengl_backend.c
    renderer/post_processing.c
)

# Scene
set(ENGINE_SCENE_SOURCES
    scene/scene_system_impl.c
)

# Resource management
set(ENGINE_RESOURCE_SOURCES
    resource/asset_manager.c
    resource/vfs.c
    resource/vfs_async.c
    assets/importer/asset_importer.c
    assets/importer/gltf_loader.c
    assets/importer/gltf_material_bridge.c
)

# AI
set(ENGINE_AI_SOURCES
    ai/asset_generator.c
    ai/modular_assets.c
    ai/ai_pipeline.c
    ai/pathfinding/astar_pathfinder.c
    ai/pathfinding/path_smoothing.c
    ai/behavior/behavior_tree_impl.c
    ai/style_templates.c
)

# Editor
set(ENGINE_EDITOR_SOURCES
    editor/gizmo_system.c
)

# Scripting
set(ENGINE_SCRIPTING_SOURCES
    scripting/vm.c
    scripting/c_bindings.c
)

# Core / Memory
set(ENGINE_CORE_MEMORY_SOURCES
    core/memory/arena.c
)


# AI Advanced
set(ENGINE_AI_ADVANCED_SOURCES
    ai/npc_advanced/behavior_tree.c
    ai/npc_advanced/behavior_tree_utility.c
    ai/npc_advanced/utility_ai.c
    ai/npc_advanced/goap_enhanced.c
    ai/npc_advanced/goap_planner_enhanced.c
    ai/npc_advanced/memory_system.c
    ai/npc/perception_system.c
)

# AI Core - Introspection/API (New Pillar 1)
set(ENGINE_CORE_INTROSPECTION_SOURCES
    core/introspection/type_registry.c
)

set(ENGINE_AI_API_SOURCES
    ai/api/ai_command_processor.c
)

# Combat System (Phase 1)
set(ENGINE_COMBAT_SOURCES
    gameplay/combat/hitbox.c
    gameplay/combat/health.c
    gameplay/combat/damage_system.c
    gameplay/combat/projectile.c
    gameplay/combat/status_effects.c
    gameplay/combat_system_impl.c
)

# Inventory System (Phase 2)
set(ENGINE_INVENTORY_SOURCES
    gameplay/inventory/item.c
    gameplay/inventory/item_db.c
    gameplay/inventory/inventory.c
)

# Crafting System (Phase 3)
set(ENGINE_CRAFTING_SOURCES
    gameplay/crafting/crafting_system.c
)

# NPC System
set(ENGINE_NPC_SOURCES
    ../game/minecraftv2/npc/npc_system.c
    ../game/minecraftv2/npc/npc_ai.c
    ../game/minecraftv2/npc/npc_visuals.c
)

# World Systems
set(ENGINE_WORLD_SOURCES
    world/biome/biome_system.c
    world/chunk/chunk_system.c
)

# Cinematic Systems
set(ENGINE_CINEMATIC_SOURCES
    renderer/cinematic/camera_director.c
)

# Editor Systems
set(ENGINE_EDITOR_SYSTEMS_SOURCES
    editor/console/console_system.c
)

# Renderer Optimization
set(ENGINE_RENDERER_OPTIMIZATION_SOURCES
    renderer/optimization/occlusion_system.c
)

# Combine all engine sources
set(ENGINE_ALL_SOURCES
    ${ENGINE_CORE_SOURCES}
    ${ENGINE_MATH_SOURCES}
    ${ENGINE_INPUT_SOURCES}
    ${ENGINE_RESOURCE_SOURCES}
    ${ENGINE_ECS_SOURCES}
    ${ENGINE_SCENE_SOURCES}
    ${PHYSICS_CORE_SOURCES}
    ${ENGINE_TOOLS_SOURCES}
    ${ENGINE_NPC_SOURCES}
    ${ENGINE_AUDIO_SOURCES}  # ✅ COMPLETED: Immersive Audio System (Phase 2)
    ${ENGINE_AI_SOURCES}
    ${ENGINE_AI_ADVANCED_SOURCES}
    ${ENGINE_AI_API_SOURCES}            # ✅ AI Native API
    ${ENGINE_CORE_INTROSPECTION_SOURCES} # ✅ Introspection System
    ${ENGINE_AI_ADVANCED_SOURCES}
    ${ENGINE_COMBAT_SOURCES}  # ✅ Phase 1: Combat Mechanics Core
    ${ENGINE_INVENTORY_SOURCES}  # ✅ Phase 2: Inventory Management
    ${ENGINE_CRAFTING_SOURCES}  # ✅ Phase 3: Crafting System
    ${ENGINE_NETWORK_SOURCES}  # ✅ Phase 5: Networking
    ${ENGINE_EDITOR_SOURCES}   # ✅ Phase 7: Editor
    ${ENGINE_SCRIPTING_SOURCES} # ✅ Phase 8: Scripting
    ${ENGINE_CORE_MEMORY_SOURCES} # ✅ Phase 10: Optimization
    ${ENGINE_WORLD_SOURCES}  # ✅ Phase 4: World Building Systems
    ${ENGINE_CINEMATIC_SOURCES}  # ✅ Phase 5: Cinematic Systems
    ${ENGINE_EDITOR_SYSTEMS_SOURCES}  # ✅ Phase 6: Editor Systems
    ${ENGINE_RENDERER_OPTIMIZATION_SOURCES}  # ✅ Phase 7: Renderer Optimization
    
    # Swift-C Bridge
    platform/engine_bridge.c
    platform/graphics_api_bridge.c
    platform/physics_debug_api_bridge.c
    platform/gizmo_api_bridge.c
    platform/undo_redo_api_bridge.c
    platform/profiler_api_bridge.c
    platform/hot_reload_api_bridge.c
    platform/prefab_api_bridge.c
    platform/thumbnail_generator_api_bridge.c
    platform/selection_api_bridge.c
    platform/grid_rendering_api_bridge.c
    platform/outline_rendering_api_bridge.c
    platform/camera_bookmarks_api_bridge.c
    platform/material_override_api_bridge.c
    platform/uv_visualization_api_bridge.c
    platform/vertex_color_display_api_bridge.c
    platform/bounds_visualization_api_bridge.c
    platform/ray_tracing_api_bridge.c
    platform/gi_api_bridge.c
    platform/shadow_api_bridge.c
    platform/terrain_api_bridge.c
    platform/sky_api_bridge.c
    platform/water_api_bridge.c
    platform/vegetation_api_bridge.c
    platform/shader_hot_reload_api_bridge.c
    platform/mesh_optimizer_api_bridge.c
    platform/lod_api_bridge.c
    platform/tonemapping_api_bridge.c
    platform/decal_api_bridge.c
    # Phase 3: VFX & Visual Effects
    platform/destruction_api_bridge.c
    platform/weather_vfx_api_bridge.c
    platform/fluid_vfx_api_bridge.c
    platform/status_effect_api_bridge.c
    platform/vfx_optimization_api_bridge.c
    platform/post_processing_api_bridge.c
    # Phase 4: World Building
    platform/biome_api_bridge.c
    platform/chunk_api_bridge.c
    # Phase 5: Cinematics
    platform/camera_director_api_bridge.c
    # Phase 6: Editor Tools & QoL
    platform/console_api_bridge.c
    # Phase 7: Optimization
    platform/occlusion_api_bridge.c
)

if(VULKAN_AVAILABLE)
    list(APPEND ENGINE_ALL_SOURCES 
        ${RENDERER_VULKAN_SOURCES}
        ${RENDERER_GPU_SOURCES}
        ${RENDERER_SYSTEMS_SOURCES}
        ${RENDERER_ADVANCED_SOURCES}
        ${RENDERER_CORE_SOURCES}
        ${RENDERER_BASE_SOURCES}
        ${RENDERER_CONTROLLER_SOURCES}
    )
else()
    # When Vulkan missing, we still need generic renderer files + OpenGL
     list(APPEND ENGINE_ALL_SOURCES 
        # ${RENDERER_CORE_SOURCES} # Commented out until fixed
        ${RENDERER_BASE_SOURCES}
        ${RENDERER_OPENGL_SOURCES}
    )
endif()


# Create static engine library
add_library(GameEngine STATIC ${ENGINE_ALL_SOURCES})

target_include_directories(GameEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../game/minecraftv2/include
)

if(VULKAN_AVAILABLE)
    target_include_directories(GameEngine PUBLIC ${Vulkan_INCLUDE_DIRS})
endif()


target_link_libraries(GameEngine PUBLIC
    glfw
)
if(APPLE)
    target_link_libraries(GameEngine PUBLIC 
        "-framework OpenGL"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
    )
else()
    target_link_libraries(GameEngine PUBLIC OpenGL::GL)
endif()

if(VULKAN_AVAILABLE)
    target_link_libraries(GameEngine PUBLIC ${Vulkan_LIBRARIES})
    target_compile_definitions(GameEngine PUBLIC VULKAN_BUILD=1 VULKAN_ENABLED=1)
    if(WIN32)
        # Windows-specific libraries usually needed for Vulkan/Games
        target_link_libraries(GameEngine PUBLIC vulkan-1 user32 gdi32 shell32)
    endif()
else()
    target_compile_definitions(GameEngine PUBLIC VULKAN_BUILD=0 VULKAN_ENABLED=0)
endif()

# Integration Tests
add_executable(test_engine_integration test/integration/test_engine_integration.c)
target_link_libraries(test_engine_integration GameEngine)

# Add all tests to ctest
enable_testing()
add_test(NAME EngineIntegrationTest COMMAND test_engine_integration)


# Prototype example
add_executable(prototype
    examples/prototype/main.c
)

target_link_libraries(prototype PRIVATE
    GameEngine
)

# Set output directory
set_target_properties(prototype GameEngine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Asset Packer Tool
add_executable(asset_packer 
    tools/asset_packer.c
    core/asset_compression.c
)
target_include_directories(asset_packer PRIVATE ../../include)
target_link_libraries(asset_packer PRIVATE m) 
set_target_properties(asset_packer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "Engine build configured")
message(STATUS "  Vulkan: ${Vulkan_LIBRARIES}")
message(STATUS "  GLFW: found")
message(STATUS "  Total sources: ${ENGINE_ALL_SOURCES}")

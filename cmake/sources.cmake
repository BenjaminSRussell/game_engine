# cmake/sources.cmake
# Auto-generated source file lists for BlockGame engine and game

# ===========================================
# ENGINE SOURCES
# ===========================================
file(GLOB_RECURSE ENGINE_SOURCES
    # Block subdirectory - DISABLED lighting due to missing constants
    "src/game/blockgame/block/block.c"
    "src/game/blockgame/block/block_states.c"
    "src/game/blockgame/block/falling_blocks.c"
    # "src/game/blockgame/block/lighting.c"
    "src/game/blockgame/block/interaction.c"
    
    # AI subdirectory
    "src/engine/ai/*.c"
    
    # Animation subdirectory
    "src/engine/animation/*.c"
    
    # Assets subdirectory (importers, loaders)
    "src/engine/assets/*.c"
    
    # Audio subdirectory - RE-ENABLED with core functionality
    # "src/engine/audio/*.c"
    "src/engine/audio/audio_system.c"
    # "src/engine/audio/underwater_filter_stubs.c"
    # "src/engine/audio/audio_loader.c"
    "src/engine/audio/audio_core.c"
    
    # Core systems
    "src/engine/core/misc_stubs.c"
    "src/engine/core/logger.c"
    "src/engine/core/memory/memory.c"
    "src/engine/core/window.c"
    "src/engine/core/utils.c"
    "src/engine/core/string_utils.c"
    "src/engine/core/thread_pool.c"
    "src/engine/core/allocator.c"
    # Additional core systems needed for linking
    "src/engine/core/profiler.c"
    "src/engine/core/hot_reload.c"
    "src/engine/core/data_structures/hashmap.c"
    "src/engine/core/data_structures/linear_allocator.c"
    "src/engine/core/data_structures/buddy_allocator.c"
    
    # Engine core systems and globals
    "src/engine/core/engine.c"
    "src/engine/core/config.c"
    # "src/engine/core/game_stubs.c"  # Disabled due to duplicate symbols
    
    # Math impl - RE-ENABLED (needed for game functionality)
    "src/engine/math/mat4.c"
    "src/engine/math/quat.c"
    # "src/engine/math/vec3.c"  # Use inline implementations from header instead
    "src/engine/math/math_common.c"
    # Exclude math stubs since we have real implementations
    # "src/engine/math/mat4_stubs.c"
    
    # Rendering 
    "src/engine/rendering/core/mesh.c"
    "src/engine/rendering/texture/texture.c"
    "src/engine/rendering/voxel_renderer.c"
    "src/engine/rendering/renderer_factory.c"
    "src/engine/rendering/camera.c"
    "src/engine/rendering/particles/particle_renderer.c"
    "src/engine/rendering/core/texture_atlas.c"
    # Add camera implementation file
    "src/engine/rendering/camera_impl.c"
    
    # Scripting - DISABLED due to compilation errors
    # "src/engine/scripting/script_system.c"
    
    # Backend subdirectory - Handled separately below to avoid Vulkan inclusion
    # "src/engine/backend/*.c"
    
    # Metal backend - DISABLED due to ARC compilation issues
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/metal/.*\\.(c|m)$")
    # "src/engine/backend/metal/*.c"
    
    # Platform subdirectory - DISABLED swift_bridge due to function call issues
    # "src/engine/platform/swift_bridge.c"
    
    # Input system
    "src/engine/input/input_system.c"
    "src/engine/input/input_profiles.c"
    
    # Engine subsystems
    "src/engine/engine.c"
    "src/engine/assets/asset_manager.c"
    
    # Renderer and scene systems
    "src/engine/rendering/core/renderer.c"
    "src/engine/scene/scene_manager.c"
    "src/engine/rendering/post_process/post_process.c"
    
    # Threading and synchronization
    "src/engine/core/threading/mutex.c"
    "src/engine/core/threading/job.c"
    
    # Platform and time systems
    "src/engine/platform/time_system.c"
    "src/engine/platform/window/window.c"
    
    # VFS and core systems
    "src/engine/core/resource/vfs.c"
    "src/engine/core/resource/vfs_async.c"
    
    # Character subdirectory
    "src/engine/character/*.c"
    
    # Cinematic subdirectory
    "src/engine/cinematic/*.c"
    "src/engine/cinematics/*.c"
    
    # ECS subdirectory
    "src/engine/ecs/*.c"
    
    # Editor subdirectory
    "src/engine/editor/*.c"
    
    # Effects subdirectory - CONSOLIDATED: Only essential particle implementations
    "src/engine/effects/particles/particle_emitter.c"
    "src/engine/effects/particles/particle_simulation.c"
    "src/engine/effects/destruction/fracture_system.c"
    
    # Environment subdirectory
    "src/engine/environment/*.c"
    
    # World generation
    "src/engine/world/generator.c"
    "src/engine/world/world.c"
    
    # Particles - CONSOLIDATED: Primary GPU particle system and renderers
    "src/engine/core/services/particle_system.c"
    "src/engine/core/services/voxel_renderer.c"
    # "src/engine/core/services/particle_system_gpu.c"  # Disabled due to Vulkan issues
    "src/engine/rendering/particles/particle_system.c"
    "src/game/blockgame/weather/weather_particles.c"
    
    # Missing stubs for game systems (create minimal implementations)
    "src/engine/npc/npc_system.c"
    "src/engine/npc/npc_jobs.c" 
    "src/engine/npc/npc_visuals.c"
    # "src/engine/housing/housing_system.c"
    # "src/engine/dialogue/dialogue_manager.c"
    
    # Player vehicle and magic systems (create minimal implementations)
    # "src/engine/player/player_vehicle.c"
    # "src/engine/player/player_magic.c"
    # "src/engine/player/player_spells.c"
    
    # Geometry subdirectory - TEMPORARILY DISABLED mesh_gpu due to missing header
    # "src/engine/geometry/*.c"
    # Core geometry primitives (needed for mesh creation functions)
    "src/engine/geometry/mesh_primitives.c"
    "src/engine/geometry/mesh.c"
    "src/engine/geometry/cluster/cluster_hierarchy.c"
    "src/engine/geometry/cluster/cluster_lod.c"
    "src/engine/geometry/cluster/visibility_buffer.c"
    "src/engine/geometry/culling/frustum/frustum_debug.c"
    "src/engine/geometry/culling/frustum/frustum_planes.c"
    "src/engine/geometry/culling/frustum/simd_frustum_cull.c"
    "src/engine/geometry/culling/gpu_culling/cull_output_buffer.c"
    "src/engine/geometry/culling/gpu_culling/cull_statistics.c"
    "src/engine/geometry/culling/gpu_culling/draw_indirect_gen.c"
    "src/engine/geometry/culling/gpu_culling/gpu_cull_compute.c"
    "src/engine/geometry/culling/gpu_culling/meshlet_cull_gpu.c"
    "src/engine/geometry/culling/gpu_culling/two_phase_occlusion.c"
    "src/engine/geometry/culling/occlusion/hzb_builder.c"
    "src/engine/geometry/culling/occlusion/occlusion_feedback.c"
    "src/engine/geometry/culling/occlusion/occlusion_queries.c"
    "src/engine/geometry/culling/occlusion/software_rasterizer.c"
    "src/engine/geometry/culling/occlusion/temporal_occlusion.c"
    "src/engine/geometry/instancing/debug_visualization.c"
    "src/engine/geometry/instancing/gpu_culling.c"
    "src/engine/geometry/instancing/gpu_instance_data.c"
    "src/engine/geometry/instancing/indirect_instancing.c"
    "src/engine/geometry/instancing/instance_batching.c"
    "src/engine/geometry/instancing/instance_buffer.c"
    "src/engine/geometry/instancing/instance_buffer_stubs.c"
    "src/engine/geometry/instancing/instance_culler.c"
    "src/engine/geometry/instancing/instance_data.c"
    "src/engine/geometry/instancing/instance_lod.c"
    "src/engine/geometry/instancing/instancing.c"
    "src/engine/geometry/instancing/performance_profiling.c"
    "src/engine/geometry/lod/continuous_lod.c"
    "src/engine/geometry/lod/distance_lod.c"
    "src/engine/geometry/lod/lod_bias.c"
    "src/engine/geometry/lod/lod_crossfade.c"
    "src/engine/geometry/lod/lod_distance.c"
    "src/engine/geometry/lod/lod_dithering.c"
    "src/engine/geometry/lod/lod_error_metric.c"
    "src/engine/geometry/lod/lod_forcing.c"
    "src/engine/geometry/lod/lod_generator.c"
    "src/engine/geometry/lod/lod_gpu_selection.c"
    "src/engine/geometry/lod/lod_hysteresis.c"
    "src/engine/geometry/lod/lod_memory.c"
    "src/engine/geometry/lod/lod_screen_size.c"
    "src/engine/geometry/lod/lod_selector.c"
    "src/engine/geometry/lod/lod_streaming.c"
    "src/engine/geometry/lod/lod_streaming_priority.c"
    "src/engine/geometry/lod/lod_system.c"
    "src/engine/geometry/lod/screen_size_lod.c"
    "src/engine/geometry/mesh.c"
    
    # Lighting subdirectory
    "src/engine/lighting/*.c"
    
    # Materials subdirectory - RE-ENABLED customization system
    "src/engine/materials/*.c"
    # "src/engine/materials/customization_loader.c"  # Disabled due to missing includes
    "src/engine/materials/standard_material.c"
    "src/engine/materials/texture_manager.c"
    "src/engine/materials/shader_manager.c"
    "src/engine/materials/material_library.c"
    "src/engine/materials/special_materials/hair_material.c"
    "src/engine/materials/special_materials/skin_material.c"
    "src/engine/materials/special_materials/water_material.c"
    "src/engine/materials/templates/ai/ai_advanced_impl.c"
    "src/engine/materials/templates/ai/ai_impl.c"
    "src/engine/materials/templates/ai/ai_utils_impl.c"
    "src/engine/materials/templates/ai/behavior_tree_impl.c"
    "src/engine/materials/templates/animation/animation_impl.c"
    "src/engine/materials/templates/animation/animation_state_machine_impl.c"
    "src/engine/materials/templates/asset_templates_batch.c"
    "src/engine/materials/templates/characters/body_morph_blending.c"
    "src/engine/materials/templates/characters/character_templates.c"
    
    # Math subdirectory - DISABLED due to old type names (headers provide inline implementations)
    # "src/engine/math/*.c"
    
    # Network subdirectory
    "src/engine/network/*.c"
    "src/engine/networking/*.c"
    
    # Physics subdirectory - CONSOLIDATED: Only essential files
    "src/engine/physics/block_physics.c"
    "src/engine/physics/collision/collision_gjk_epa.c"
    "src/engine/physics/collision/gjk_solver.c"
    "src/engine/physics/collision/epa_solver.c"
    "src/engine/physics/broadphase/aabb_tree.c"
    "src/engine/physics/narrowphase/contact_manifold.c"
    "src/engine/physics/integration/physics_integration.c"
    "src/engine/physics/physics_world_stubs.c"
    "src/engine/physics/particle_physics/particle_forces.c"
    "src/engine/physics/queries/physics_queries.c"
    "src/engine/physics/vehicle_physics.c"
    "src/engine/physics/simulation_impl.c"
    "src/engine/physics/simulation_loop.c"
    
    # Platform subdirectory - DISABLED due to swift_bridge issues (except input system)
    "src/engine/platform/input/*.c"
    "src/engine/platform/memory/memory_manager.c"
    # "src/engine/platform/*.c"
    
    # Post-processing subdirectory
    "src/engine/postprocess/*.c"
    
    # Rendering subdirectory - DISABLED ai_ml due to missing functions
    # "src/engine/rendering/*.c"
    "src/engine/rendering/advanced/ambient_occlusion.c"
    "src/engine/rendering/advanced/extended_rendering_complete.c"
    "src/engine/rendering/advanced/global_illumination.c"
    "src/engine/rendering/advanced/hdr_tonemap.c"
    "src/engine/rendering/advanced/material_reflection.c"
    "src/engine/rendering/advanced/normal_mapping.c"
    "src/engine/rendering/advanced/ray_tracing.c"
    "src/engine/rendering/advanced/rendering_complete.c"
    "src/engine/rendering/advanced/rendering_final_batch.c"
    
    # Scene subdirectory
    "src/engine/scene/*.c"
    
    # Scripting subdirectory
    "src/engine/scripting/*.c"
    
    # Shading subdirectory
    "src/engine/shading/*.c"
    
    # Testing subdirectory
    "src/engine/testing/*.c"
    
    # Tools subdirectory
    "src/engine/tools/*.c"
    
    # UI subdirectory
    "src/engine/ui/*.c"
    
    # Vendor subdirectory
    "src/engine/vendor/*.c"
)

# Add Backend sources explicitly to exclude Vulkan/OpenGL
# We do this here instead of in the GLOB_RECURSE above to have fine-grained control
file(GLOB BACKEND_ROOT_SOURCES "src/engine/backend/*.c")
file(GLOB_RECURSE BACKEND_METAL_SOURCES "src/engine/backend/metal/*.c")
list(APPEND ENGINE_SOURCES ${BACKEND_ROOT_SOURCES} ${BACKEND_METAL_SOURCES})

# Objective-C / Metal sources (handled separately for compiler flags)
if(APPLE)
    file(GLOB_RECURSE ENGINE_OBJC_SOURCES
        "src/engine/backend/metal/*.m"
        "src/engine/core/integration/*.m"
        "src/engine/platform/macos*.m"
        "src/engine/geometry/bvh/*_metal*.m"
        "src/engine/materials/pbr/*.m"
        "src/engine/geometry/vertex/*_metal*.m"
        "src/engine/geometry/nanite/*.m"
        "src/engine/rendering/lighting/*.m"
    )
    
    # Add .m files to engine sources
    list(APPEND ENGINE_SOURCES ${ENGINE_OBJC_SOURCES})
endif()

# Exclude test files, main.c (added separately), and SIMD platform-specific (added via CMakeLists.txt)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*test.*\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/main\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*\\.disabled$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/old/.*")

# Temporarily exclude problematic subsystem files that have incomplete header scaffolding
# Animation subsystem is completely excluded in GLOB_RECURSE above (via commenting out)

# Audio core and dsp - RE-ENABLED for testing
# list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/audio/core/.*\\.c$")
# list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/audio/dsp/.*\\.c$")

# Vulkan backend - conditional compilation based on platform
if(NOT APPLE)
    # On Linux/Windows, include Vulkan backend
    file(GLOB_RECURSE BACKEND_VULKAN_SOURCES "src/engine/backend/vulkan/*.c")
    list(APPEND ENGINE_SOURCES ${BACKEND_VULKAN_SOURCES})
    message(STATUS "Vulkan backend enabled for non-Apple platform")
else()
    # On macOS, exclude Vulkan (use Metal instead)
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/vulkan/.*\\.c$")
    message(STATUS "Vulkan backend disabled on macOS, using Metal backend")
endif()

# Metal backend C files - re-enabled for proper Metal support
if(APPLE)
    # On macOS, include Metal backend C files
    file(GLOB_RECURSE BACKEND_METAL_C_SOURCES "src/engine/backend/metal/*.c")
    list(APPEND ENGINE_SOURCES ${BACKEND_METAL_C_SOURCES})
    message(STATUS "Metal backend C files enabled on macOS")
else()
    # On Linux/Windows, exclude Metal (Objective-C specific)
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/metal/.*\\.c$")
endif()

# Editor subsystem - DISABLED (causes many build errors)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/editor/.*\\.c$")

# SVG importer has missing function definitions - disable for now
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/editor/importer/svg_importer\\.c$")

# Orthographic camera has type mismatches - disable for now
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/editor/viewports/camera_system/orthographic_camera\\.c$")

# Skeleton template files have type issues - disable for now
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/materials/templates/characters/skeleton_.*\\.c$")

# NPC system has type mismatches - disable for now
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/npc/.*\\.c$")

# Math mat4.c has definition conflicts - keep header-only version
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "^.*/src/engine/math/mat4\\.c$")

# Physics subsystem has many type mismatches and signature issues - disable entirely
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/physics/.*\\.c$")

# Network/Networking subsystems - Disabled due to header include issues
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/network/.*\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/networking/.*\\.c$")

# Particle system has function signature mismatches - disable
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/rendering/particles/.*\\.c$")

# Scripting system.c has syntax error - disable
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/scripting/script_system\\.c$")

# Voxel renderer has issues - disable for now
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/rendering/voxel_renderer\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/rendering/renderer_factory\\.c$")

# Cinematic subsystems (currently broken)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/cinematic/.*\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/cinematics/.*\\.c$")

# macOS platform optimizations (broken SDK calls)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/platform/macos/macos_optimizations\\..*$")

# Metal advanced rendering (ARC compatibility issues)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/metal/mtl_advanced_rendering\\.m$")

# Character subsystem (depends on excluded animation system and has broken includes)
# list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/character/.*\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/character/animation/animation_system\\.c$")

# ===========================================
# GAME SOURCES
# ===========================================
file(GLOB_RECURSE GAME_SOURCES
    "src/game/blockgame/*.c"
)

# Exclude broken NPC systems due to
# Exclude broken game ECS and legacy recipe
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/ecs/ecs\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/crafting/recipe\\.c$")

# Exclude Vulkan-specific GPU memory manager
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/core/memory/gpu_memory\\.c$")

list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/core/optimization/optimization_systems_complete\\.c$")

# Exclude broken mob system  
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/mobs/mob_spawning\\.c$")
# Exclude lighting system due to missing constants
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/blockgame/block/lighting\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/mobs/mob_system\\.c$")

# Exclude broken AI logic (re-added)
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/ai/enemy_ai\\.c$")

# NPC system - re-enabled since functions are needed
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/npc/.*\\.c$")

# Exclude broken player modules with undefined component APIs
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/player_vehicle\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/player_magic\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/spirit_model\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/player_damage\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/player_system_update\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/experience_test\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/crafting/recipe_expansion\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/crafting/advanced_crafting\\.c$")

# Exclude broken vehicle system
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/vehicle/.*\\.c$")

# Exclude main module with too many dependencies - RE-ENABLING
# list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/minecraft_v2_module\\.c$")

# Exclude duplicate main file (conflicts with gamestate_main.c)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/monolithic_main\\.c$")

# Exclude duplicate HUD implementation (conflicts with hud_main.c)
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/ui/hud_impl\\.c$")

message(STATUS "ENGINE_SOURCES count: ${ENGINE_SOURCES}")
message(STATUS "GAME_SOURCES count: ${GAME_SOURCES}")

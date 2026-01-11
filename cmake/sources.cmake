# cmake/sources.cmake
# Auto-generated source file lists for BlockGame engine and game

# ===========================================
# ENGINE SOURCES
# ===========================================
file(GLOB_RECURSE ENGINE_SOURCES
    # Core subdirectory
    "src/engine/core/*.c"
    
    # AI subdirectory
    "src/engine/ai/*.c"
    
    # Animation subdirectory
    "src/engine/animation/*.c"
    
    # Assets subdirectory (importers, loaders)
    "src/engine/assets/*.c"
    
    # Audio subdirectory - TEMPORARILY DISABLED due to API mismatches and missing headers
    # "src/engine/audio/*.c"
    "src/engine/audio/audio_system.c"
    "src/engine/audio/underwater_filter_stubs.c"
    "src/engine/audio/audio_loader.c"
    "src/engine/audio/audio_reverb.c"
    
    # Core stubs
    "src/engine/core/misc_stubs.c"
    
    # Math impl - DISABLED due to old type names
    # "src/engine/math/mat4.c"
    # "src/engine/math/quat.c"
    
    # Rendering 
    "src/engine/rendering/core/mesh.c"
    
    # Scripting
    "src/engine/scripting/script_system.c"
    
    # Backend subdirectory - Handled separately below to avoid Vulkan inclusion
    # "src/engine/backend/*.c"
    # "src/engine/backend/metal/*.c"
    
    # Platform files - DISABLED swift_bridge due to function call issues
    # "src/engine/platform/swift_bridge.c"
    
    # Character subdirectory
    "src/engine/character/*.c"
    
    # Cinematic subdirectory
    "src/engine/cinematic/*.c"
    "src/engine/cinematics/*.c"
    
    # ECS subdirectory
    "src/engine/ecs/*.c"
    
    # Editor subdirectory
    "src/engine/editor/*.c"
    
    # Effects subdirectory - TEMPORARILY DISABLED GPU particles due to ObjC compilation issues
    # "src/engine/effects/*.c"
    "src/engine/effects/destruction/fracture_system.c"
    
    # Environment subdirectory
    "src/engine/environment/*.c"
    
    # Combat subdirectory - TEMPORARILY DISABLED status_effects due to missing header
    # "src/engine/gameplay/combat/*.c"
    "src/engine/gameplay/combat/hitbox.c"
    "src/engine/gameplay/combat/projectile.c"
    
    # Geometry subdirectory - TEMPORARILY DISABLED mesh_gpu due to missing header
    # "src/engine/geometry/*.c"
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
    
    # Materials subdirectory - TEMPORARILY DISABLED customization_loader due to struct issues
    # "src/engine/materials/*.c"
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
    
    # Physics subdirectory - CONSOLIDATED: Only essential files - DISABLED due to missing headers
    # Using block physics as canonical voxel physics implementation
    # "src/engine/physics/block_physics.c"
    # Collision detection
    "src/engine/physics/collision/collision_gjk_epa.c"
    "src/engine/physics/collision/gjk_solver.c"
    "src/engine/physics/collision/epa_solver.c"
    # Broadphase
    "src/engine/physics/broadphase/aabb_tree.c"
    # Contact generation
    "src/engine/physics/narrowphase/contact_manifold.c"
    # Physics integration (links everything together)
    "src/engine/physics/integration/physics_integration.c"
    # Math/queries
    "src/engine/physics/queries/physics_queries.c"

    # DISABLED SUBSYSTEMS (Future work):
    # - Rigid body dynamics
    # - Cloth simulation
    # - Fluid simulation
    # - Particle physics
    # - Ragdoll physics
    # - Vehicle dynamics
    # - Destruction/fracture
    
    # Platform subdirectory - DISABLED due to swift_bridge issues
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

# Audio core and dsp (has missing headers and API mismatches)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/audio/core/.*\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/audio/dsp/.*\\.c$")

# Vulkan backend (not needed for macOS/Metal build and has compilation errors)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/vulkan/.*\\.c$")

# Metal backend .c files (mostly broken synchronization examples/stubs)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/metal/.*\\.c$")

# Editor subsystem (currently broken, excluding to unblock core verification)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/editor/.*\\.c$")

# Network/Networking subsystems (currently broken)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/network/.*\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/networking/.*\\.c$")

# Cinematic subsystems (currently broken)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/cinematic/.*\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/cinematics/.*\\.c$")

# macOS platform optimizations (broken SDK calls)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/platform/macos/macos_optimizations\\..*$")

# Character subsystem (depends on excluded animation system and has broken includes)
# list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/character/.*\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/character/animation/animation_system\\.c$")

# ===========================================
# GAME SOURCES
# ===========================================
file(GLOB_RECURSE GAME_SOURCES
    "src/game/blockgame/*.c"
)

# Exclude any demo or test files
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*demo.*\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*test.*\\.c$")

# Exclude Vulkan-specific GPU memory manager
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/core/memory/gpu_memory\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/core/services/particle_system_gpu\\.c$")

# Exclude redundant optimization complete file
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/core/optimization/optimization_systems_complete\\.c$")

# Exclude broken mob system  
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/mobs/mob_spawning\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/mobs/mob_system\\.c$")

# Exclude broken AI logic (re-added)
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/ai/enemy_ai\\.c$")

# Exclude entire NPC subsystem (deprecated ECS API usage)
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/npc/.*\\.c$")

# Exclude broken player modules with undefined component APIs
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/player_vehicle\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/player_damage\\.c$")

# Exclude broken game ECS and legacy recipe
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/ecs/ecs\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/crafting/recipe\\.c$")

# Exclude broken crafting modules
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

# cmake/sources.cmake
# Auto-generated source file lists for MinecraftV2 engine and game

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
    
    # Math impl
    "src/engine/math/mat4.c"
    "src/engine/math/quat.c"
    
    # Rendering 
    "src/engine/rendering/core/mesh.c"
    
    # Scripting
    "src/engine/scripting/script_system.c"
    
    # Backend subdirectory - Handled separately below to avoid Vulkan inclusion
    # "src/engine/backend/*.c"
    # "src/engine/backend/metal/*.c"
    
    # Character subdirectory
    "src/engine/character/*.c"
    
    # Cinematic subdirectory
    "src/engine/cinematic/*.c"
    "src/engine/cinematics/*.c"
    
    # ECS subdirectory
    "src/engine/ecs/*.c"
    
    # Editor subdirectory
    "src/engine/editor/*.c"
    
    # Effects subdirectory
    "src/engine/effects/*.c"
    
    # Environment subdirectory
    "src/engine/environment/*.c"
    
    # Gameplay subdirectory
    "src/engine/gameplay/*.c"
    
    # Geometry subdirectory
    "src/engine/geometry/*.c"
    
    # Lighting subdirectory
    "src/engine/lighting/*.c"
    
    # Materials subdirectory
    "src/engine/materials/*.c"
    
    # Math subdirectory
    "src/engine/math/*.c"
    
    # Network subdirectory
    "src/engine/network/*.c"
    "src/engine/networking/*.c"
    
    # Physics subdirectory
    "src/engine/physics/*.c"
    
    # Platform subdirectory
    "src/engine/platform/*.c"
    
    # Post-processing subdirectory
    "src/engine/postprocess/*.c"
    
    # Rendering subdirectory  
    "src/engine/rendering/*.c"
    
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
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/simd_math_(x86|neon)\\.c$")

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

# ===========================================
# GAME SOURCES
# ===========================================
file(GLOB_RECURSE GAME_SOURCES
    "src/game/minecraftv2/*.c"
)

# Exclude any demo or test files
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*demo.*\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*test.*\\.c$")

# Exclude Vulkan-specific GPU memory manager
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/core/memory/gpu_memory\\.c$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/core/services/particle_system_gpu\\.c$")

# Exclude broken mob system  
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/mobs/mob_spawning\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/mobs/mob_system\\.c$")

# Exclude broken AI logic (re-added)
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/ai/enemy_ai\\.c$")

# Exclude entire NPC subsystem (deprecated ECS API usage)
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/npc/.*\\.c$")

# Exclude player modules with missing component IDs
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/player_damage\\.c$")
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/player/experience_system\\.c$")

# Exclude main module with too many dependencies
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/minecraft_v2_module\\.c$")

# Exclude duplicate main file (conflicts with gamestate_main.c)
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/monolithic_main\\.c$")

# Exclude duplicate HUD implementation (conflicts with hud_main.c)
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/ui/hud_impl\\.c$")

message(STATUS "ENGINE_SOURCES count: ${ENGINE_SOURCES}")
message(STATUS "GAME_SOURCES count: ${GAME_SOURCES}")

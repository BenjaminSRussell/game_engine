# Automatic Source Discovery
# -------------------------
# Engine Sources
file(GLOB_RECURSE ENGINE_CORE_SOURCES
    "src/engine/*.c"
    "src/engine/*.m"
    "src/engine/*.mm"
    "src/engine/*.cpp"
)

# Game Sources
file(GLOB_RECURSE GAME_SOURCES
    "src/game/minecraftv2/*.c"
    "src/game/minecraftv2/*.m"
    "src/game/minecraftv2/*.mm"
    "src/game/minecraftv2/*.cpp"
)

set(ENGINE_SOURCES ${ENGINE_CORE_SOURCES} ${GAME_SOURCES})

# Filter out conditional specific backends to add them explicitly if dependencies met
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/vulkan/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/metal/old/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/backend/metal/tests/.*")

# Isolate broken subsystems for Tier 2 Minimal Build
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/ai/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/animation/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/audio/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/physics/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/scripting/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/core/monolithic_main.c")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/core/main.c")

# Add Stub Implementations (re-adding them after filters removed them)
list(APPEND ENGINE_SOURCES
    "src/engine/audio/audio_stubs.c"
    "src/engine/physics/physics_stubs.c"
    "src/engine/scripting/script_stubs.c"
)

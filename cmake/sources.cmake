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

# Isolate broken subsystems for Tier 2 Minimal Build
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/ai/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/animation/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/audio/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/physics/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/scripting/.*")


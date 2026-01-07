# Add engine bridge to CMake build

# Engine bridge library
add_library(engine_bridge STATIC
    platform/engine_bridge.c
    platform/engine_bridge.h
)

target_include_directories(engine_bridge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link bridge to main engine
target_link_libraries(voxelforge_engine PUBLIC
    engine_bridge
)

# Install bridge header for Swift
install(FILES platform/engine_bridge.h
    DESTINATION include/voxelforge
)

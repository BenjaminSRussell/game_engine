# Spatial Audio Tests

cmake_minimum_required(VERSION 3.20)

# Include paths to source files
include_directories(
    ${CMAKE_SOURCE_DIR}/src/engine/audio/spatial
    ${CMAKE_SOURCE_DIR}/src/engine
)

# Test executable for configuration
add_executable(test_spatial_audio_config
    test_spatial_audio_config.c
    ${CMAKE_SOURCE_DIR}/src/engine/audio/spatial/spatial_audio_config.c
)
target_link_libraries(test_spatial_audio_config m)

# Test executable for Dolby Atmos renderer
add_executable(test_dolby_atmos_renderer
    test_dolby_atmos_renderer.c
    ${CMAKE_SOURCE_DIR}/src/engine/audio/spatial/spatial_audio_config.c
    ${CMAKE_SOURCE_DIR}/src/engine/audio/spatial/dolby_atmos_renderer.c
)
target_link_libraries(test_dolby_atmos_renderer m)

# Test executable for bridge API
add_executable(test_spatial_audio_bridge
    test_spatial_audio_bridge.c
    ${CMAKE_SOURCE_DIR}/src/engine/audio/spatial/spatial_audio_config.c
    ${CMAKE_SOURCE_DIR}/src/engine/audio/spatial/dolby_atmos_renderer.c
    ${CMAKE_SOURCE_DIR}/src/engine/audio/spatial/spatial_audio_bridge.c
)
target_link_libraries(test_spatial_audio_bridge m)

# Add tests to CTest
enable_testing()

add_test(NAME SpatialAudioConfig COMMAND test_spatial_audio_config)
add_test(NAME DolbyAtmosRenderer COMMAND test_dolby_atmos_renderer)
add_test(NAME SpatialAudioBridge COMMAND test_spatial_audio_bridge)

# Custom target to run all spatial audio tests
add_custom_target(test_spatial_audio
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "SpatialAudio|DolbyAtmos"
    DEPENDS test_spatial_audio_config test_dolby_atmos_renderer test_spatial_audio_bridge
    COMMENT "Running all spatial audio tests"
)

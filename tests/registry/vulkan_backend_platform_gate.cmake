# Issue #256. Script test: evaluates cmake/sources.cmake with APPLE on and off.
# Not add_engine_test: that macro links Engine, which does not build on origin/main.
add_test(
    NAME vulkan_backend_platform_gate
    COMMAND ${CMAKE_COMMAND} --log-level=WARNING
        -DSOURCE_CMAKE=${CMAKE_SOURCE_DIR}/cmake/sources.cmake
        -P ${CMAKE_SOURCE_DIR}/tests/cmake/vulkan_backend_platform_gate.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

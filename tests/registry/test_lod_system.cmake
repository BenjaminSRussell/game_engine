# LOD registration, screen-size selection, bias, and forced level.
# Not add_engine_test: that macro links Engine and
# tests/unit/test_framework_unified.c, which is missing. Engine does not
# build on origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_lod_system
    tests/lod_system_test.c
    src/engine/geometry/lod/lod_system.c
)
target_include_directories(test_lod_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_lod_system PRIVATE m)
add_test(NAME test_lod_system COMMAND test_lod_system)

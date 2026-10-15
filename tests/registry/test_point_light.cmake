# Point light create and source-parameter setters.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles point_light.c
# directly.
add_executable(test_point_light
    tests/point_light_test.c
    src/engine/lighting/sources/point_light.c
)
target_include_directories(test_point_light PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
if(NOT APPLE)
    # include/simd/simd.h includes a missing math/types.h off Apple.
    target_include_directories(test_point_light BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/tests/support/point_light
    )
endif()
add_test(NAME test_point_light COMMAND test_point_light)

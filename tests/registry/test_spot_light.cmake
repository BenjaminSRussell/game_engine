# Issue #364: spot light create, IES profile, and source radius.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles spot_light.c
# directly. The support include is listed first so hosts without Apple
# simd headers do not pick up the engine fallback.
add_executable(test_spot_light
    tests/spot_light_test.c
    src/engine/lighting/sources/spot_light.c
)
target_include_directories(test_spot_light BEFORE PRIVATE
    ${CMAKE_SOURCE_DIR}/tests/support/spot_light
)
target_include_directories(test_spot_light PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/engine/include
)
add_test(NAME test_spot_light COMMAND test_spot_light)

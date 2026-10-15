# Procedural city config, footprint, and generator lifecycle.
# Not add_engine_test: that macro links Engine and
# tests/unit/test_framework_unified.c, which is missing. Engine does not
# build on origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_procedural_city
    tests/procedural_city_test.c
    src/engine/environment/procedural/procedural_city.c
)
target_include_directories(test_procedural_city PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/environment/procedural
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_procedural_city PRIVATE m)
add_test(NAME test_procedural_city COMMAND test_procedural_city)

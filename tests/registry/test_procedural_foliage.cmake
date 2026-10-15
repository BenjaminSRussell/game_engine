# Issue #345: procedural foliage placement. Not add_engine_test: that macro
# links Engine, which does not build on origin/main (missing GLFW, then
# gpu_profiler.c). Compiles procedural_foliage.c directly.
add_executable(test_procedural_foliage
    tests/procedural_foliage_test.c
    src/engine/environment/procedural/procedural_foliage.c
)
target_include_directories(test_procedural_foliage PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_procedural_foliage PRIVATE m)
add_test(NAME test_procedural_foliage COMMAND test_procedural_foliage)

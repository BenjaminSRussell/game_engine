# Node-graph evaluation in visual_scripting.c (tests/visual_scripting_test.c).
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles visual_scripting.c
# directly and stubs the allocator.
add_executable(test_visual_scripting
    tests/visual_scripting_test.c
    src/engine/scripting/visual_scripting.c
)
target_include_directories(test_visual_scripting PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_visual_scripting COMMAND test_visual_scripting)
# A regression that loops inside vs_graph_execute should time out, not hang the run.
set_tests_properties(test_visual_scripting PROPERTIES TIMEOUT 30)

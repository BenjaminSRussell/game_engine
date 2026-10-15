# Issue #231. Unit test for the developer console command parser and dispatcher
# (src/engine/scripting/console.c). Not add_engine_test: that macro links Engine,
# which does not build on origin/main (missing GLFW, then gpu_profiler.c).
# The test stubs the logger, so console.c is the only engine source it compiles.
add_executable(test_console
    tests/unit/scripting/console_test.c
    src/engine/scripting/console.c)
target_include_directories(test_console PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_console COMMAND test_console)

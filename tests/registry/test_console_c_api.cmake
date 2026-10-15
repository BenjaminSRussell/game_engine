# Issue #439: console.c must stay valid C when ENABLE_IMGUI is set.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c:80). Compiles console.c
# directly against the cimgui-shaped C API.
add_executable(test_console_c_api
    tests/console_c_api_test.c
    src/engine/scripting/console.c
)
target_compile_definitions(test_console_c_api PRIVATE ENABLE_IMGUI)
target_include_directories(test_console_c_api PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/engine/scripting
)
add_test(NAME test_console_c_api COMMAND test_console_c_api)

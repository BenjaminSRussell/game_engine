# Issue #342: grass instance counts and distance culling.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c:80). Compiles grass_renderer.c
# directly. No GPU, GLFW, or Vulkan headers.
add_executable(test_grass_renderer
    tests/grass_renderer_test.c
    src/engine/environment/landscape/terrain/landscape/vegetation/grass_renderer.c
)
target_include_directories(test_grass_renderer PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_compile_options(test_grass_renderer PRIVATE
    -Werror=implicit-function-declaration
)
add_test(NAME test_grass_renderer COMMAND test_grass_renderer)

# Issue #402: unit tests for src/engine/scene/world_building/weather/weather_impl.c.
# Not add_engine_test: that macro links Engine, which does not build on origin/main
# (missing GLFW, then gpu_profiler.c:80). The unit is compiled directly instead.
add_executable(test_weather_impl
    tests/weather_impl_test.c
    src/engine/scene/world_building/weather/weather_impl.c
)
target_include_directories(test_weather_impl PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/scene/world_building/weather
    ${CMAKE_SOURCE_DIR}/src/engine/include/core/memory
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_compile_options(test_weather_impl PRIVATE
    -Werror=implicit-function-declaration
)
# weather_impl.c calls fminf, which lives in libm on Linux.
target_link_libraries(test_weather_impl PRIVATE m)
add_test(NAME test_weather_impl COMMAND test_weather_impl)

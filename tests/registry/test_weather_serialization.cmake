# Issue #424: weather_serialization.c must build, round-trip its JSON and binary forms, and read
# its JSON helpers. Not add_engine_test: that macro links Engine, which does not build on main.
# Compile the unit and weather_manager_init (weather_system.c) directly instead.
add_executable(test_weather_serialization
    tests/weather_serialization_test.c
    src/engine/scene/world_building/weather/weather_serialization.c
    src/engine/scene/world_building/weather/weather_system.c
)
target_include_directories(test_weather_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/scene/world_building/weather
    ${CMAKE_SOURCE_DIR}/src/engine/include/core/memory
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_weather_serialization COMMAND test_weather_serialization)

# Issue #141: biome-specific vegetation placement in mega_terrain.c.
# Not add_engine_test: that macro links Engine, which does not build on origin/main.
# Compiles the unit directly; the shim supplies what mega_terrain.c needs to compile
# (pre-existing errors tracked in #275).
add_executable(test_mega_terrain_vegetation
    tests/unit/terrain/mega_terrain_vegetation_test.c
    src/engine/scene/world/terrain_mega/mega_terrain.c)
target_include_directories(test_mega_terrain_vegetation PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/scene/world/terrain_mega)
target_compile_options(test_mega_terrain_vegetation PRIVATE
    -include ${CMAKE_SOURCE_DIR}/tests/unit/terrain/mega_terrain_test_shim.h)
target_link_libraries(test_mega_terrain_vegetation PRIVATE m)
add_test(NAME test_mega_terrain_vegetation COMMAND test_mega_terrain_vegetation)

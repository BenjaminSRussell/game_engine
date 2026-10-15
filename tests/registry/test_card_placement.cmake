# Issue #359: Lumen surface-cache card placement slot table.
# Not add_engine_test: that macro links Engine, which does not build on
# origin/main (missing GLFW, then gpu_profiler.c). Compiles card_placement.c
# directly.
add_executable(test_card_placement
    tests/card_placement_test.c
    src/engine/lighting/lumen/surface_cache/card_placement.c
)
target_include_directories(test_card_placement PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/lighting/lumen/surface_cache
)
add_test(NAME test_card_placement COMMAND test_card_placement)

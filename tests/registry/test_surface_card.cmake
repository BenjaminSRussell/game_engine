# Issue #361. Unit tests for the Lumen surface card pool. The test includes
# surface_card.c directly, so it is not add_engine_test: that macro links Engine,
# which does not build on origin/main (missing GLFW, then gpu_profiler.c:80).
add_executable(test_surface_card
    tests/unit/lighting/surface_card_test.c)
target_include_directories(test_surface_card PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_surface_card COMMAND test_surface_card)

# Issue #227: unit test for src/engine/rendering/post_processing/taa_compute.c.
# Not add_engine_test: that macro links Engine, which does not build on main.
# The compute backend, texture and allocator calls are faked in the test.
add_executable(test_taa_compute
    tests/taa_compute_test.c
    src/engine/rendering/post_processing/taa_compute.c
)
target_include_directories(test_taa_compute PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_taa_compute COMMAND test_taa_compute)

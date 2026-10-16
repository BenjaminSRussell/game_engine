# Issue #110: bloom_add_to_graph sizes its textures from the resolution given to bloom_create.
# Compiles bloom.c directly, because the Engine library does not build on this host.
add_executable(test_bloom_resolution
    tests/unit/rendering/bloom_resolution_test.c
    src/engine/rendering/post_processing/bloom.c)
target_include_directories(test_bloom_resolution PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_bloom_resolution COMMAND test_bloom_resolution)

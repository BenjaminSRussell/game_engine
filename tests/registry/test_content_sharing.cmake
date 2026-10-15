# Package checksums, validation, and local content statistics.
# Not add_engine_test: that macro links Engine, which does not build
# on origin/main (missing GLFW, then gpu_profiler.c). Compiles
# content_sharing_system.c directly. SHA-256 is internal, so this test
# does not need OpenSSL or libcurl.
add_executable(test_content_sharing
    tests/content_sharing_test.c
    src/engine/network/content_sharing_system.c
    src/engine/core/utils.c
)
target_include_directories(test_content_sharing PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
add_test(NAME test_content_sharing COMMAND test_content_sharing)

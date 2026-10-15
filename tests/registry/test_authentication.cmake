# Anti-cheat movement, combat, kick, and ban checks.
# Not add_engine_test: that macro links Engine and
# tests/unit/test_framework_unified.c, which is missing. Engine does not
# build on origin/main (missing GLFW, then gpu_profiler.c).
add_executable(test_authentication
    tests/authentication_test.c
    src/engine/network/security/authentication.c
)
target_include_directories(test_authentication PRIVATE
    ${CMAKE_SOURCE_DIR}/src/engine/include
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(test_authentication PRIVATE m)
add_test(NAME test_authentication COMMAND test_authentication)
